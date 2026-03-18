#include "SpectralEngine.h"
#include <cmath>

SpectralEngine::SpectralEngine()
    : fft    (kFFTOrder),
      window ((size_t) kFFTSize, juce::dsp::WindowingFunction<float>::hann, false)
{
    fftScratch.resize    (2 * kFFTSize, 0.0f);
    inputMag.resize      (kNumBins, 0.0f);
    inputPhase.resize    (kNumBins, 0.0f);
    liveEnvelope.resize  (kNumBins, 0.0f);
    donorMag.resize      (kNumBins, 0.0f);
    donorEnvelope.resize (kNumBins, 0.0f);

    donorBuffer.setSize (2, kMaxDonorSamples);
    donorBuffer.clear();

    ch[0].init();
    ch[1].init();
}

void SpectralEngine::prepare (double /*sampleRate*/, int /*blockSize*/)
{
    donorBuffer.clear();
    donorWritePos  = 0;
    donorLength    = 0;
    donorReadPos   = 0;
    donorRecording = false;
    hasDonor       = false;
    hopCount       = 0;
    grainSpawnCounter = 0;

    ch[0].init();
    ch[1].init();

    for (auto& gr : grains) gr.active = false;

    std::fill (donorMag.begin(),      donorMag.end(),      0.0f);
    std::fill (donorEnvelope.begin(), donorEnvelope.end(), 0.0f);
}

//==============================================================================
void SpectralEngine::startRecording()
{
    if (donorRecording) return;
    donorBuffer.clear();
    donorWritePos  = 0;
    donorLength    = 0;
    donorReadPos   = 0;
    hasDonor       = false;
    donorRecording = true;
}

void SpectralEngine::stopRecording()
{
    if (!donorRecording) return;
    donorRecording = false;
    hasDonor       = donorLength >= kFFTSize;
    donorReadPos   = 0;
    if (hasDonor)
        analyseDonorFrame();
}

float SpectralEngine::getDonorFillLevel() const
{
    return (float) donorLength / (float) kMaxDonorSamples;
}

//==============================================================================
// Spectral envelope via prefix-sum box filter (O(N) per call).
void SpectralEngine::computeEnvelope (const float* mag, float* env, int numBins, int halfWindow) const
{
    std::array<float, kNumBins + 1> prefix;
    prefix[0] = 0.0f;
    for (int k = 0; k < numBins; ++k)
        prefix[k + 1] = prefix[k] + mag[k];

    for (int k = 0; k < numBins; ++k)
    {
        const int lo = std::max (0, k - halfWindow);
        const int hi = std::min (numBins - 1, k + halfWindow);
        env[k] = (prefix[hi + 1] - prefix[lo]) / (float) (hi - lo + 1);
    }
}

//==============================================================================
// Analyse a kFFTSize frame from the donor buffer at donorReadPos.
// Stores magnitude spectrum in donorMag[] and envelope in donorEnvelope[].
void SpectralEngine::analyseDonorFrame()
{
    if (donorLength < kFFTSize) return;

    for (int i = 0; i < kFFTSize; ++i)
        fftScratch[i] = donorBuffer.getSample (0, (donorReadPos + i) % donorLength);

    std::fill (fftScratch.begin() + kFFTSize, fftScratch.end(), 0.0f);
    window.multiplyWithWindowingTable (fftScratch.data(), (size_t) kFFTSize);
    fft.performRealOnlyForwardTransform (fftScratch.data(), false);

    for (int k = 0; k < kNumBins; ++k)
    {
        const float re = fftScratch[2 * k];
        const float im = fftScratch[2 * k + 1];
        donorMag[k] = std::sqrt (re * re + im * im);
    }

    computeEnvelope (donorMag.data(), donorEnvelope.data(), kNumBins, 50);
}

//==============================================================================
void SpectralEngine::processFFTFrame (int chIdx)
{
    auto& cs = ch[chIdx];

    // Build analysis frame (oldest sample first)
    for (int i = 0; i < kFFTSize; ++i)
        fftScratch[i] = cs.inputRing[(cs.inputWritePos + i) & (kFFTSize - 1)];

    std::fill (fftScratch.begin() + kFFTSize, fftScratch.end(), 0.0f);
    window.multiplyWithWindowingTable (fftScratch.data(), (size_t) kFFTSize);
    fft.performRealOnlyForwardTransform (fftScratch.data(), false);

    // Extract magnitudes and phases
    for (int k = 0; k < kNumBins; ++k)
    {
        const float re = fftScratch[2 * k];
        const float im = fftScratch[2 * k + 1];
        inputMag[k]   = std::sqrt (re * re + im * im);
        inputPhase[k] = std::atan2 (im, re);
    }

    // Compute live spectral envelope (only when formant transfer is active)
    const float fm = formant;
    if (hasDonor && fm > 0.0f)
        computeEnvelope (inputMag.data(), liveEnvelope.data(), kNumBins, 50);

    // Morph blend + formant transfer → reconstruct complex spectrum
    const float m = morph;
    for (int k = 0; k < kNumBins; ++k)
    {
        // Spectral magnitude morph
        float blended = hasDonor ? inputMag[k] * (1.0f - m) + donorMag[k] * m
                                 : inputMag[k];

        // Formant envelope transfer: shape blended spectrum with donor's formants
        if (hasDonor && fm > 0.0f)
        {
            const float liveEnv = std::max (liveEnvelope[k], 1e-6f);
            const float ratio   = juce::jlimit (0.1f, 10.0f, donorEnvelope[k] / liveEnv);
            blended *= juce::jmax (0.0f, 1.0f + fm * (ratio - 1.0f));
        }

        fftScratch[2 * k]     = blended * std::cos (inputPhase[k]);
        fftScratch[2 * k + 1] = blended * std::sin (inputPhase[k]);
    }

    // Mirror conjugate symmetry for real IFFT output
    for (int k = 1; k < kFFTSize / 2; ++k)
    {
        fftScratch[2 * (kFFTSize - k)]     =  fftScratch[2 * k];
        fftScratch[2 * (kFFTSize - k) + 1] = -fftScratch[2 * k + 1];
    }

    // IFFT + normalize (1/(2N) for 4x Hann OLA)
    fft.performRealOnlyInverseTransform (fftScratch.data());
    const float scale = 1.0f / (2.0f * (float) kFFTSize);

    for (int i = 0; i < kFFTSize; ++i)
        cs.outputAccum[i] += fftScratch[i] * scale;

    // Deliver kHopSize ready samples, shift accumulator
    std::copy (cs.outputAccum.begin(), cs.outputAccum.begin() + kHopSize, cs.outputQueue.begin());
    std::copy (cs.outputAccum.begin() + kHopSize, cs.outputAccum.end(), cs.outputAccum.begin());
    std::fill (cs.outputAccum.end() - kHopSize, cs.outputAccum.end(), 0.0f);

    cs.outputQueuePos = 0;
}

//==============================================================================
void SpectralEngine::spawnGrain()
{
    for (auto& gr : grains)
    {
        if (gr.active) continue;

        gr.size   = kGrainSamples;
        gr.age    = 0;
        gr.active = true;

        const int scatterRange = (int) (scatter * (float) donorLength * 0.25f);
        const int offset = scatterRange > 0
                               ? (int) ((rng.nextFloat() * 2.0f - 1.0f) * (float) scatterRange)
                               : 0;
        gr.readPos = (donorReadPos + offset + donorLength) % donorLength;
        return;
    }
    // No free grain slot — drop
}

void SpectralEngine::processGrains (juce::AudioBuffer<float>& buffer, int numCh, int numSamples)
{
    const float g = grain;
    const int spawnInterval = 256 + (int) (3840.0f * (1.0f - g));

    for (int i = 0; i < numSamples; ++i)
    {
        if (++grainSpawnCounter >= spawnInterval)
        {
            grainSpawnCounter = 0;
            spawnGrain();
        }

        float grainOut = 0.0f;
        for (auto& gr : grains)
        {
            if (!gr.active) continue;

            const float t   = (float) gr.age / (float) (gr.size - 1);
            const float env = 0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi * t));
            grainOut += donorBuffer.getSample (0, gr.readPos % donorLength) * env;

            if (++gr.readPos, ++gr.age >= gr.size)
                gr.active = false;
        }

        const float out = grainOut * g * 0.2f;
        for (int c = 0; c < numCh; ++c)
            buffer.addSample (c, i, out);
    }
}

//==============================================================================
void SpectralEngine::process (juce::AudioBuffer<float>& buffer)
{
    const int numCh      = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    // Donor capture
    if (donorRecording)
    {
        const int toWrite = juce::jmin (numSamples, kMaxDonorSamples - donorWritePos);
        for (int c = 0; c < numCh; ++c)
            for (int i = 0; i < toWrite; ++i)
                donorBuffer.setSample (c, donorWritePos + i, buffer.getSample (c, i));
        donorWritePos += toWrite;
        donorLength    = donorWritePos;
        if (donorWritePos >= kMaxDonorSamples)
            stopRecording();
    }

    // FFT overlap-add synthesis (per sample)
    const float dw = dryWet;

    for (int i = 0; i < numSamples; ++i)
    {
        for (int c = 0; c < numCh; ++c)
        {
            ch[c].inputRing[ch[c].inputWritePos] = buffer.getSample (c, i);
            ch[c].inputWritePos = (ch[c].inputWritePos + 1) & (kFFTSize - 1);
        }

        if (++hopCount >= kHopSize)
        {
            hopCount = 0;
            for (int c = 0; c < numCh; ++c)
                processFFTFrame (c);

            if (hasDonor && donorLength > 0)
            {
                donorReadPos = (donorReadPos + kHopSize) % donorLength;
                analyseDonorFrame();
            }
        }

        for (int c = 0; c < numCh; ++c)
        {
            const float dry = buffer.getSample (c, i);
            const float wet = (ch[c].outputQueuePos < kHopSize)
                                  ? ch[c].outputQueue[ch[c].outputQueuePos++]
                                  : 0.0f;
            buffer.setSample (c, i, dry * (1.0f - dw) + wet * dw);
        }
    }

    // Granular texture (added on top of morphed signal)
    if (hasDonor && grain > 0.0f)
        processGrains (buffer, numCh, numSamples);
}
