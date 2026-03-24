#include "SpectralEngine.h"
#include <cmath>

SpectralEngine::SpectralEngine()
    : fft    (kFFTOrder),
      window ((size_t) kFFTSize, juce::dsp::WindowingFunction<float>::hann, false)
{
    fftScratch.resize     (2 * kFFTSize, 0.0f);
    inputMag.resize       (kNumBins, 0.0f);
    inputPhase.resize     (kNumBins, 0.0f);
    liveEnvelope.resize   (kNumBins, 0.0f);
    donorMag.resize       (kNumBins, 0.0f);
    donorEnvelope.resize  (kNumBins, 0.0f);
    blendedMag.resize     (kNumBins, 0.0f);
    envelopePrefix.resize (kNumBins + 1, 0.0f);
    donorPhasePrev.resize  (kNumBins, 0.0f);
    donorTrueFreq.resize   (kNumBins, 0.0f);
    donorPhaseAccum.resize (kNumBins, 0.0f);

    donorBuffer.setSize (2, kMaxDonorSamples);
    donorBuffer.clear();
    for (auto& slot : donorSlots)
    {
        slot.buf.setSize (2, kMaxDonorSamples);
        slot.buf.clear();
    }

    ch[0].init();
    ch[1].init();
}

void SpectralEngine::prepare (double sampleRate, int /*blockSize*/)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = (juce::uint32) 4096;
    spec.numChannels      = 2;
    limiter.prepare (spec);
    limiter.setThreshold (-1.0f);  // -1 dBFS ceiling
    limiter.setRelease   (50.0f);  // 50 ms release

    engageGain.reset (sampleRate, 0.015);
    engageGain.setCurrentAndTargetValue (0.0f);
    phraseEngageGain.reset (sampleRate, 0.015);
    phraseEngageGain.setCurrentAndTargetValue (0.0f);

    constexpr double kSmoothSec = 0.02; // 20 ms ramp
    morphSmoothed.reset   (sampleRate, kSmoothSec);  morphSmoothed.setCurrentAndTargetValue   (0.5f);
    dryWetSmoothed.reset  (sampleRate, kSmoothSec);  dryWetSmoothed.setCurrentAndTargetValue  (0.8f);
    grainSmoothed.reset   (sampleRate, kSmoothSec);  grainSmoothed.setCurrentAndTargetValue   (0.0f);
    formantSmoothed.reset (sampleRate, kSmoothSec);  formantSmoothed.setCurrentAndTargetValue (0.5f);
    scatterSmoothed.reset (sampleRate, kSmoothSec);  scatterSmoothed.setCurrentAndTargetValue (0.3f);
    pitchSmoothed.reset   (sampleRate, kSmoothSec);  pitchSmoothed.setCurrentAndTargetValue   (0.0f);
    donorBuffer.clear();
    donorWritePos  = 0;
    donorLength    = 0;
    donorLengthAtomic.store (0, std::memory_order_relaxed);
    donorReadPos   = 0;
    donorRecording.store (false, std::memory_order_relaxed);
    hasDonor       = false;
    hopCount       = 0;
    grainSpawnCounter = 0;

    ch[0].init();
    ch[1].init();

    for (auto& gr : grains) gr.active = false;

    std::fill (donorMag.begin(),        donorMag.end(),        0.0f);
    std::fill (donorEnvelope.begin(),   donorEnvelope.end(),   0.0f);
    std::fill (donorPhasePrev.begin(),  donorPhasePrev.end(),  0.0f);
    std::fill (donorTrueFreq.begin(),   donorTrueFreq.end(),   0.0f);
    std::fill (donorPhaseAccum.begin(), donorPhaseAccum.end(), 0.0f);
    donorFrozen   = false;
    phraseEngaged = false;

    storedSampleRate = sampleRate;
    for (auto& v : midiVoices) { v.reset(); v.adsr.setSampleRate (sampleRate); }
    effectAdsr_.setSampleRate (sampleRate);
    effectAdsr_.reset();
    effectAdsrOn     = false;
    midiMode         = false;
    scatterOldPosF   = 0.0f;
    scatterFadeCount = 0;
}

//==============================================================================
void SpectralEngine::startRecording()
{
    if (donorRecording.load (std::memory_order_relaxed)) return;
    donorBuffer.clear();
    donorWritePos  = 0;
    donorLength    = 0;
    donorLengthAtomic.store (0, std::memory_order_relaxed);
    donorReadPos   = 0;
    hasDonor       = false;
    donorRecording.store (true, std::memory_order_relaxed);
}

void SpectralEngine::stopRecording()
{
    if (!donorRecording.load (std::memory_order_relaxed)) return;
    donorRecording.store (false, std::memory_order_relaxed);
    hasDonor       = donorLength > 0;
    donorReadPos   = 0;
    phraseReadPosF = 0.0f;   // always rewind phrase to start of new recording

    // Force-disengage so the upcoming setEngage(true) call from auto-engage
    // actually runs (not skipped by the donorFrozen == v early-return guard).
    // Gains are zeroed instantly — recording was already gating all output anyway.
    donorFrozen   = false;
    phraseEngaged = false;
    engageGain.setCurrentAndTargetValue       (0.0f);
    phraseEngageGain.setCurrentAndTargetValue (0.0f);

    if (hasDonor)
    {
        // Analyse from near the end of the recording — the most recently captured audio,
        // not silence at position 0 which may precede the actual content.
        donorReadPos = juce::jmax (0, donorLength - kFFTSize * 2);
        analyseDonorFrame();
        updateVisSnapshot();       // show captured spectrum immediately, before ENGAGE
        updateWaveformSnapshot();  // show captured waveform immediately
        autoEngagePending.store (true);

        // Snapshot working buffer into the active slot (pre-allocated, no heap use)
        auto& slot = donorSlots[activeSlot];
        const int numCh = donorBuffer.getNumChannels();
        for (int c = 0; c < numCh; ++c)
            slot.buf.copyFrom (c, 0, donorBuffer, c, 0, donorLength);
        slot.length  = donorLength;
        slot.hasData = true;
    }
}

//==============================================================================
void SpectralEngine::snapSmoothers (float morph_, float grain_, float formant_,
                                     float scatter_, float drywet_, float pitch_) noexcept
{
    morphSmoothed.setCurrentAndTargetValue   (morph_);
    grainSmoothed.setCurrentAndTargetValue   (grain_);
    formantSmoothed.setCurrentAndTargetValue (formant_);
    scatterSmoothed.setCurrentAndTargetValue (scatter_);
    dryWetSmoothed.setCurrentAndTargetValue  (drywet_);
    pitchSmoothed.setCurrentAndTargetValue   (pitch_);
}

//==============================================================================
void SpectralEngine::setMidiMode (bool v) noexcept
{
    if (midiMode == v) return;
    midiMode = v;
    if (v)
    {
        // Entering MIDI mode: engageGain stays at 1 — ADSR per voice controls amplitude
        engageGain.setCurrentAndTargetValue (1.0f);
    }
    else
    {
        // Leaving MIDI mode: silence all voices, disengage
        for (auto& voice : midiVoices) voice.reset();
        phraseEngaged = false;
        phraseEngageGain.setCurrentAndTargetValue (0.0f);
        if (donorFrozen)
        {
            donorFrozen = false;
            engageGain.setCurrentAndTargetValue (0.0f);
        }
    }
}

void SpectralEngine::setAdsrParams (float a, float d, float s, float r) noexcept
{
    adsrParams.attack  = a;
    adsrParams.decay   = d;
    adsrParams.sustain = s;
    adsrParams.release = r;
}

void SpectralEngine::triggerMidiNoteOn (int noteNum, float semitones, float velocity) noexcept
{
    if (!hasDonor) return;

    // Find a free (inactive) voice, or steal the first releasing one
    MidiVoice* target = nullptr;
    for (auto& v : midiVoices)
        if (!v.active && !v.adsr.isActive()) { target = &v; break; }
    if (!target)  // all busy — steal the first active voice
        target = &midiVoices[0];

    // Freeze the spectral snapshot on the first note if not already frozen
    if (!donorFrozen)
    {
        donorFrozen    = true;
        phraseReadPosF = (float) donorReadPos;
        for (int k = 0; k < kNumBins; ++k)
            donorPhaseAccum[k] = donorPhasePrev[k];
    }

    target->noteNumber = noteNum;
    target->pitchRate  = std::pow (2.0f, semitones / 12.0f);
    target->phrasePos  = (float) donorReadPos;
    target->velocity   = juce::jlimit (0.0f, 1.0f, velocity);
    target->active     = true;
    target->adsr.setSampleRate (storedSampleRate);
    target->adsr.setParameters (adsrParams);
    target->adsr.reset();
    target->adsr.noteOn();
}

void SpectralEngine::triggerMidiNoteOff (int noteNum) noexcept
{
    for (auto& v : midiVoices)
    {
        if (v.noteNumber == noteNum && v.active)
        {
            v.adsr.noteOff();
            v.noteNumber = -1;   // allow same note to re-trigger cleanly
            break;
        }
    }
}

void SpectralEngine::setEngage (bool v) noexcept
{
    if (donorFrozen == v) return;
    donorFrozen = v;
    engageGain.setTargetValue (v ? 1.0f : 0.0f);
    if (v) phraseReadPosF = (float) donorReadPos;  // freeze at the current analysis position

    // Effect ADSR: trigger note on/off so the envelope shapes the freeze onset/release
    if (effectAdsrEnabled && !midiMode)
    {
        effectAdsr_.setParameters (adsrParams);
        if (v && !effectAdsrOn) { effectAdsr_.noteOn();  effectAdsrOn = true;  }
        if (!v && effectAdsrOn) { effectAdsr_.noteOff(); effectAdsrOn = false; }
    }
    // OLA flush is deferred until the fade-out ramp completes in process()
}

void SpectralEngine::setPhraseEngage (bool v) noexcept
{
    if (phraseEngaged == v) return;
    phraseEngaged = v;
    phraseEngageGain.setTargetValue (v ? 1.0f : 0.0f);
    if (v)
    {
        phraseReadPosF   = (float) donorReadPos;  // start loop from current analysis position
        scatterFadeCount = 0;                     // clear any leftover crossfade
    }
    else
    {
        scatterFadeCount = 0;

        // Clear phrase formant OLA state so stale audio doesn't bleed into the next engage
        for (auto& cs : ch)
        {
            std::fill (cs.phraseFormantRing.begin(),  cs.phraseFormantRing.end(),  0.0f);
            std::fill (cs.phraseFormantAccum.begin(), cs.phraseFormantAccum.end(), 0.0f);
            std::fill (cs.phraseFormantQueue.begin(), cs.phraseFormantQueue.end(), 0.0f);
            cs.phraseFormantWritePos = 0;
            cs.phraseFormantQueuePos = kHopSize;
        }
    }
}

float SpectralEngine::getDonorFillLevel() const
{
    return (float) donorLength / (float) kMaxDonorSamples;
}

void SpectralEngine::setDonorData (const juce::AudioBuffer<float>& buf, int length)
{
    const int safeLen = juce::jmin (length, kMaxDonorSamples);
    const int numCh   = juce::jmin (buf.getNumChannels(), 2);

    donorBuffer.clear();
    for (int c = 0; c < numCh; ++c)
        donorBuffer.copyFrom (c, 0, buf, c, 0, safeLen);

    donorLength    = safeLen;
    donorLengthAtomic.store (safeLen, std::memory_order_relaxed);
    donorWritePos  = safeLen;
    donorReadPos   = 0;
    donorRecording.store (false, std::memory_order_relaxed);
    hasDonor       = safeLen >= kFFTSize;

    if (hasDonor)
    {
        analyseDonorFrame();
        updateWaveformSnapshot();
    }
}

void SpectralEngine::importDonorData (const juce::AudioBuffer<float>& buf, int length)
{
    setDonorData (buf, length);   // load into working buffer + run analysis

    // Persist into the active slot so it survives slot switches and preset saves
    auto& slot = donorSlots[activeSlot];
    const int numCh = donorBuffer.getNumChannels();
    for (int c = 0; c < numCh; ++c)
        slot.buf.copyFrom (c, 0, donorBuffer, c, 0, donorLength);
    slot.length  = donorLength;
    slot.hasData = donorLength >= kFFTSize;
}

//==============================================================================
void SpectralEngine::setActiveSlot (int n) noexcept
{
    if (n < 0 || n >= kNumDonorSlots || n == activeSlot) return;
    activeSlot = n;

    const auto& slot = donorSlots[n];
    if (slot.hasData)
    {
        // Load slot into working buffer — uses pre-allocated storage, no heap
        setDonorData (slot.buf, slot.length);
    }
    else
    {
        // Empty slot: clear working state
        donorBuffer.clear();
        donorLength    = 0;
        donorLengthAtomic.store (0, std::memory_order_relaxed);
        donorWritePos  = 0;
        donorReadPos   = 0;
        hasDonor       = false;
    }
}

//==============================================================================
// Spectrum visualiser helpers
//==============================================================================
void SpectralEngine::updateVisSnapshot() noexcept
{
    if (!visLock.tryEnter()) return;

    // Map kVisBins display bins to kNumBins FFT bins (skip DC at 0, skip Nyquist at end).
    // Peak-hold within each display bin so thin peaks aren't lost.
    const float norm = 1.0f / (float) kFFTSize;
    for (int i = 0; i < kVisBins; ++i)
    {
        const int b0 = 1 + i       * (kNumBins - 2) / kVisBins;
        const int b1 = 1 + (i + 1) * (kNumBins - 2) / kVisBins;
        float mi = 0.0f, md = 0.0f;
        for (int b = b0; b <= b1 && b < kNumBins; ++b)
        {
            mi = std::max (mi, inputMag[b]);
            md = std::max (md, donorMag[b]);
        }
        visPending.inputMag[(size_t) i] = mi * norm;
        visPending.donorMag[(size_t) i] = md * norm;
    }
    visPending.hasData = true;
    visLock.exit();
}

void SpectralEngine::updateTunerResult() noexcept
{
    if (!tunerLock.tryEnter()) return;

    // Search up to ~2000 Hz (MIDI note ~96) to avoid chasing harmonics above musical range
    const int maxBin = (int) (2000.0 * kFFTSize / storedSampleRate) + 1;
    const int clampedMax = juce::jmin (maxBin, kNumBins / 2);

    int   peakBin = 1;
    float peakMag = 0.0f;
    for (int k = 2; k < clampedMax; ++k)
    {
        if (inputMag[k] > peakMag)
        {
            peakMag = inputMag[k];
            peakBin = k;
        }
    }

    if (peakMag < 1e-5f)   // no significant signal
    {
        tunerPending.hasData = false;
        tunerLock.exit();
        return;
    }

    // Quadratic interpolation for sub-bin frequency refinement
    float refinedBin = (float) peakBin;
    if (peakBin > 1 && peakBin < kNumBins - 1)
    {
        const float a = inputMag[peakBin - 1];
        const float b = inputMag[peakBin];
        const float c = inputMag[peakBin + 1];
        const float denom = a - 2.0f * b + c;
        if (std::abs (denom) > 1e-10f)
            refinedBin += 0.5f * (a - c) / denom;
    }

    const float freqHz = refinedBin * (float) storedSampleRate / (float) kFFTSize;

    if (freqHz > 20.0f)
    {
        const float midiF  = 69.0f + 12.0f * std::log2 (freqHz / 440.0f);
        const int   note   = juce::jlimit (0, 127, (int) std::round (midiF));
        tunerPending.frequencyHz = freqHz;
        tunerPending.midiNote    = note;
        tunerPending.centsOffset = (midiF - (float) note) * 100.0f;
        tunerPending.hasData     = true;
    }

    tunerLock.exit();
}

bool SpectralEngine::getTunerResult (TunerResult& out) const
{
    juce::ScopedLock sl (tunerLock);
    if (!tunerPending.hasData) return false;
    out = tunerPending;
    return true;
}

bool SpectralEngine::getSpectrumSnapshot (SpectrumSnapshot& out) const
{
    juce::ScopedLock sl (visLock);
    if (!visPending.hasData) return false;
    out = visPending;
    return true;   // deliberately does NOT clear hasData — display keeps last frame when idle
}

//==============================================================================
// Waveform overview helpers
//==============================================================================
void SpectralEngine::updateWaveformSnapshot() noexcept
{
    if (!hasDonor || donorLength == 0) return;
    if (!waveformLock.tryEnter()) return;

    const int    len = donorLength;
    const float* src = donorBuffer.getReadPointer (0);

    float maxPeak = 1e-6f;
    for (int i = 0; i < kWavePoints; ++i)
    {
        const int start = i * len / kWavePoints;
        const int end   = juce::jmin ((i + 1) * len / kWavePoints, len);
        float pk = 0.0f;
        for (int s = start; s < end; ++s)
            pk = std::max (pk, std::abs (src[s]));
        waveformPending.peaks[(size_t) i] = pk;
        maxPeak = std::max (maxPeak, pk);
    }

    const float invMax = 1.0f / maxPeak;
    for (auto& p : waveformPending.peaks)
        p *= invMax;

    waveformPending.hasData = true;
    waveformLock.exit();
}

bool SpectralEngine::getWaveformSnapshot (WaveformSnapshot& out) const
{
    juce::ScopedLock sl (waveformLock);
    if (!waveformPending.hasData) return false;
    out          = waveformPending;
    out.playhead = waveformPlayheadAtomic.load (std::memory_order_relaxed);
    return true;
}

//==============================================================================
// Spectral envelope via prefix-sum box filter (O(N) per call).
// envelopePrefix is a pre-allocated member — no heap/stack allocation on the RT thread.
void SpectralEngine::computeEnvelope (const float* mag, float* env, int numBins, int halfWindow) const
{
    envelopePrefix[0] = 0.0f;
    for (int k = 0; k < numBins; ++k)
        envelopePrefix[k + 1] = envelopePrefix[k] + mag[k];

    for (int k = 0; k < numBins; ++k)
    {
        const int lo = std::max (0, k - halfWindow);
        const int hi = std::min (numBins - 1, k + halfWindow);
        env[k] = (envelopePrefix[hi + 1] - envelopePrefix[lo]) / (float) (hi - lo + 1);
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

    const float twoPi          = juce::MathConstants<float>::twoPi;
    const float hopOverFFT     = (float) kHopSize / (float) kFFTSize;

    for (int k = 0; k < kNumBins; ++k)
    {
        const float re = fftScratch[2 * k];
        const float im = fftScratch[2 * k + 1];
        donorMag[k] = std::sqrt (re * re + im * im);

        // True frequency estimation (phase vocoder)
        const float phase          = std::atan2 (im, re);
        const float expectedAdv    = twoPi * (float) k * hopOverFFT;
        float       delta          = phase - donorPhasePrev[k] - expectedAdv;
        delta -= twoPi * std::round (delta / twoPi);          // wrap to [-pi, pi]
        donorTrueFreq[k]           = expectedAdv + delta;     // radians per hop
        donorPhasePrev[k]          = phase;
        donorPhaseAccum[k]         = phase;                   // resync to current frame
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

    // Push spectrum + tuner snapshots to the UI thread (ch 0 only, non-blocking)
    if (chIdx == 0)
    {
        updateVisSnapshot();
        updateTunerResult();
    }

    // Compute live spectral envelope (only when formant transfer is active)
    const float fm = formant;
    if (hasDonor && fm > 0.0f)
        computeEnvelope (inputMag.data(), liveEnvelope.data(), kNumBins, 50);

    // ── Pass 1: SIMD magnitude lerp ────────────────────────────────────────────
    const float m = morph;
    if (hasDonor)
    {
        // blendedMag = inputMag * (1-m) + donorMag * m
        juce::FloatVectorOperations::copy            (blendedMag.data(), inputMag.data(), kNumBins);
        juce::FloatVectorOperations::multiply        (blendedMag.data(), 1.0f - m,        kNumBins);
        juce::FloatVectorOperations::addWithMultiply (blendedMag.data(), donorMag.data(), m, kNumBins);
    }
    else
    {
        juce::FloatVectorOperations::copy (blendedMag.data(), inputMag.data(), kNumBins);
    }

    // ── Pass 2: formant envelope transfer (energy-normalised) ─────────────────
    // Renormalise after shaping so formant only changes spectral shape, not volume.
    if (hasDonor && fm > 0.0f)
    {
        float energyBefore = 0.0f, energyAfter = 0.0f;
        for (int k = 0; k < kNumBins; ++k)
        {
            energyBefore += blendedMag[k] * blendedMag[k];
            const float liveEnv = std::max (liveEnvelope[k], 1e-6f);
            const float ratio   = juce::jlimit (0.1f, 10.0f, donorEnvelope[k] / liveEnv);
            blendedMag[k] *= juce::jmax (0.0f, 1.0f + fm * (ratio - 1.0f));
            energyAfter += blendedMag[k] * blendedMag[k];
        }
        if (energyAfter > 1e-12f)
            juce::FloatVectorOperations::multiply (blendedMag.data(),
                                                   std::sqrt (energyBefore / energyAfter),
                                                   kNumBins);
    }

    // ── Pass 3: polar-to-cartesian reconstruction ──────────────────────────────
    const bool frozen = donorFrozen && hasDonor;
    for (int k = 0; k < kNumBins; ++k)
    {
        const float phase         = frozen ? donorPhaseAccum[k] : inputPhase[k];
        fftScratch[2 * k]         = blendedMag[k] * std::cos (phase);
        fftScratch[2 * k + 1]     = blendedMag[k] * std::sin (phase);
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

    // SIMD accumulate: outputAccum += fftScratch * scale
    juce::FloatVectorOperations::addWithMultiply (cs.outputAccum.data(), fftScratch.data(), scale, kFFTSize);

    // Deliver kHopSize ready samples, shift accumulator
    std::copy (cs.outputAccum.begin(), cs.outputAccum.begin() + kHopSize, cs.outputQueue.begin());
    std::copy (cs.outputAccum.begin() + kHopSize, cs.outputAccum.end(), cs.outputAccum.begin());
    juce::FloatVectorOperations::clear (cs.outputAccum.data() + kFFTSize - kHopSize, kHopSize);

    cs.outputQueuePos = 0;
}

//==============================================================================
// Cross-synthesis: shapes the phrase playback stream toward the live input's spectral envelope.
// Called once per channel at each hop boundary when phraseEngaged && formant > 0.
// liveEnvelope (set by processFFTFrame) is the cross-synthesis target; blendedMag is used as
// scratch for the phrase envelope.  Safe: runs after all processFFTFrame calls for the hop.
void SpectralEngine::processPhraseFftFrame (int chIdx)
{
    auto& cs = ch[chIdx];

    // Build analysis frame from phrase ring buffer (oldest sample first)
    for (int i = 0; i < kFFTSize; ++i)
        fftScratch[i] = cs.phraseFormantRing[(cs.phraseFormantWritePos + i) & (kFFTSize - 1)];

    std::fill (fftScratch.begin() + kFFTSize, fftScratch.end(), 0.0f);
    window.multiplyWithWindowingTable (fftScratch.data(), (size_t) kFFTSize);
    fft.performRealOnlyForwardTransform (fftScratch.data(), false);

    // Extract phrase magnitudes and phases into shared scratch (inputMag / inputPhase)
    for (int k = 0; k < kNumBins; ++k)
    {
        const float re = fftScratch[2 * k];
        const float im = fftScratch[2 * k + 1];
        inputMag[k]   = std::sqrt (re * re + im * im);
        inputPhase[k] = std::atan2 (im, re);
    }

    // Compute phrase spectral envelope into blendedMag (spare scratch not used in this path).
    computeEnvelope (inputMag.data(), blendedMag.data(), kNumBins, 50);

    // Formant correction: push the pitch-shifted phrase spectrum back toward the original donor
    // formant envelope.  When pitch = 0 the phrase and donor envelopes are nearly equal so the
    // ratio is ~1 and the effect is subtle.  When pitch != 0 the phrase ring carries the
    // frequency-shifted version of the donor audio; donorEnvelope / phraseEnv pulls the spectral
    // tilt back toward the original timbral character, preserving perceived vowel / timbre during
    // pitch shifting.  Energy normalisation keeps output level constant regardless of ratio.
    float energyBefore = 0.0f, energyAfter = 0.0f;
    for (int k = 0; k < kNumBins; ++k)
        energyBefore += inputMag[k] * inputMag[k];

    // If the ring has no real content yet (warmup), skip shaping to avoid output being silent.
    if (energyBefore < 1e-8f)
    {
        cs.phraseFormantQueuePos = kHopSize;  // keep fallback to rawPhrase
        return;
    }

    for (int k = 0; k < kNumBins; ++k)
    {
        const float phraseEnv = std::max (blendedMag[k], 1e-6f);
        const float ratio     = juce::jlimit (0.1f, 10.0f, donorEnvelope[k] / phraseEnv);
        const float shaped    = inputMag[k] * juce::jmax (0.0f, 1.0f + formant * (ratio - 1.0f));
        energyAfter += shaped * shaped;

        fftScratch[2 * k]     = shaped * std::cos (inputPhase[k]);
        fftScratch[2 * k + 1] = shaped * std::sin (inputPhase[k]);
    }

    if (energyAfter > 1e-12f)
    {
        const float gain = std::sqrt (energyBefore / energyAfter);
        for (int k = 0; k < kNumBins; ++k)
        {
            fftScratch[2 * k]     *= gain;
            fftScratch[2 * k + 1] *= gain;
        }
    }

    // Mirror conjugate symmetry for real IFFT
    for (int k = 1; k < kFFTSize / 2; ++k)
    {
        fftScratch[2 * (kFFTSize - k)]     =  fftScratch[2 * k];
        fftScratch[2 * (kFFTSize - k) + 1] = -fftScratch[2 * k + 1];
    }

    // IFFT + normalize, OLA accumulate
    fft.performRealOnlyInverseTransform (fftScratch.data());
    const float scale = 1.0f / (2.0f * (float) kFFTSize);

    for (int i = 0; i < kFFTSize; ++i)
        cs.phraseFormantAccum[i] += fftScratch[i] * scale;

    // Deliver kHopSize ready samples, shift accumulator
    std::copy (cs.phraseFormantAccum.begin(), cs.phraseFormantAccum.begin() + kHopSize, cs.phraseFormantQueue.begin());
    std::copy (cs.phraseFormantAccum.begin() + kHopSize, cs.phraseFormantAccum.end(), cs.phraseFormantAccum.begin());
    std::fill (cs.phraseFormantAccum.end() - kHopSize, cs.phraseFormantAccum.end(), 0.0f);

    cs.phraseFormantQueuePos = 0;
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
        // When PHRASE is engaged, scatter grains around the audible playhead position;
        // otherwise use the spectral analysis cursor (donorReadPos).
        const int center = phraseEngaged ? (int) phraseReadPosF : donorReadPos;
        gr.readPos = (center + offset + donorLength) % donorLength;
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

        float grainOut[2] = { 0.0f, 0.0f };
        for (auto& gr : grains)
        {
            if (!gr.active) continue;

            const float t   = (float) gr.age / (float) (gr.size - 1);
            const float env = 0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi * t));
            const int   pos = gr.readPos % donorLength;
            for (int c = 0; c < numCh; ++c)
                grainOut[c] += donorBuffer.getSample (c, pos) * env;

            if (++gr.readPos, ++gr.age >= gr.size)
                gr.active = false;
        }

        for (int c = 0; c < numCh; ++c)
            buffer.addSample (c, i, grainOut[c] * g * 0.2f);
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
        const int limit   = juce::jmin (recordLimitSamples, kMaxDonorSamples);
        const int toWrite = juce::jmin (numSamples, limit - donorWritePos);
        for (int c = 0; c < numCh; ++c)
            for (int i = 0; i < toWrite; ++i)
                donorBuffer.setSample (c, donorWritePos + i, buffer.getSample (c, i));
        donorWritePos += toWrite;
        donorLength    = donorWritePos;
        donorLengthAtomic.store (donorLength, std::memory_order_relaxed);
        if (donorWritePos >= limit)
            stopRecording();
    }

    // Advance block-level smoothers and snapshot working values
    morphSmoothed.skip   (numSamples);  morph   = morphSmoothed.getCurrentValue();
    grainSmoothed.skip   (numSamples);  grain   = grainSmoothed.getCurrentValue();
    formantSmoothed.skip (numSamples);  formant = formantSmoothed.getCurrentValue();
    scatterSmoothed.skip (numSamples);  scatter = scatterSmoothed.getCurrentValue();
    pitchSmoothed.skip   (numSamples);  pitch   = pitchSmoothed.getCurrentValue();
    // dryWet is advanced per-sample below for click-free fades

    // Gate: process while any voice is active (MIDI) or while engaged/fading (effect).
    auto anyVoiceActive = [this]() noexcept {
        for (auto& v : midiVoices) if (v.adsr.isActive()) return true;
        return false;
    };
    const bool isFreezeActive = donorFrozen
                                || engageGain.isSmoothing()
                                || engageGain.getCurrentValue() > 0.001f;
    const bool isPhraseActive = phraseEngaged
                                || phraseEngageGain.isSmoothing()
                                || phraseEngageGain.getCurrentValue() > 0.001f;
    const bool shouldProcess  = (midiMode ? anyVoiceActive() : (isFreezeActive || isPhraseActive))
                                && hasDonor && !donorRecording;

    if (!shouldProcess)
    {
        engageGain.skip      (numSamples); // keep smoothers in sync
        phraseEngageGain.skip (numSamples);
        dryWetSmoothed.skip  (numSamples);
        // Keep inputRing current so engage starts with up-to-date audio
        for (int i = 0; i < numSamples; ++i)
        {
            for (int c = 0; c < numCh; ++c)
            {
                ch[c].inputRing[ch[c].inputWritePos] = buffer.getSample (c, i);
                ch[c].inputWritePos = (ch[c].inputWritePos + 1) & (kFFTSize - 1);
            }
            if (++hopCount >= kHopSize)
                hopCount = 0;
        }
        // Grains run independently when a donor is loaded — no FFT needed
        if (grain > 0.0f && hasDonor)
        {
            processGrains (buffer, numCh, numSamples);
            juce::dsp::AudioBlock<float>             block (buffer);
            juce::dsp::ProcessContextReplacing<float> ctx (block);
            limiter.process (ctx);
        }
        return; // dry passthrough (+ optional grain texture)
    }

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

            // Advance phase accumulators (frozen mode only)
            for (int k = 0; k < kNumBins; ++k)
                donorPhaseAccum[k] += donorTrueFreq[k];

            // Scrub donor read head through the recording when not frozen.
            // This keeps the spectral snapshot current so FREEZE captures
            // the audio at the moment of engage, not always sample 0.
            if (hasDonor && !donorFrozen)
            {
                donorReadPos = (donorReadPos + kHopSize) % donorLength;
                analyseDonorFrame();
            }

            // Scatter: jitter the phrase playhead at each hop boundary.
            // Mirrors grain scatter range (±25% of donor length at max scatter).
            if (phraseEngaged && hasDonor && donorLength > 0 && scatter > 0.0f)
            {
                const int scatterRange = (int) (scatter * (float) donorLength * 0.25f);
                if (scatterRange > 0)
                {
                    // Save old position and start a kHopSize-sample crossfade to prevent clicks
                    scatterOldPosF   = phraseReadPosF;
                    scatterFadeCount = kHopSize;
                    phraseReadPosF  += (rng.nextFloat() * 2.0f - 1.0f) * (float) scatterRange;
                    if (phraseReadPosF < 0.0f)                  phraseReadPosF += (float) donorLength;
                    if (phraseReadPosF >= (float) donorLength)   phraseReadPosF -= (float) donorLength;
                }
            }

            // Phrase formant: run phrase ring buffer through donor spectral envelope shaping.
            // Called after scatter so the ring reflects the audio that was just output.
            if (phraseEngaged && hasDonor && formant > 0.0f)
                for (int c = 0; c < numCh; ++c)
                    processPhraseFftFrame (c);

            // Update playhead for waveform display
            if (hasDonor && donorLength > 0)
                waveformPlayheadAtomic.store (phraseReadPosF / (float) donorLength,
                                              std::memory_order_relaxed);
        }

        const float dw = dryWetSmoothed.getNextValue();

        if (midiMode)
        {
            // ── Polyphonic MIDI mode ──────────────────────────────────────
            // Advance each voice ADSR once per sample; collect envelope values.
            float voiceEnv[kMaxVoices] = {};
            float totalEnv = 0.0f;
            for (int vi = 0; vi < kMaxVoices; ++vi)
            {
                auto& v = midiVoices[vi];
                if (v.active || v.adsr.isActive())
                {
                    voiceEnv[vi] = v.adsr.getNextSample() * v.velocity;
                    if (!v.adsr.isActive()) v.active = false;
                    totalEnv = std::max (totalEnv, voiceEnv[vi]);
                }
            }

            // Per-channel mix: spectral (shared) + sum of per-voice phrase loops
            for (int c = 0; c < numCh; ++c)
            {
                const float dry        = buffer.getSample (c, i);
                const float spectralWet = (ch[c].outputQueuePos < kHopSize)
                                              ? ch[c].outputQueue[ch[c].outputQueuePos++]
                                              : 0.0f;
                float voicePhrase = 0.0f;
                for (int vi = 0; vi < kMaxVoices; ++vi)
                {
                    if (voiceEnv[vi] == 0.0f) continue;
                    auto& v = midiVoices[vi];
                    const int   p0   = (int) v.phrasePos % donorLength;
                    const int   p1   = (p0 + 1) % donorLength;
                    const float frac = v.phrasePos - std::floor (v.phrasePos);
                    voicePhrase += (donorBuffer.getSample (c, p0) * (1.0f - frac)
                                  + donorBuffer.getSample (c, p1) * frac) * voiceEnv[vi];
                }
                const float wet = spectralWet * morph + voicePhrase * (1.0f - morph);
                buffer.setSample (c, i, dry * (1.0f - dw * totalEnv) + wet * dw * totalEnv);
            }

            // Advance each voice phrase playhead (reverse-aware, per-voice pitch)
            for (int vi = 0; vi < kMaxVoices; ++vi)
            {
                if (voiceEnv[vi] == 0.0f) continue;
                auto& v = midiVoices[vi];
                v.phrasePos += reverse ? -v.pitchRate : v.pitchRate;
                if (v.phrasePos < 0.0f)                   v.phrasePos += (float) donorLength;
                if (v.phrasePos >= (float) donorLength)    v.phrasePos -= (float) donorLength;
            }
        }
        else
        {
            // ── Effect mode: FREEZE and PHRASE are independent ────────────
            const float adsrMult   = (effectAdsrEnabled && !midiMode)
                                         ? effectAdsr_.getNextSample() : 1.0f;
            const float freezeGain = engageGain.getNextValue() * adsrMult;
            const float phraseGain = phraseEngageGain.getNextValue();
            const float masterEnv  = std::max (freezeGain, phraseGain);
            const float rate   = std::pow (2.0f, pitch / 12.0f);
            const int   pos0   = (int) phraseReadPosF % donorLength;
            const int   pos1   = (pos0 + 1) % donorLength;
            const float frac   = phraseReadPosF - std::floor (phraseReadPosF);

            // Crossfade weight: blends old playhead (pre-jump/wrap) into current position
            // to eliminate the click from scatter jumps and loop-wrap discontinuities.
            const float xfadeT  = scatterFadeCount > 0
                                   ? (float) scatterFadeCount / (float) kHopSize : 0.0f;
            const int   oldPos0 = scatterFadeCount > 0 ? (int) scatterOldPosF % donorLength : 0;
            const int   oldPos1 = scatterFadeCount > 0 ? (oldPos0 + 1) % donorLength : 0;
            const float oldFrac = scatterFadeCount > 0
                                   ? scatterOldPosF - std::floor (scatterOldPosF) : 0.0f;

            // Morph blends freeze <-> phrase only when BOTH are active.
            // If only one is engaged, its weight is 1.0 regardless of morph position.
            const bool hasFreezeAudio = freezeGain > 0.001f;
            const bool hasPhraseAudio = phraseGain > 0.001f;
            const float freezeWeight  = (hasFreezeAudio && hasPhraseAudio) ? morph          : (hasFreezeAudio ? 1.0f : 0.0f);
            const float phraseWeight  = (hasFreezeAudio && hasPhraseAudio) ? (1.0f - morph) : (hasPhraseAudio ? 1.0f : 0.0f);

            for (int c = 0; c < numCh; ++c)
            {
                const float dry         = buffer.getSample (c, i);
                const float spectralWet = (ch[c].outputQueuePos < kHopSize)
                                              ? ch[c].outputQueue[ch[c].outputQueuePos++]
                                              : 0.0f;

                float rawPhrase = donorBuffer.getSample (c, pos0) * (1.0f - frac)
                                + donorBuffer.getSample (c, pos1) * frac;

                // Blend old playhead during crossfade (scatter jump or loop wrap)
                if (xfadeT > 0.0f)
                {
                    const float oldSample = donorBuffer.getSample (c, oldPos0) * (1.0f - oldFrac)
                                          + donorBuffer.getSample (c, oldPos1) * oldFrac;
                    rawPhrase = oldSample * xfadeT + rawPhrase * (1.0f - xfadeT);
                }

                // Feed raw phrase into formant ring buffer for processing at the next hop
                if (formant > 0.0f && phraseEngaged)
                {
                    ch[c].phraseFormantRing[ch[c].phraseFormantWritePos] = rawPhrase;
                    ch[c].phraseFormantWritePos = (ch[c].phraseFormantWritePos + 1) & (kFFTSize - 1);
                }

                // Blend raw phrase with formant-shaped output (falls back to raw during warmup).
                // Constant-power crossfade prevents energy drop when the two paths are decorrelated.
                float shapedPhrase = rawPhrase;
                if (formant > 0.0f && ch[c].phraseFormantQueuePos < kHopSize)
                    shapedPhrase = ch[c].phraseFormantQueue[ch[c].phraseFormantQueuePos++];
                const float fmSqrt  = std::sqrt (formant);
                const float ifmSqrt = std::sqrt (1.0f - formant);
                const float phraseOut = rawPhrase * ifmSqrt + shapedPhrase * fmSqrt;

                const float wet = spectralWet * freezeGain * freezeWeight
                                + phraseOut   * phraseGain * phraseWeight;

                // Compensate for energy loss when formant decorrelates wet from dry in the
                // linear dry/wet blend.  At formant=0 wet≈dry (correlated), comp→1.  At
                // formant=1 the signals are decorrelated; constant-power scaling restores energy.
                // Use effectiveWetG = dw * masterEnv so comp→1 when engage is fully off.
                const float dryG    = 1.0f - dw * masterEnv;
                const float wetGEff = dw * masterEnv;   // effective wet contribution
                const float mixE    = dryG * dryG + wetGEff * wetGEff;
                const float cp      = mixE > 1e-4f ? 1.0f / std::sqrt (mixE) : 1.0f;
                const float comp    = 1.0f + (cp - 1.0f) * formant;
                buffer.setSample (c, i, (dry * dryG + wet * dw) * comp);
            }

            // Advance phrase playhead; trigger crossfade on loop wrap
            const float posBeforeAdvance = phraseReadPosF;
            phraseReadPosF += reverse ? -rate : rate;
            bool loopWrapped = false;
            if (phraseReadPosF < 0.0f)                { phraseReadPosF += (float) donorLength; loopWrapped = true; }
            if (phraseReadPosF >= (float) donorLength) { phraseReadPosF -= (float) donorLength; loopWrapped = true; }

            if (loopWrapped && scatterFadeCount == 0)
            {
                scatterOldPosF   = posBeforeAdvance;
                scatterFadeCount = kHopSize;
            }

            // Advance old playhead in parallel and count down the crossfade
            if (scatterFadeCount > 0)
            {
                scatterOldPosF += reverse ? -rate : rate;
                if (scatterOldPosF < 0.0f)                 scatterOldPosF += (float) donorLength;
                if (scatterOldPosF >= (float) donorLength)  scatterOldPosF -= (float) donorLength;
                --scatterFadeCount;
            }
        }
    }

    // Granular texture (added on top of morphed signal)
    if (grain > 0.0f)
        processGrains (buffer, numCh, numSamples);

    // Deferred OLA flush — clear stale wet state once silent.
    // MIDI mode: flush when all voice ADSRs have finished.
    // Effect mode: flush when engageGain ramp completes.
    const bool midiDone   = midiMode  && !anyVoiceActive() && donorFrozen;
    const bool effectDone = !midiMode && !donorFrozen && !phraseEngaged
                            && !engageGain.isSmoothing()     && engageGain.getCurrentValue()     < 0.001f
                            && !phraseEngageGain.isSmoothing() && phraseEngageGain.getCurrentValue() < 0.001f;
    if (midiDone || effectDone)
    {
        if (midiDone)
            donorFrozen = false;

        for (auto& cs : ch)
        {
            std::fill (cs.outputAccum.begin(), cs.outputAccum.end(), 0.0f);
            std::fill (cs.outputQueue.begin(), cs.outputQueue.end(), 0.0f);
            cs.outputQueuePos = kHopSize;

            // Also reset phrase formant OLA state
            std::fill (cs.phraseFormantAccum.begin(), cs.phraseFormantAccum.end(), 0.0f);
            std::fill (cs.phraseFormantQueue.begin(), cs.phraseFormantQueue.end(), 0.0f);
            cs.phraseFormantQueuePos = kHopSize;
        }
        for (auto& gr : grains)
            gr.active = false;
    }

    // Output limiter — catches hot signals from heavy morph/grain combinations
    juce::dsp::AudioBlock<float>        block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    limiter.process (ctx);
}
