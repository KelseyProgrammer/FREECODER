#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <SpectralEngine.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// The exact blend formula used inside SpectralEngine::processFFTFrame.
// Tested in isolation so we can verify math without running a full FFT frame.
static float blendMag (float inputMag, float donorMag, float morph)
{
    return inputMag * (1.0f - morph) + donorMag * morph;
}

// Build a mono buffer filled with a constant value.
static juce::AudioBuffer<float> makeBuffer (int numSamples, float value = 0.0f)
{
    juce::AudioBuffer<float> buf (1, numSamples);
    buf.clear();
    for (int i = 0; i < numSamples; ++i)
        buf.setSample (0, i, value);
    return buf;
}

// Record N blocks of a 440 Hz sine wave (amplitude 0.5) into engine.
// Buffer is refilled before each process() call because process() writes output in-place.
static void recordSine (SpectralEngine& engine,
                        int numBlocks = 100,
                        float freqHz  = 440.0f,
                        double sr     = 44100.0)
{
    constexpr int kBlock = 512;
    engine.startRecording();
    for (int block = 0; block < numBlocks; ++block)
    {
        juce::AudioBuffer<float> buf (1, kBlock);
        for (int i = 0; i < kBlock; ++i)
        {
            const float t = (float) (block * kBlock + i) / (float) sr;
            buf.setSample (0, i, 0.5f * std::sin (2.0f * juce::MathConstants<float>::pi * freqHz * t));
        }
        engine.process (buf);
    }
    engine.stopRecording();
    // One silent block to let finishRecording() complete
    auto silence = makeBuffer (kBlock);
    engine.process (silence);
}

// ---------------------------------------------------------------------------
// Magnitude blend math
// ---------------------------------------------------------------------------

TEST_CASE ("Magnitude blend - morph extremes and midpoint", "[SpectralEngine][blend]")
{
    const float inputMag = 0.8f;
    const float donorMag = 0.2f;

    SECTION ("morph = 0.0 → full input magnitude")
    {
        REQUIRE (blendMag (inputMag, donorMag, 0.0f) == Catch::Approx (inputMag));
    }

    SECTION ("morph = 1.0 → full donor magnitude")
    {
        REQUIRE (blendMag (inputMag, donorMag, 1.0f) == Catch::Approx (donorMag));
    }

    SECTION ("morph = 0.5 → equal mix")
    {
        REQUIRE (blendMag (inputMag, donorMag, 0.5f) == Catch::Approx (0.5f));
    }

    SECTION ("blend is linear")
    {
        for (float m = 0.0f; m <= 1.0f; m += 0.1f)
            REQUIRE (blendMag (inputMag, donorMag, m) == Catch::Approx (inputMag + m * (donorMag - inputMag)));
    }
}

TEST_CASE ("Magnitude blend - identical magnitudes", "[SpectralEngine][blend]")
{
    const float mag = 0.6f;
    for (float m = 0.0f; m <= 1.0f; m += 0.25f)
        REQUIRE (blendMag (mag, mag, m) == Catch::Approx (mag));
}

// ---------------------------------------------------------------------------
// SpectralEngine state management
// ---------------------------------------------------------------------------

TEST_CASE ("SpectralEngine - initial state", "[SpectralEngine][state]")
{
    SpectralEngine engine;
    engine.prepare (44100.0, 512);

    REQUIRE (engine.getDonorFillLevel() == Catch::Approx (0.0f));
}

TEST_CASE ("SpectralEngine - donor fill level grows during recording", "[SpectralEngine][state]")
{
    SpectralEngine engine;
    engine.prepare (44100.0, 512);

    engine.startRecording();

    // Feed 4410 samples (0.1 sec @ 44100) of non-zero audio
    auto buf = makeBuffer (512, 0.5f);
    for (int block = 0; block < 9; ++block)
        engine.process (buf);

    // Fill level should be > 0 after recording some audio
    REQUIRE (engine.getDonorFillLevel() > 0.0f);

    // Still < 1.0 (we haven't filled 5 seconds yet)
    REQUIRE (engine.getDonorFillLevel() < 1.0f);
}

TEST_CASE ("SpectralEngine - startRecording resets fill level", "[SpectralEngine][state]")
{
    SpectralEngine engine;
    engine.prepare (44100.0, 512);

    // Record some audio
    engine.startRecording();
    auto buf = makeBuffer (512, 0.5f);
    engine.process (buf);
    engine.stopRecording();

    const float fillAfterFirst = engine.getDonorFillLevel();
    REQUIRE (fillAfterFirst > 0.0f);

    // Start recording again - should reset the buffer
    engine.startRecording();
    REQUIRE (engine.getDonorFillLevel() == Catch::Approx (0.0f));
}

TEST_CASE ("SpectralEngine - redundant start/stop calls are safe", "[SpectralEngine][state]")
{
    SpectralEngine engine;
    engine.prepare (44100.0, 512);

    // Calling stop before any recording started should not crash
    engine.stopRecording();

    // Double-start should not reset an in-progress recording
    engine.startRecording();
    auto buf = makeBuffer (512, 0.5f);
    engine.process (buf);
    const float fillMid = engine.getDonorFillLevel();

    engine.startRecording();  // should be a no-op
    REQUIRE (engine.getDonorFillLevel() == Catch::Approx (fillMid));
}

TEST_CASE ("SpectralEngine - process does not crash with silence", "[SpectralEngine][process]")
{
    SpectralEngine engine;
    engine.prepare (44100.0, 512);

    auto buf = makeBuffer (512, 0.0f);

    // Process 100 blocks of silence without a donor - should not crash or produce NaN
    for (int i = 0; i < 100; ++i)
    {
        engine.process (buf);
        for (int s = 0; s < buf.getNumSamples(); ++s)
            REQUIRE (std::isfinite (buf.getSample (0, s)));
    }
}

// ---------------------------------------------------------------------------
// Waveform snapshot
// ---------------------------------------------------------------------------

TEST_CASE ("SpectralEngine - waveform snapshot populated after recording", "[SpectralEngine][waveform]")
{
    SpectralEngine engine;
    engine.prepare (44100.0, 512);

    recordSine (engine);

    SpectralEngine::WaveformSnapshot snap;
    REQUIRE (engine.getWaveformSnapshot (snap));
    REQUIRE (snap.hasData);

    // At least one peak segment must be non-zero (recording had signal)
    float maxPeak = 0.0f;
    for (float p : snap.peaks)
        maxPeak = std::max (maxPeak, p);
    REQUIRE (maxPeak > 0.0f);
}

TEST_CASE ("SpectralEngine - waveform snapshot not present before recording", "[SpectralEngine][waveform]")
{
    SpectralEngine engine;
    engine.prepare (44100.0, 512);

    SpectralEngine::WaveformSnapshot snap;
    engine.getWaveformSnapshot (snap);
    REQUIRE_FALSE (snap.hasData);
}

// ---------------------------------------------------------------------------
// Scatter crossfade
// ---------------------------------------------------------------------------

TEST_CASE ("SpectralEngine - scatter + phrase: output stays finite", "[SpectralEngine][scatter]")
{
    SpectralEngine engine;
    engine.prepare (44100.0, 512);
    recordSine (engine, 100, 441.0f);  // 441 Hz: integer period, seamless loop wrap

    // Snap smoothers: morph=0 (phrase only), grain=0, formant=0, scatter=1, drywet=1, pitch=0
    engine.snapSmoothers (0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
    engine.setPhraseEngage (true);

    for (int block = 0; block < 100; ++block)
    {
        auto buf = makeBuffer (512);
        engine.process (buf);
        for (int i = 0; i < 512; ++i)
            REQUIRE (std::isfinite (buf.getSample (0, i)));
    }
}

TEST_CASE ("SpectralEngine - scatter crossfade: no full-scale per-sample jumps", "[SpectralEngine][scatter]")
{
    // Use 441 Hz: period = 44100/441 = 100 samples exactly.
    // 100 blocks x 512 = 51200 samples = 512 complete periods => seamless loop wrap.
    // Without the scatter crossfade, a jump covers the full amplitude swing (~1.0).
    // With a kHopSize-sample crossfade, each scattered jump contributes < 0.002/sample.
    // The 441 Hz sine slope is ~0.031/sample at its steepest; threshold 0.2 is safe.

    SpectralEngine engine;
    engine.prepare (44100.0, 512);
    recordSine (engine, 100, 441.0f);

    engine.snapSmoothers (0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
    engine.setPhraseEngage (true);

    // Warm up 5 blocks so the phrase engage ramp reaches 1.0 before checking deltas.
    // Capture the last warmup sample so the first checking delta isn't from zero.
    float prevSample = 0.0f;
    for (int warmup = 0; warmup < 5; ++warmup)
    {
        auto buf = makeBuffer (512);
        engine.process (buf);
        prevSample = buf.getSample (0, 511);
    }
    for (int block = 0; block < 100; ++block)
    {
        auto buf = makeBuffer (512);
        engine.process (buf);
        for (int i = 0; i < 512; ++i)
        {
            const float s = buf.getSample (0, i);
            REQUIRE (std::abs (s - prevSample) < 0.2f);
            prevSample = s;
        }
    }
}

// ---------------------------------------------------------------------------
// MIDI voice steal
// ---------------------------------------------------------------------------

TEST_CASE ("SpectralEngine - MIDI voice steal: no crash beyond kMaxVoices", "[SpectralEngine][midi]")
{
    SpectralEngine engine;
    engine.prepare (44100.0, 512);
    recordSine (engine);

    engine.setMidiMode (true);
    engine.setAdsrParams (0.01f, 0.2f, 1.0f, 0.1f);

    // Trigger two more notes than the voice limit — the extras must steal voices
    for (int note = 60; note < 60 + SpectralEngine::kMaxVoices + 2; ++note)
        engine.triggerMidiNoteOn (note, (float) (note - 60), 0.8f);

    for (int block = 0; block < 20; ++block)
    {
        auto buf = makeBuffer (512);
        engine.process (buf);
        for (int i = 0; i < 512; ++i)
            REQUIRE (std::isfinite (buf.getSample (0, i)));
    }
}

TEST_CASE ("SpectralEngine - MIDI note on produces non-zero output", "[SpectralEngine][midi]")
{
    SpectralEngine engine;
    engine.prepare (44100.0, 512);
    recordSine (engine);

    engine.setMidiMode (true);
    engine.setAdsrParams (0.001f, 10.0f, 1.0f, 0.1f);  // near-instant attack, long sustain
    engine.snapSmoothers (0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);  // morph=0 (phrase), drywet=1

    engine.triggerMidiNoteOn (60, 0.0f, 1.0f);  // root note, full velocity

    // Let attack ramp complete (0.001 s = ~44 samples, well within one block)
    float maxOut = 0.0f;
    for (int block = 0; block < 10; ++block)
    {
        auto buf = makeBuffer (512);
        engine.process (buf);
        for (int i = 0; i < 512; ++i)
            maxOut = std::max (maxOut, std::abs (buf.getSample (0, i)));
    }

    REQUIRE (maxOut > 0.0f);
}

TEST_CASE ("SpectralEngine - MIDI note off: output fades after fast release", "[SpectralEngine][midi]")
{
    SpectralEngine engine;
    engine.prepare (44100.0, 512);
    recordSine (engine);

    engine.setMidiMode (true);
    // Fast attack, fast decay, sustain=1, very fast release (≈ 2 ms)
    engine.setAdsrParams (0.001f, 0.001f, 1.0f, 0.002f);
    engine.snapSmoothers (0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    engine.triggerMidiNoteOn (60, 0.0f, 1.0f);

    // Hold for 10 blocks so the voice is clearly active
    for (int block = 0; block < 10; ++block)
    {
        auto buf = makeBuffer (512);
        engine.process (buf);
    }

    engine.triggerMidiNoteOff (60);

    // Process enough blocks for the release (0.002 s ≈ 88 samples ≈ 1 block) to finish
    for (int block = 0; block < 10; ++block)
    {
        auto buf = makeBuffer (512);
        engine.process (buf);
    }

    // After release, output should be essentially silent
    float maxAfterRelease = 0.0f;
    for (int block = 0; block < 5; ++block)
    {
        auto buf = makeBuffer (512);
        engine.process (buf);
        for (int i = 0; i < 512; ++i)
            maxAfterRelease = std::max (maxAfterRelease, std::abs (buf.getSample (0, i)));
    }

    REQUIRE (maxAfterRelease < 0.01f);
}
