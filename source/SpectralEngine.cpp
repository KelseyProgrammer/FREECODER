#include "SpectralEngine.h"
#include <cmath>

SpectralEngine::SpectralEngine()
    : fft    (kFFTOrder),
      window ((size_t) kFFTSize, juce::dsp::WindowingFunction<float>::hann, false)
{
    fftScratch.resize (2 * kFFTSize, 0.0f);
    donorMag.resize   (kFFTSize / 2 + 1, 0.0f);
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
    ch[0].init();
    ch[1].init();
    std::fill (donorMag.begin(), donorMag.end(), 0.0f);
}

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
// Analyse a kFFTSize frame from the donor buffer at donorReadPos,
// storing the magnitude spectrum in donorMag[].
void SpectralEngine::analyseDonorFrame()
{
    if (donorLength < kFFTSize)
        return;

    for (int i = 0; i < kFFTSize; ++i)
        fftScratch[i] = donorBuffer.getSample (0, (donorReadPos + i) % donorLength);

    std::fill (fftScratch.begin() + kFFTSize, fftScratch.end(), 0.0f);
    window.multiplyWithWindowingTable (fftScratch.data(), (size_t) kFFTSize);
    fft.performRealOnlyForwardTransform (fftScratch.data(), false);

    for (int k = 0; k <= kFFTSize / 2; ++k)
    {
        const float re = fftScratch[2 * k];
        const float im = fftScratch[2 * k + 1];
        donorMag[k] = std::sqrt (re * re + im * im);
    }
}

//==============================================================================
// Process one FFT overlap-add frame for a single channel.
void SpectralEngine::processFFTFrame (int chIdx)
{
    auto& cs = ch[chIdx];

    // Build analysis frame: oldest sample first from the circular ring
    for (int i = 0; i < kFFTSize; ++i)
        fftScratch[i] = cs.inputRing[(cs.inputWritePos + i) & (kFFTSize - 1)];

    std::fill (fftScratch.begin() + kFFTSize, fftScratch.end(), 0.0f);

    // Analysis window + forward FFT
    window.multiplyWithWindowingTable (fftScratch.data(), (size_t) kFFTSize);
    fft.performRealOnlyForwardTransform (fftScratch.data(), false);

    // Spectral magnitude morphing (keep live input phase)
    const float m = morph;
    for (int k = 0; k <= kFFTSize / 2; ++k)
    {
        const float re    = fftScratch[2 * k];
        const float im    = fftScratch[2 * k + 1];
        const float mag   = std::sqrt (re * re + im * im);
        const float phase = std::atan2 (im, re);
        const float blended = hasDonor ? mag * (1.0f - m) + donorMag[k] * m : mag;

        fftScratch[2 * k]     = blended * std::cos (phase);
        fftScratch[2 * k + 1] = blended * std::sin (phase);
    }

    // Mirror conjugate symmetry so IFFT produces real output
    for (int k = 1; k < kFFTSize / 2; ++k)
    {
        fftScratch[2 * (kFFTSize - k)]     =  fftScratch[2 * k];
        fftScratch[2 * (kFFTSize - k) + 1] = -fftScratch[2 * k + 1];
    }

    // Inverse FFT
    fft.performRealOnlyInverseTransform (fftScratch.data());

    // Scale: 1/(N * overlap_factor).  For 4x Hann overlap, sum of windows = 2, so 1/(2N).
    const float scale = 1.0f / (2.0f * (float) kFFTSize);

    // Overlap-add into accumulator
    for (int i = 0; i < kFFTSize; ++i)
        cs.outputAccum[i] += fftScratch[i] * scale;

    // Deliver the next kHopSize ready samples
    std::copy (cs.outputAccum.begin(),
               cs.outputAccum.begin() + kHopSize,
               cs.outputQueue.begin());

    // Shift accumulator left by kHopSize, zero the tail
    std::copy (cs.outputAccum.begin() + kHopSize,
               cs.outputAccum.end(),
               cs.outputAccum.begin());
    std::fill (cs.outputAccum.end() - kHopSize, cs.outputAccum.end(), 0.0f);

    cs.outputQueuePos = 0;
}

//==============================================================================
void SpectralEngine::process (juce::AudioBuffer<float>& buffer)
{
    const int numCh      = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    // --- Donor capture ---
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

    // --- Overlap-add synthesis (per sample) ---
    const float dw = dryWet;

    for (int i = 0; i < numSamples; ++i)
    {
        // Push new samples into channel ring buffers
        for (int c = 0; c < numCh; ++c)
        {
            ch[c].inputRing[ch[c].inputWritePos] = buffer.getSample (c, i);
            ch[c].inputWritePos = (ch[c].inputWritePos + 1) & (kFFTSize - 1);
        }

        // Trigger FFT frame every kHopSize samples
        if (++hopCount >= kHopSize)
        {
            hopCount = 0;
            for (int c = 0; c < numCh; ++c)
                processFFTFrame (c);

            // Advance donor read head and re-analyse for the next frame
            if (hasDonor && donorLength > 0)
            {
                donorReadPos = (donorReadPos + kHopSize) % donorLength;
                analyseDonorFrame();
            }
        }

        // Mix dry + wet
        for (int c = 0; c < numCh; ++c)
        {
            const float dry = buffer.getSample (c, i);
            const float wet = (ch[c].outputQueuePos < kHopSize)
                                  ? ch[c].outputQueue[ch[c].outputQueuePos++]
                                  : 0.0f;
            buffer.setSample (c, i, dry * (1.0f - dw) + wet * dw);
        }
    }
}
