#pragma once

#include "PluginProcessor.h"
#include "PresetManager.h"
#include "BinaryData.h"
#include "melatonin_inspector/melatonin_inspector.h"

class FreecoderLookAndFeel;

//==============================================================================
class PluginEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    PluginProcessor& processorRef;
    std::unique_ptr<FreecoderLookAndFeel> laf;

    // Top horizontal sliders (like the pedal's strip sliders)
    juce::Slider morphSlider  { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
    juce::Slider drywetSlider { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };

    // Pad sliders (illuminated rectangular pads, all 4 active)
    juce::Slider grainSlider   { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider scatterSlider { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider formantSlider { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider pitchSlider   { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };

    // Footswitches
    juce::TextButton recButton     { "REC" };
    juce::TextButton engageButton  { "ENGAGE" };
    juce::TextButton reverseButton { "REVERSE" };
    juce::TextButton phraseButton  { "PHRASE" };

    // APVTS attachments
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    SA morphAttachment   { processorRef.apvts, "morph",      morphSlider };
    SA drywetAttachment  { processorRef.apvts, "drywet",     drywetSlider };
    SA grainAttachment   { processorRef.apvts, "grain",      grainSlider };
    SA scatterAttachment { processorRef.apvts, "scatter",    scatterSlider };
    SA formantAttachment { processorRef.apvts, "formant",    formantSlider };
    SA pitchAttachment   { processorRef.apvts, "pitch",      pitchSlider };
    BA recAttachment     { processorRef.apvts, "recTrigger",   recButton };
    BA engageAttachment  { processorRef.apvts, "engage",       engageButton };
    BA reverseAttachment { processorRef.apvts, "reverse",      reverseButton };
    BA phraseAttachment  { processorRef.apvts, "phraseEngage", phraseButton };

    // State updated by timer
    float donorFillLevel = 0.0f;
    juce::Rectangle<int> displayBounds;

    // Spectrum visualizer + tuner
    SpectralEngine::SpectrumSnapshot spectrumSnapshot;
    SpectralEngine::TunerResult      tunerResult;

    // MIDI mode controls
    juce::TextButton modeButton     { "EFFECT" };
    juce::Slider     rootNoteSlider { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
    BA modeAttachment     { processorRef.apvts, "midiMode", modeButton };
    SA rootNoteAttachment { processorRef.apvts, "rootNote", rootNoteSlider };

    // ADSR knobs (visible only in MIDI mode)
    juce::Slider adsrAttackSlider  { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider adsrDecaySlider   { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider adsrSustainSlider { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider adsrReleaseSlider { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    SA adsrAttachA { processorRef.apvts, "adsrAttack",  adsrAttackSlider };
    SA adsrAttachD { processorRef.apvts, "adsrDecay",   adsrDecaySlider };
    SA adsrAttachS { processorRef.apvts, "adsrSustain", adsrSustainSlider };
    SA adsrAttachR { processorRef.apvts, "adsrRelease", adsrReleaseSlider };

    // Record length selector (small stepped slider near REC)
    juce::Slider recLengthSlider { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
    SA recLengthAttachment { processorRef.apvts, "recLength", recLengthSlider };

    // Auto-engage toggle
    juce::TextButton autoEngageButton { "AUTO" };
    BA autoEngageAttachment { processorRef.apvts, "autoEngage", autoEngageButton };

    // Effect ADSR toggle (enables ADSR shaping of FREEZE engage in effect mode)
    juce::TextButton effectAdsrButton { "ADSR" };
    BA effectAdsrAttachment { processorRef.apvts, "effectAdsr", effectAdsrButton };

    // Latch mode toggle (MIDI mode only)
    juce::TextButton latchButton { "LATCH" };
    BA latchAttachment { processorRef.apvts, "latch", latchButton };

    // Donor slot buttons A / B / C + export
    juce::TextButton slotButtonA { "A" }, slotButtonB { "B" }, slotButtonC { "C" };
    juce::TextButton* slotButtons[SpectralEngine::kNumDonorSlots] { &slotButtonA, &slotButtonB, &slotButtonC };
    juce::TextButton exportButton { "EXP" };
    juce::TextButton importButton { "IMP" };

    // Preset browser strip
    PresetManager&   presetManager;
    juce::TextButton prevPresetButton { "<" };
    juce::TextButton nextPresetButton { ">" };
    juce::TextButton savePresetButton { "SAVE" };

    // Dev inspector (tiny, tucked in corner)
    std::unique_ptr<melatonin::Inspector> inspector;
    juce::TextButton inspectButton { "i" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
