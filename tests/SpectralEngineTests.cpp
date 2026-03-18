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
