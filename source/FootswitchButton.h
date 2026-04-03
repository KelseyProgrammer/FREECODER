#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <BinaryData.h>

//==============================================================================
// FootswitchButton — same photorealistic knob image as the parameter knobs,
// with a prominent jade LED that glows when the toggle is active.
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
        auto b      = getLocalBounds().toFloat().reduced (3.0f);
        const float cx     = b.getCentreX();
        const float cy     = b.getCentreY();
        const float skirtR = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
        const float faceR  = skirtR * 0.62f;
        const bool  lit    = getToggleState();
        const auto  jade   = juce::Colour (0xff3dffaa);

        // ── Outer ambient bloom when active ───────────────────────────────────
        if (lit)
        {
            g.setColour (jade.withAlpha (0.20f));
            g.fillEllipse (cx - skirtR - 9, cy - skirtR - 9, (skirtR + 9) * 2, (skirtR + 9) * 2);
            g.setColour (jade.withAlpha (0.07f));
            g.fillEllipse (cx - skirtR - 18, cy - skirtR - 18, (skirtR + 18) * 2, (skirtR + 18) * 2);
        }

        // ── Drop shadow ───────────────────────────────────────────────────────
        g.setColour (juce::Colours::black.withAlpha (0.80f));
        g.fillEllipse (cx - skirtR + 2, cy - skirtR + 5, skirtR * 2, skirtR * 2);

        // ── Knob image body (same asset as parameter knobs) ───────────────────
        const auto& img = getKnobImage();
        if (img.isValid())
        {
            g.saveState();
            juce::Path clip;
            clip.addEllipse (cx - skirtR, cy - skirtR, skirtR * 2, skirtR * 2);
            g.reduceClipRegion (clip);

            const float scale = (skirtR * 2.0f) / (float) img.getWidth();
            g.drawImageTransformed (img,
                juce::AffineTransform::scale (scale).translated (cx - skirtR, cy - skirtR), false);

            // Jade face wash when active
            if (lit)
            {
                juce::ColourGradient wash (
                    jade.withAlpha (0.18f), cx, cy - faceR,
                    jade.withAlpha (0.06f), cx, cy + faceR * 0.8f, false);
                g.setGradientFill (wash);
                g.fillEllipse (cx - faceR, cy - faceR, faceR * 2, faceR * 2);
            }

            g.restoreState();
        }
        else
        {
            // Fallback procedural body
            {
                juce::ColourGradient sk (
                    juce::Colour (0xff303040), cx - skirtR * 0.35f, cy - skirtR * 0.55f,
                    juce::Colour (0xff0d0d18), cx + skirtR * 0.45f, cy + skirtR * 0.45f, true);
                g.setGradientFill (sk);
                g.fillEllipse (cx - skirtR, cy - skirtR, skirtR * 2, skirtR * 2);
            }
            {
                juce::ColourGradient fc (
                    lit ? juce::Colour (0xff182a20) : juce::Colour (0xff20203a),
                    cx, cy - faceR * 0.55f,
                    lit ? juce::Colour (0xff080f0c) : juce::Colour (0xff08080f),
                    cx, cy + faceR * 0.55f, false);
                g.setGradientFill (fc);
                g.fillEllipse (cx - faceR, cy - faceR, faceR * 2, faceR * 2);
            }
        }

        // ── Pressed darkening ─────────────────────────────────────────────────
        if (isDown)
        {
            g.saveState();
            juce::Path mask;
            mask.addEllipse (cx - skirtR, cy - skirtR, skirtR * 2, skirtR * 2);
            g.reduceClipRegion (mask);
            g.setColour (juce::Colours::black.withAlpha (0.28f));
            g.fillEllipse (cx - skirtR, cy - skirtR, skirtR * 2, skirtR * 2);
            g.restoreState();
        }

        // ── Active edge ring ──────────────────────────────────────────────────
        g.setColour (lit ? jade.withAlpha (0.65f) : juce::Colour (0xff2a2a3c));
        g.drawEllipse (cx - skirtR, cy - skirtR, skirtR * 2, skirtR * 2, lit ? 1.8f : 1.0f);

        // ── LED indicator ─────────────────────────────────────────────────────
        {
            const float ledR  = juce::jmax (3.5f, faceR * 0.21f);
            const float ledCx = cx;
            const float ledCy = cy - faceR * 0.52f;

            if (lit)
            {
                // Wide soft bloom
                g.setColour (jade.withAlpha (0.38f));
                g.fillEllipse (ledCx - ledR * 2.8f, ledCy - ledR * 2.8f, ledR * 5.6f, ledR * 5.6f);
                // Tighter inner glow
                g.setColour (jade.withAlpha (0.65f));
                g.fillEllipse (ledCx - ledR * 1.6f, ledCy - ledR * 1.6f, ledR * 3.2f, ledR * 3.2f);
            }

            // LED body — gradient from white-green centre to jade edge
            {
                juce::ColourGradient led (
                    lit ? juce::Colour (0xffd0fff0) : juce::Colour (0xff141422),
                    ledCx - ledR * 0.28f, ledCy - ledR * 0.36f,
                    lit ? jade                      : juce::Colour (0xff080810),
                    ledCx + ledR * 0.28f, ledCy + ledR * 0.36f, false);
                g.setGradientFill (led);
                g.fillEllipse (ledCx - ledR, ledCy - ledR, ledR * 2, ledR * 2);
            }

            if (lit)
            {
                // Specular dot (white glint, top-left of LED)
                g.setColour (juce::Colours::white.withAlpha (0.88f));
                g.fillEllipse (ledCx - ledR * 0.38f, ledCy - ledR * 0.62f,
                               ledR * 0.52f, ledR * 0.38f);
            }
        }
    }

private:
    mutable juce::Image knobImage;

    const juce::Image& getKnobImage() const
    {
        if (! knobImage.isValid())
        {
            auto full = juce::ImageCache::getFromMemory (BinaryData::knobs_png, BinaryData::knobs_pngSize);
            if (full.isValid())
                knobImage = full.getClippedImage ({ 0, 0, full.getWidth(), full.getWidth() });
        }
        return knobImage;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FootswitchButton)
};
