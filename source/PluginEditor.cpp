#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    // Labels
    morphLabel.setText   ("MORPH",   juce::dontSendNotification);
    grainLabel.setText   ("GRAIN",   juce::dontSendNotification);
    formantLabel.setText ("FORMANT", juce::dontSendNotification);
    scatterLabel.setText ("SCATTER", juce::dontSendNotification);
    drywetLabel.setText  ("DRY/WET", juce::dontSendNotification);

    for (auto* label : { &morphLabel, &grainLabel, &formantLabel, &scatterLabel, &drywetLabel })
    {
        label->setJustificationType (juce::Justification::centred);
        addAndMakeVisible (label);
    }

    for (auto* knob : { &morphKnob, &grainKnob, &formantKnob, &scatterKnob, &drywetKnob })
        addAndMakeVisible (knob);

    recButton.setColour (juce::TextButton::buttonOnColourId, juce::Colours::red);
    addAndMakeVisible (recButton);

    inspectButton.onClick = [&] {
        if (!inspector)
        {
            inspector = std::make_unique<melatonin::Inspector> (*this);
            inspector->onClose = [this]() { inspector.reset(); };
        }
        inspector->setVisible (true);
    };
    addAndMakeVisible (inspectButton);

    setSize (480, 280);
}

PluginEditor::~PluginEditor()
{
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e2e));

    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (20.0f, juce::Font::bold));
    g.drawText ("FREECODER", getLocalBounds().removeFromTop (40), juce::Justification::centred, false);

    g.setFont (11.0f);
    g.setColour (juce::Colours::grey);
    g.drawText ("v" VERSION, getLocalBounds().removeFromTop (60).removeFromBottom (20), juce::Justification::centred, false);
}

void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced (12);

    // Title row
    area.removeFromTop (60);

    // REC button + inspect
    auto topRow = area.removeFromTop (36);
    recButton.setBounds (topRow.removeFromLeft (80));
    topRow.removeFromLeft (8);
    inspectButton.setBounds (topRow.removeFromRight (70));

    area.removeFromTop (8);

    // Knobs row 1: MORPH, GRAIN, FORMANT
    auto knobRow1 = area.removeFromTop (110);
    const int knobW = knobRow1.getWidth() / 3;

    auto layoutKnob = [&] (juce::Rectangle<int> cell, juce::Slider& knob, juce::Label& label)
    {
        label.setBounds (cell.removeFromTop (18));
        knob.setBounds  (cell);
    };

    layoutKnob (knobRow1.removeFromLeft (knobW), morphKnob,   morphLabel);
    layoutKnob (knobRow1.removeFromLeft (knobW), grainKnob,   grainLabel);
    layoutKnob (knobRow1,                        formantKnob, formantLabel);

    area.removeFromTop (8);

    // Knobs row 2: SCATTER, DRY/WET
    auto knobRow2 = area.removeFromTop (110);
    const int knobW2 = knobRow2.getWidth() / 3;

    layoutKnob (knobRow2.removeFromLeft (knobW2), scatterKnob, scatterLabel);
    layoutKnob (knobRow2.removeFromLeft (knobW2), drywetKnob,  drywetLabel);
}
