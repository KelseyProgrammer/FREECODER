#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    morphLabel.setText   ("MORPH",   juce::dontSendNotification);
    grainLabel.setText   ("GRAIN",   juce::dontSendNotification);
    formantLabel.setText ("FORMANT", juce::dontSendNotification);
    scatterLabel.setText ("SCATTER", juce::dontSendNotification);
    drywetLabel.setText  ("DRY/WET", juce::dontSendNotification);

    for (auto* label : { &morphLabel, &grainLabel, &formantLabel, &scatterLabel, &drywetLabel })
    {
        label->setJustificationType (juce::Justification::centred);
        label->setColour (juce::Label::textColourId, juce::Colour (0xffaaaacc));
        addAndMakeVisible (label);
    }

    for (auto* knob : { &morphKnob, &grainKnob, &formantKnob, &scatterKnob, &drywetKnob })
    {
        knob->setColour (juce::Slider::rotarySliderFillColourId,   juce::Colour (0xff7070ee));
        knob->setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff333355));
        knob->setColour (juce::Slider::textBoxTextColourId,         juce::Colour (0xffaaaacc));
        knob->setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
        addAndMakeVisible (knob);
    }

    recButton.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff333355));
    recButton.setColour (juce::TextButton::buttonOnColourId, juce::Colours::red);
    recButton.setColour (juce::TextButton::textColourOffId,  juce::Colours::white);
    addAndMakeVisible (recButton);

    inspectButton.setColour (juce::TextButton::buttonColourId,  juce::Colour (0xff222233));
    inspectButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff666688));
    inspectButton.onClick = [&] {
        if (!inspector)
        {
            inspector = std::make_unique<melatonin::Inspector> (*this);
            inspector->onClose = [this]() { inspector.reset(); };
        }
        inspector->setVisible (true);
    };
    addAndMakeVisible (inspectButton);

    startTimerHz (15); // refresh fill meter ~15x per second
    setSize (480, 300);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
}

void PluginEditor::timerCallback()
{
    const float newLevel = processorRef.getDonorFillLevel();
    if (std::abs (newLevel - donorFillLevel) > 0.001f)
    {
        donorFillLevel = newLevel;
        repaint (fillMeterBounds);
    }
}

void PluginEditor::paint (juce::Graphics& g)
{
    // Background
    g.fillAll (juce::Colour (0xff1a1a2e));

    // Title
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (20.0f).withStyle ("Bold"));
    g.drawText ("FREECODER", getLocalBounds().removeFromTop (38), juce::Justification::centred, false);

    // Version
    g.setFont (juce::FontOptions (10.0f));
    g.setColour (juce::Colour (0xff555577));
    g.drawText ("v" VERSION, getLocalBounds().removeFromTop (54).removeFromBottom (16),
                juce::Justification::centred, false);

    // Donor fill meter background
    g.setColour (juce::Colour (0xff222240));
    g.fillRoundedRectangle (fillMeterBounds.toFloat(), 3.0f);

    // Donor fill meter fill
    if (donorFillLevel > 0.0f)
    {
        auto filled = fillMeterBounds.withWidth ((int) (fillMeterBounds.getWidth() * donorFillLevel));
        g.setColour (donorFillLevel >= 1.0f ? juce::Colours::red
                                             : juce::Colour (0xff5050dd));
        g.fillRoundedRectangle (filled.toFloat(), 3.0f);
    }

    // Meter label
    g.setFont (juce::FontOptions (9.0f));
    g.setColour (juce::Colour (0xff555577));
    g.drawText ("DONOR", fillMeterBounds, juce::Justification::centred, false);
}

void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced (12);
    area.removeFromTop (58); // title + version

    // REC button + fill meter + inspect button
    auto topRow = area.removeFromTop (28);
    recButton.setBounds (topRow.removeFromLeft (80));
    topRow.removeFromLeft (8);
    inspectButton.setBounds (topRow.removeFromRight (60));
    topRow.removeFromRight (8);
    fillMeterBounds = topRow; // remaining space is the fill meter

    area.removeFromTop (10);

    // Knobs row 1: MORPH, GRAIN, FORMANT
    auto knobRow1 = area.removeFromTop (110);
    const int kw = knobRow1.getWidth() / 3;

    auto layoutKnob = [](juce::Rectangle<int> cell, juce::Slider& knob, juce::Label& label)
    {
        label.setBounds (cell.removeFromTop (16));
        knob.setBounds  (cell);
    };

    layoutKnob (knobRow1.removeFromLeft (kw), morphKnob,   morphLabel);
    layoutKnob (knobRow1.removeFromLeft (kw), grainKnob,   grainLabel);
    layoutKnob (knobRow1,                     formantKnob, formantLabel);

    area.removeFromTop (8);

    // Knobs row 2: SCATTER, DRY/WET
    auto knobRow2 = area.removeFromTop (110);
    const int kw2 = knobRow2.getWidth() / 3;

    layoutKnob (knobRow2.removeFromLeft (kw2), scatterKnob, scatterLabel);
    layoutKnob (knobRow2.removeFromLeft (kw2), drywetKnob,  drywetLabel);
}
