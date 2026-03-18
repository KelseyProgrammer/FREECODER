#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class SpectralEngine
{
public:
    SpectralEngine();
    ~SpectralEngine() = default;

    void prepare (double sampleRate, int blockSize);
    void process (juce::AudioBuffer<float>& buffer);

    void startRecording();
    void stopRecording();

    float getDonorFillLevel() const;

    void setMorph   (float v) noexcept { morph   = v; }
    void setDryWet  (float v) noexcept { dryWet  = v; }
    void setGrain   (float v) noexcept { grain   = v; }
    void setFormant (float v) noexcept { formant = v; }
    void setScatter (float v) noexcept { scatter = v; }

private:
    static constexpr int kFFTOrder        = 11;
    static constexpr int kFFTSize         = 1 << kFFTOrder;  // 2048
    static constexpr int kHopSize         = kFFTSize / 4;    // 512  (4x overlap)
    static constexpr int kNumBins         = kFFTSize / 2 + 1; // 1025
    static constexpr int kMaxDonorSamples = 220500;           // ~5 sec @ 44100
    static constexpr int kMaxGrains       = 16;
    static constexpr int kGrainSamples    = 2048;

    // FFT frame processing
    void processFFTFrame (int ch);
    void analyseDonorFrame();
    void computeEnvelope (const float* mag, float* env, int numBins, int halfWindow) const;

    // Grain engine
    struct Grain { int readPos = 0, size = 0, age = 0; bool active = false; };
    void processGrains (juce::AudioBuffer<float>& buffer, int numCh, int numSamples);
    void spawnGrain();

    // FFT objects
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;

    // Per-channel OLA state
    struct ChannelState
    {
        std::vector<float> inputRing;
        int                inputWritePos  = 0;
        std::vector<float> outputAccum;
        std::vector<float> outputQueue;
        int                outputQueuePos = 0;

        void init()
        {
            inputRing.assign   (kFFTSize, 0.0f);
            outputAccum.assign (kFFTSize, 0.0f);
            outputQueue.assign (kHopSize, 0.0f);
            inputWritePos = outputQueuePos = 0;
        }
    };

    std::array<ChannelState, 2> ch;
    int hopCount = 0;

    // Scratch buffers (audio thread only)
    std::vector<float> fftScratch;    // 2 * kFFTSize
    std::vector<float> inputMag;      // kNumBins
    std::vector<float> inputPhase;    // kNumBins
    std::vector<float> liveEnvelope;  // kNumBins
    std::vector<float> donorMag;      // kNumBins
    std::vector<float> donorEnvelope; // kNumBins

    // Donor buffer
    juce::AudioBuffer<float> donorBuffer;
    int  donorWritePos  = 0;
    int  donorLength    = 0;
    int  donorReadPos   = 0;
    bool donorRecording = false;
    bool hasDonor       = false;

    // Grain state
    std::array<Grain, kMaxGrains> grains;
    int          grainSpawnCounter = 0;
    juce::Random rng;

    // Parameters (audio thread)
    float morph   = 0.5f;
    float dryWet  = 0.8f;
    float grain   = 0.0f;
    float formant = 0.5f;
    float scatter = 0.3f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectralEngine)
};
