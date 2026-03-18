#pragma once

#include "PluginProcessor.h"
#include "BinaryData.h"
#include "melatonin_inspector/melatonin_inspector.h"

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

    // Knobs
    juce::Slider morphKnob    { juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Slider grainKnob    { juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Slider formantKnob  { juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Slider scatterKnob  { juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Slider drywetKnob   { juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow };

    juce::Label morphLabel, grainLabel, formantLabel, scatterLabel, drywetLabel;

    // REC button
    juce::TextButton recButton { "● REC" };

    // APVTS attachments
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    SliderAttachment morphAttachment   { processorRef.apvts, "morph",      morphKnob };
    SliderAttachment grainAttachment   { processorRef.apvts, "grain",      grainKnob };
    SliderAttachment formantAttachment { processorRef.apvts, "formant",    formantKnob };
    SliderAttachment scatterAttachment { processorRef.apvts, "scatter",    scatterKnob };
    SliderAttachment drywetAttachment  { processorRef.apvts, "drywet",     drywetKnob };
    ButtonAttachment recAttachment     { processorRef.apvts, "recTrigger", recButton };

    // Donor fill meter state (updated on timer thread)
    float donorFillLevel = 0.0f;
    juce::Rectangle<int> fillMeterBounds;

    // Dev inspector
    std::unique_ptr<melatonin::Inspector> inspector;
    juce::TextButton inspectButton { "Inspect" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
