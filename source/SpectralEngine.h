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

    void setMorph  (float m) noexcept { morph  = m; }
    void setDryWet (float d) noexcept { dryWet = d; }

private:
    static constexpr int kFFTOrder        = 11;              // 2^11 = 2048
    static constexpr int kFFTSize         = 1 << kFFTOrder;  // 2048
    static constexpr int kHopSize         = kFFTSize / 4;    // 512  (4x overlap)
    static constexpr int kMaxDonorSamples = 220500;          // ~5 sec @ 44100

    void processFFTFrame (int ch);
    void analyseDonorFrame();

    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;

    struct ChannelState
    {
        std::vector<float> inputRing;    // circular, size kFFTSize
        int                inputWritePos = 0;
        std::vector<float> outputAccum;  // overlap-add accumulator, size kFFTSize
        std::vector<float> outputQueue;  // ready-to-deliver, size kHopSize
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

    std::vector<float> fftScratch; // size 2 * kFFTSize
    std::vector<float> donorMag;   // size kFFTSize / 2 + 1

    juce::AudioBuffer<float> donorBuffer;
    int  donorWritePos  = 0;
    int  donorLength    = 0;
    int  donorReadPos   = 0;
    bool donorRecording = false;
    bool hasDonor       = false;

    float morph  = 0.5f;
    float dryWet = 0.8f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectralEngine)
};
