#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "SpectralEngine.h"

//==============================================================================
// Manages factory presets (param-only, donor preserved) and user presets
// (full binary: APVTS XML + donor buffer).  User presets are stored in
//   ~/Documents/Ament Audio/FREECODER/Presets/*.freecoder
//==============================================================================
class PresetManager
{
public:
    static const char* const kPresetExtension;   // ".freecoder"

    PresetManager (juce::AudioProcessorValueTreeState& apvts, SpectralEngine& engine);

    // Navigation
    void nextPreset();
    void previousPreset();
    void loadPresetAtIndex (int index);

    // Save — opens a native save dialog; async, safe to call from message thread
    void promptSavePreset (juce::Component* parent);

    // Re-scans the user presets folder (call after saving)
    void refreshUserPresets();

    // Query
    juce::String getCurrentPresetName() const { return currentName; }
    int  getCurrentPresetIndex()        const { return currentIndex; }
    int  getNumPresets()                const;

    static juce::File getUserPresetsDir();

private:
    juce::AudioProcessorValueTreeState& apvts;
    SpectralEngine& engine;

    //-- Factory presets -------------------------------------------------------
    struct FactoryPreset
    {
        const char* name;
        float morph, grain, formant, scatter, drywet, pitch;
    };
    static const std::vector<FactoryPreset>& factoryList();
    int numFactory() const;

    //-- User presets (files on disk) ------------------------------------------
    juce::Array<juce::File>            userPresets;
    int                                currentIndex = 0;
    juce::String                       currentName  = "Init";
    std::unique_ptr<juce::FileChooser> fileChooser;

    void applyFactory  (int factoryIndex);
    void applyUserFile (const juce::File& f);
    void saveToFile    (const juce::File& f);
};
