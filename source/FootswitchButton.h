#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
// FootswitchButton — self-painting rubber-dome footswitch toggle
// Owns its own paintButton() so no external LookAndFeel override is needed.
//==============================================================================
class FootswitchButton : public juce::Button
{
public:
    explicit FootswitchButton (const juce::String& name) : juce::Button (name)
    {
        setClickingTogglesState (true);
    }

protected:
    void paintButton (juce::Graphics& g, bool, bool isDown) override
    {
        auto b  = getLocalBounds().toFloat().reduced (4.0f);
        float cx = b.getCentreX(), cy = b.getCentreY();
        float r  = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
        const bool lit = getToggleState();

        // Ambient glow when engaged
        if (lit)
        {
            g.setColour (juce::Colour (0xff44ff44).withAlpha (0.13f));
            g.fillEllipse (cx - r - 6, cy - r - 6, (r + 6) * 2, (r + 6) * 2);
        }

        // Drop shadow
        g.setColour (juce::Colours::black.withAlpha (0.70f));
        g.fillEllipse (cx - r + 2, cy - r + 4, r * 2, r * 2);

        // Bevelled outer metal ring
        {
            juce::ColourGradient ring (
                juce::Colour (0xff555555), cx - r * 0.45f, cy - r * 0.75f,
                juce::Colour (0xff181818), cx + r * 0.35f, cy + r * 0.65f, false);
            g.setGradientFill (ring);
            g.fillEllipse (cx - r, cy - r, r * 2, r * 2);
        }

        // Inner shadow rim
        g.setColour (juce::Colours::black.withAlpha (0.50f));
        g.fillEllipse (cx - r * 0.90f, cy - r * 0.90f, r * 1.80f, r * 1.80f);

        // Rubber dome body
        {
            const float dr = r * 0.82f;
            const float pressY = isDown ? 1.5f : 0.0f;
            juce::ColourGradient dome (
                lit ? juce::Colour (0xff142814) : juce::Colour (0xff252525),
                cx, cy - dr * 0.5f + pressY,
                lit ? juce::Colour (0xff080f08) : juce::Colour (0xff0a0a0a),
                cx, cy + dr * 0.65f + pressY, false);
            g.setGradientFill (dome);
            g.fillEllipse (cx - dr, cy - dr, dr * 2, dr * 2);
        }

        // Dome specular — convex highlight upper-left
        if (!isDown)
        {
            juce::ColourGradient spec (
                juce::Colours::white.withAlpha (0.28f), cx - r * 0.22f, cy - r * 0.68f,
                juce::Colours::white.withAlpha (0.00f), cx + r * 0.10f, cy - r * 0.08f, false);
            g.setGradientFill (spec);
            g.fillEllipse (cx - r * 0.65f, cy - r * 0.88f, r * 1.25f, r * 0.82f);
        }

        // Outer ring stroke
        g.setColour (juce::Colour (lit ? 0xff385038u : 0xff3a3a3au));
        g.drawEllipse (cx - r, cy - r, r * 2, r * 2, 1.5f);

        // LED
        const float ledR = 5.0f;
        const float ledY = cy - r * 0.62f;
        if (lit)
        {
            g.setColour (juce::Colour (0xff44ff44).withAlpha (0.45f));
            g.fillEllipse (cx - ledR - 3, ledY - 3, (ledR + 3) * 2, (ledR + 3) * 2);
        }
        g.setColour (juce::Colour (lit ? 0xff88ff88u : 0xff0b250bu));
        g.fillEllipse (cx - ledR, ledY, ledR * 2, ledR * 2);
        if (lit)  // LED specular
        {
            g.setColour (juce::Colours::white.withAlpha (0.55f));
            g.fillEllipse (cx - ledR * 0.45f, ledY + ledR * 0.12f, ledR * 0.55f, ledR * 0.42f);
        }

    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FootswitchButton)
};
