#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "SpectralEngine.h"
#include "PresetManager.h"

#if (MSVC)
#include "ipps.h"
#endif

class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    float getDonorFillLevel() const { return spectralEngine.getDonorFillLevel(); }
    const juce::AudioBuffer<float>& getDonorBuffer() const { return spectralEngine.getDonorBuffer(); }
    int   getDonorLength()   const { return spectralEngine.getDonorLength(); }

    PresetManager& getPresetManager() { return presetManager; }

    bool getSpectrumSnapshot (SpectralEngine::SpectrumSnapshot& out)
    {
        return spectralEngine.getSpectrumSnapshot (out);
    }

    bool getTunerResult (SpectralEngine::TunerResult& out)
    {
        return spectralEngine.getTunerResult (out);
    }

    // Donor slot access for the UI
    void requestSlot    (int n)  { requestedSlot.store (n); }
    int  getActiveSlot  () const { return spectralEngine.getActiveSlot(); }
    bool donorSlotHasData (int n) const { return spectralEngine.donorSlotHasData (n); }

    // Export the active donor slot's audio to a WAV file chosen by the user
    void exportActiveDonorSlotToWav();

    // Import any audio file from disk as the donor for the active slot
    void importDonorFromFile();

    // Diagnostics — read by editor, written by audio thread
    std::atomic<int> diagInputChannels { 0 };
    std::atomic<int> diagBlockSize     { 0 };
    std::atomic<int> diagBlockCount    { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    SpectralEngine spectralEngine;
    PresetManager  presetManager;
    bool           prevRecTrigger   = false; // edge-detection for REC button; audio thread only
    std::atomic<int> requestedSlot { 0 };   // UI writes; audio thread reads each block
    int            currentSlot     = 0;     // audio thread only

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
