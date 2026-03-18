#pragma once

#include "PluginProcessor.h"
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

    // APVTS attachments
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    SA morphAttachment   { processorRef.apvts, "morph",      morphSlider };
    SA drywetAttachment  { processorRef.apvts, "drywet",     drywetSlider };
    SA grainAttachment   { processorRef.apvts, "grain",      grainSlider };
    SA scatterAttachment { processorRef.apvts, "scatter",    scatterSlider };
    SA formantAttachment { processorRef.apvts, "formant",    formantSlider };
    SA pitchAttachment   { processorRef.apvts, "pitch",      pitchSlider };
    BA recAttachment     { processorRef.apvts, "recTrigger", recButton };
    BA engageAttachment  { processorRef.apvts, "engage",     engageButton };
    BA reverseAttachment { processorRef.apvts, "reverse",    reverseButton };

    // State updated by timer
    float donorFillLevel = 0.0f;
    juce::Rectangle<int> displayBounds;

    // Dev inspector (tiny, tucked in corner)
    std::unique_ptr<melatonin::Inspector> inspector;
    juce::TextButton inspectButton { "i" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
