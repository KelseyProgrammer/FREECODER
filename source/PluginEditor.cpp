#include "PluginEditor.h"

//==============================================================================
// Pedal-inspired LookAndFeel — black + neon green
//==============================================================================
class FreecoderLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // Horizontal strip slider (MORPH / DRY-WET)
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float, float,
                           juce::Slider::SliderStyle, juce::Slider&) override
    {
        const float cy = y + height * 0.5f;
        const float th = 5.0f;

        // Track background
        juce::ColourGradient trackBg (juce::Colour (0xff0d200d), (float) x, cy,
                                       juce::Colour (0xff071407), (float) (x + width), cy, false);
        g.setGradientFill (trackBg);
        g.fillRoundedRectangle ((float) x, cy - th * 0.5f, (float) width, th, 2.5f);

        // Active fill
        juce::ColourGradient fill (juce::Colour (0xff1a8a1a), (float) x, cy,
                                    juce::Colour (0xff44ff44), sliderPos, cy, false);
        g.setGradientFill (fill);
        g.fillRoundedRectangle ((float) x, cy - th * 0.5f, sliderPos - (float) x, th, 2.5f);

        // Thumb
        const float tw = 12.0f, th2 = 26.0f;
        {
            juce::ColourGradient thumb (juce::Colour (0xff5fff5f), sliderPos - tw * 0.4f, cy - th2 * 0.4f,
                                         juce::Colour (0xff1a8a1a), sliderPos + tw * 0.4f, cy + th2 * 0.4f, false);
            g.setGradientFill (thumb);
            g.fillRoundedRectangle (sliderPos - tw * 0.5f, cy - th2 * 0.5f, tw, th2, 3.5f);
        }
        // Thumb specular
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.fillRoundedRectangle (sliderPos - tw * 0.5f + 2.5f, cy - th2 * 0.5f + 3.0f, tw - 5.0f, th2 * 0.32f, 2.0f);
    }

    // Pad slider — 3D circular knob (Serum / FabFilter style)
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        const float prop   = sliderPosProportional;
        const float cx     = (float) x + (float) width  * 0.5f;
        const float cy     = (float) y + (float) height * 0.5f;
        const float outerR = juce::jmin ((float) width, (float) height) * 0.5f - 3.0f;
        const float arcThk = outerR * 0.175f;
        const float bodyR  = outerR - arcThk - 3.0f;
        const float innerR = bodyR * 0.76f;

        // ── Arc track ─────────────────────────────────────────────────────
        const float arcR = outerR - arcThk * 0.5f;
        {
            juce::Path track;
            track.addCentredArc (cx, cy, arcR, arcR, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
            g.setColour (juce::Colour (0xff1c1c1c));
            g.strokePath (track, juce::PathStrokeType (arcThk,
                          juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // ── Value arc ─────────────────────────────────────────────────────
        const float angle = rotaryStartAngle + prop * (rotaryEndAngle - rotaryStartAngle);
        if (prop > 0.004f)
        {
            juce::Path val;
            val.addCentredArc (cx, cy, arcR, arcR, 0.0f, rotaryStartAngle, angle, true);
            g.setColour (juce::Colour (0xff44ff44));
            g.strokePath (val, juce::PathStrokeType (arcThk,
                          juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // ── Outer glow ─────────────────────────────────────────────────────
        if (prop > 0.01f)
        {
            g.setColour (juce::Colour (0xff44ff44).withAlpha (prop * 0.10f));
            g.fillEllipse (cx - outerR - 2, cy - outerR - 2, (outerR + 2) * 2, (outerR + 2) * 2);
        }

        // ── Drop shadow ────────────────────────────────────────────────────
        g.setColour (juce::Colours::black.withAlpha (0.65f));
        g.fillEllipse (cx - bodyR + 2, cy - bodyR + 3, bodyR * 2, bodyR * 2);

        // ── Knob body (radial gradient, dark center, lighter edge) ─────────
        {
            juce::ColourGradient grad (
                juce::Colour (0xff2a2a2a), cx - bodyR * 0.35f, cy - bodyR * 0.35f,
                juce::Colour (0xff0d0d0d), cx + bodyR * 0.55f, cy + bodyR * 0.55f,
                true);
            g.setGradientFill (grad);
            g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2, bodyR * 2);
        }

        // ── Inner face (recessed look) ─────────────────────────────────────
        {
            juce::ColourGradient inner (
                juce::Colour (0xff1d1d1d), cx, cy - innerR * 0.55f,
                juce::Colour (0xff090909), cx, cy + innerR * 0.55f,
                false);
            g.setGradientFill (inner);
            g.fillEllipse (cx - innerR, cy - innerR, innerR * 2, innerR * 2);
        }

        // ── Outer ring ─────────────────────────────────────────────────────
        g.setColour (juce::Colour (0xff444444));
        g.drawEllipse (cx - bodyR, cy - bodyR, bodyR * 2, bodyR * 2, 1.2f);

        // ── Specular highlight ─────────────────────────────────────────────
        {
            juce::ColourGradient spec (
                juce::Colours::white.withAlpha (0.20f), cx - bodyR * 0.2f, cy - bodyR * 0.62f,
                juce::Colours::white.withAlpha (0.0f),  cx + bodyR * 0.1f, cy - bodyR * 0.1f,
                false);
            g.setGradientFill (spec);
            g.fillEllipse (cx - bodyR * 0.55f, cy - bodyR * 0.74f, bodyR * 0.68f, bodyR * 0.42f);
        }

        // ── Pointer dot ────────────────────────────────────────────────────
        {
            const float pa  = angle - juce::MathConstants<float>::halfPi;
            const float pr  = innerR * 0.60f;
            const float dotSize = juce::jmax (5.0f, bodyR * 0.22f);
            g.setColour (juce::Colour (0xff44ff44).withAlpha (0.85f + prop * 0.15f));
            g.fillEllipse (cx + std::cos (pa) * pr - dotSize * 0.5f,
                           cy + std::sin (pa) * pr - dotSize * 0.5f,
                           dotSize, dotSize);
        }

        // ── Value text ────────────────────────────────────────────────────
        g.setColour (juce::Colour (0xff44ff44).withAlpha (0.80f));
        g.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
        juce::String valStr;
        if (slider.getMaximum() > 1.0)
        {
            const double v = slider.getValue();
            valStr = (v >= 0.0 ? "+" : "") + juce::String (v, 1);
        }
        else
        {
            valStr = juce::String ((int) (prop * 100)) + "%";
        }
        g.drawText (valStr,
                    (int) (cx - innerR), (int) (cy - 7),
                    (int) (innerR * 2), 14,
                    juce::Justification::centred);
    }

    // Footswitch button (large circle)
    void drawButtonBackground (juce::Graphics& g, juce::Button& btn,
                                const juce::Colour&, bool, bool isDown) override
    {
        auto b  = btn.getLocalBounds().toFloat().reduced (4.0f);
        float cx = b.getCentreX(), cy = b.getCentreY();
        float r  = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;

        // Drop shadow
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillEllipse (cx - r + 2, cy - r + 3, r * 2, r * 2);

        // Outer metal ring
        g.setColour (juce::Colour (isDown ? 0xff1a1a1au : 0xff3d3d3du));
        g.fillEllipse (cx - r, cy - r, r * 2, r * 2);

        // Inner body
        const bool lit = btn.getToggleState();
        g.setColour (juce::Colour (lit ? 0xff0d2b0du : 0xff1c1c1cu));
        g.fillEllipse (cx - r * 0.82f, cy - r * 0.82f, r * 1.64f, r * 1.64f);

        // Ring highlight
        g.setColour (juce::Colour (0xff555555));
        g.drawEllipse (cx - r, cy - r, r * 2, r * 2, 1.5f);

        // LED dot
        g.setColour (juce::Colour (lit ? 0xff44ff44u : 0xff0a2a0au));
        g.fillEllipse (cx - 5, cy - r * 0.65f, 10, 10);
    }

    // Suppress default button text (we draw it in paint())
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override {}
};

//==============================================================================
// PluginEditor
//==============================================================================

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p),
      laf (std::make_unique<FreecoderLookAndFeel>()),
      presetManager (p.getPresetManager())
{
    // ── Preset strip ────────────────────────────────────────────────────────
    for (auto* btn : { &prevPresetButton, &nextPresetButton, &savePresetButton })
    {
        btn->setColour (juce::TextButton::buttonColourId,  juce::Colour (0xff0a1a0a));
        btn->setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff1a5a1a));
        btn->setColour (juce::TextButton::textColourOffId, juce::Colour (0xff44ff44));
        btn->setColour (juce::TextButton::textColourOnId,  juce::Colour (0xff44ff44));
        addAndMakeVisible (btn);
    }
    prevPresetButton.onClick = [this] { presetManager.previousPreset(); repaint(); };
    nextPresetButton.onClick = [this] { presetManager.nextPreset();     repaint(); };
    savePresetButton.onClick = [this] { presetManager.promptSavePreset (this);     };
    for (auto* s : { &morphSlider, &drywetSlider })
    {
        s->setLookAndFeel (laf.get());
        s->onValueChange = [this] { repaint(); };
        addAndMakeVisible (s);
    }

    for (auto* s : { &grainSlider, &scatterSlider, &formantSlider, &pitchSlider })
    {
        s->setLookAndFeel (laf.get());
        addAndMakeVisible (s);
    }
    pitchSlider.setRange (-12.0, 12.0, 0.1);  // semitones, overrides attachment default display

    recButton.setLookAndFeel (laf.get());
    recButton.setClickingTogglesState (true);
    addAndMakeVisible (recButton);

    engageButton.setLookAndFeel (laf.get());
    engageButton.setClickingTogglesState (true);
    addAndMakeVisible (engageButton);

    reverseButton.setClickingTogglesState (true);
    reverseButton.setColour (juce::TextButton::buttonColourId,  juce::Colour (0xff0a1a0a));
    reverseButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff1a5a1a));
    reverseButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff555555));
    reverseButton.setColour (juce::TextButton::textColourOnId,  juce::Colour (0xff44ff44));
    addAndMakeVisible (reverseButton);

    inspectButton.setColour (juce::TextButton::buttonColourId,  juce::Colour (0xff111111));
    inspectButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff2a2a2a));
    inspectButton.onClick = [&] {
        if (!inspector)
        {
            inspector = std::make_unique<melatonin::Inspector> (*this);
            inspector->onClose = [this]() { inspector.reset(); };
        }
        inspector->setVisible (true);
    };
    addAndMakeVisible (inspectButton);

    // ── MIDI mode controls ──────────────────────────────────────────────────
    modeButton.setClickingTogglesState (true);
    modeButton.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff0a1a0a));
    modeButton.setColour (juce::TextButton::buttonOnColourId,  juce::Colour (0xff0a2a2a));
    modeButton.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xff44ff44));
    modeButton.setColour (juce::TextButton::textColourOnId,   juce::Colour (0xff44ffff));
    addAndMakeVisible (modeButton);

    rootNoteSlider.setRange (0.0, 127.0, 1.0);
    rootNoteSlider.setVisible (false);   // hidden — driven via APVTS, shown as text in paint()
    addAndMakeVisible (rootNoteSlider);

    startTimerHz (15);
    setSize (540, 560);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
    for (auto* s : { &morphSlider, &drywetSlider, &grainSlider, &scatterSlider, &formantSlider, &pitchSlider })
        s->setLookAndFeel (nullptr);
    recButton.setLookAndFeel (nullptr);
    engageButton.setLookAndFeel (nullptr);
}

void PluginEditor::timerCallback()
{
    const float newLevel = processorRef.getDonorFillLevel();
    if (std::abs (newLevel - donorFillLevel) > 0.001f)
        donorFillLevel = newLevel;

    // Fetch latest spectrum (always repaint display for live visualizer)
    processorRef.getSpectrumSnapshot (spectrumSnapshot);
    repaint (displayBounds);

    // Repaint footswitch area when engage state changes so the label colour updates
    repaint (engageButton.getBounds().expanded (0, 40));
}

//==============================================================================
static juce::String midiNoteToName (int note)
{
    // Standard MIDI: note 60 = C4 (middle C)
    static const char* names[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    return juce::String (names[note % 12]) + juce::String (note / 12 - 1);
}

static void drawDots (juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour (juce::Colour (0xff1b1b1b));
    for (int dy = area.getY() + 6; dy < area.getBottom(); dy += 12)
        for (int dx = area.getX() + 6; dx < area.getRight(); dx += 12)
            g.fillEllipse ((float) dx - 1.5f, (float) dy - 1.5f, 3.0f, 3.0f);
}


void PluginEditor::paint (juce::Graphics& g)
{
    const int W = getWidth(), H = getHeight();

    // ── Background (vertical gradient) ──────────────────────────────────────
    {
        juce::ColourGradient bg (juce::Colour (0xff131313), 0.0f, 0.0f,
                                  juce::Colour (0xff080808), 0.0f, (float) H, false);
        g.setGradientFill (bg);
        g.fillAll();
    }
    drawDots (g, { 0, 104, W, H - 104 });

    // ── Header panel ────────────────────────────────────────────────────────
    {
        juce::ColourGradient hdr (juce::Colour (0xff1e1e1e), 0.0f, 0.0f,
                                   juce::Colour (0xff111111), 0.0f, 68.0f, false);
        g.setGradientFill (hdr);
        g.fillRect (0, 0, W, 68);
    }
    // Thin green separator
    g.setColour (juce::Colour (0xff44ff44).withAlpha (0.45f));
    g.drawLine (0.0f, 67.0f, (float) W, 67.0f, 1.0f);
    // Subtle inner highlight at top
    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.drawLine (0.0f, 1.0f, (float) W, 1.0f, 1.0f);

    // Green indicator LED
    {
        const float lx = 42.0f, ly = 14.0f, lr = 7.0f;
        // Glow
        g.setColour (juce::Colour (0xff44ff44).withAlpha (0.18f));
        g.fillEllipse (lx - lr - 3, ly - lr - 3, (lr + 3) * 2, (lr + 3) * 2);
        // Body
        juce::ColourGradient led (juce::Colour (0xff88ff88), lx - lr * 0.3f, ly - lr * 0.3f,
                                   juce::Colour (0xff22cc22), lx + lr * 0.3f, ly + lr * 0.3f, true);
        g.setGradientFill (led);
        g.fillEllipse (lx - lr, ly - lr, lr * 2, lr * 2);
    }

    // Wordmark — subtle green glow layer then white text
    g.setColour (juce::Colour (0xff44ff44).withAlpha (0.07f));
    g.setFont (juce::FontOptions (30.0f).withStyle ("Bold"));
    g.drawText ("FREECODER", 60, 6, 310, 36, juce::Justification::centredLeft);

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (26.0f).withStyle ("Bold"));
    g.drawText ("FREECODER", 62, 8, 308, 32, juce::Justification::centredLeft);

    // Subtitle
    g.setColour (juce::Colour (0xff44ff44).withAlpha (0.85f));
    g.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
    g.drawText ("SPECTRAL  MORPHING  WORKSTATION", 62, 41, 350, 13, juce::Justification::centredLeft);

    // Branding (top-right)
    g.setColour (juce::Colour (0xff333333));
    g.setFont (juce::FontOptions (7.5f));
    g.drawText ("AMENT  |  AUDIO", W - 112, 52, 104, 12, juce::Justification::centredRight);

    // ── Preset strip ────────────────────────────────────────────────────────
    g.setColour (juce::Colour (0xff44ff44).withAlpha (0.15f));
    g.fillRect (0, 68, W, 36);
    g.setColour (juce::Colour (0xff44ff44).withAlpha (0.20f));
    g.drawLine (0, 104, (float) W, 104, 1.0f);

    g.setColour (juce::Colour (0xff44ff44));
    g.setFont (juce::FontOptions (12.0f).withStyle ("Bold"));
    g.drawText (presetManager.getCurrentPresetName(), 52, 72, 362, 26, juce::Justification::centred);

    // ── Slider section ──────────────────────────────────────────────────────
    auto drawSliderLabels = [&] (int lx, int ly, const juce::String& name)
    {
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
        g.drawText (name, lx, ly, 100, 14, juce::Justification::centredLeft);
    };

    drawSliderLabels (20,      108, "morph");
    drawSliderLabels (W - 120, 108, "dry/wet");

    // Value readouts
    g.setColour (juce::Colour (0xff44ff44));
    g.setFont (juce::FontOptions (8.5f));
    g.drawText (juce::String (morphSlider.getValue(),  2), 20,      161, 80, 12, juce::Justification::centredLeft);
    g.drawText (juce::String (drywetSlider.getValue(), 2), W - 120, 161, 80, 12, juce::Justification::centredRight);

    // Morph endpoint labels: PHRASE (0) on left, SPECTRAL (1) on right
    g.setColour (juce::Colour (0xff333333));
    g.setFont (juce::FontOptions (7.0f));
    g.drawText ("phrase",   20,      175, 80,  10, juce::Justification::centredLeft);
    g.drawText ("spectral", 20,      175, 218, 10, juce::Justification::centredRight);

    // ── Pad labels ──────────────────────────────────────────────────────────
    const int lpad1Y = grainSlider.getY()   + grainSlider.getHeight()   / 2 - 10;
    const int lpad2Y = scatterSlider.getY() + scatterSlider.getHeight() / 2 - 10;
    const int rpad1Y = formantSlider.getY() + formantSlider.getHeight() / 2 - 10;

    // Left pad labels (to the left of the pads)
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
    g.drawText ("grain",   2, lpad1Y, grainSlider.getX() - 4, 20, juce::Justification::centredRight);
    g.drawText ("scatter", 2, lpad2Y, scatterSlider.getX() - 4, 20, juce::Justification::centredRight);

    // Right pad labels (to the right of the pads)
    const int rpRight = formantSlider.getRight();
    g.drawText ("formant", rpRight + 4, rpad1Y, W - rpRight - 6, 20, juce::Justification::centredLeft);

    const int rpad2Y = pitchSlider.getY() + pitchSlider.getHeight() / 2 - 10;
    g.drawText ("pitch",   rpRight + 4, rpad2Y, W - rpRight - 6, 20, juce::Justification::centredLeft);

    // ── Centre display ───────────────────────────────────────────────────────
    g.setColour (juce::Colour (0xff060606));
    g.fillRoundedRectangle (displayBounds.toFloat(), 8.0f);
    g.setColour (juce::Colour (0xff1a441a));
    g.drawRoundedRectangle (displayBounds.toFloat(), 8.0f, 1.5f);

    // "DONOR" label
    g.setColour (juce::Colour (0xff1f5e1f));
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("DONOR",
                displayBounds.getX(), displayBounds.getY() + 6,
                displayBounds.getWidth(), 14, juce::Justification::centred);

    // ── Spectrum visualizer ─────────────────────────────────────────────────
    {
        auto specArea = displayBounds.reduced (8, 8).withTrimmedTop (16).withTrimmedBottom (20);
        const float bx    = (float) specArea.getX();
        const float by    = (float) specArea.getY();
        const float bw    = (float) specArea.getWidth();
        const float bh    = (float) specArea.getHeight();
        const float botY  = by + bh;
        const int   nBins = SpectralEngine::kVisBins;

        // Helper: magnitude (0–1 linear) → display height using dB scale (-60 to 0 dBFS)
        auto magToY = [&] (float mag) -> float
        {
            if (mag < 1e-6f) return botY;
            const float db   = 20.0f * std::log10 (mag);
            const float norm = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
            return botY - norm * bh;
        };

        if (spectrumSnapshot.hasData)
        {
            // ── Donor spectrum (filled green) ──────────────────────────────
            juce::Path donorPath;
            donorPath.startNewSubPath (bx, botY);
            for (int i = 0; i < nBins; ++i)
            {
                const float px = bx + (float) i / (float) nBins * bw;
                donorPath.lineTo (px, magToY (spectrumSnapshot.donorMag[(size_t) i]));
            }
            donorPath.lineTo (bx + bw, botY);
            donorPath.closeSubPath();

            g.setColour (juce::Colour (0xff163d16));
            g.fillPath (donorPath);
            g.setColour (juce::Colour (0xff44ff44).withAlpha (0.75f));
            g.strokePath (donorPath, juce::PathStrokeType (1.0f));

            // ── Live input spectrum (white outline) ────────────────────────
            juce::Path inputPath;
            inputPath.startNewSubPath (bx, magToY (spectrumSnapshot.inputMag[0]));
            for (int i = 1; i < nBins; ++i)
            {
                const float px = bx + (float) i / (float) nBins * bw;
                inputPath.lineTo (px, magToY (spectrumSnapshot.inputMag[(size_t) i]));
            }
            g.setColour (juce::Colours::white.withAlpha (0.40f));
            g.strokePath (inputPath, juce::PathStrokeType (1.5f));
        }
        else
        {
            // No data yet
            g.setColour (juce::Colour (0xff1a3d1a));
            g.setFont (juce::FontOptions (28.0f).withStyle ("Bold"));
            g.drawText ("--", displayBounds.reduced (8, 16).toFloat(), juce::Justification::centred);
        }

        // Recording indicator / fill % (top-right corner of display)
        const bool isRecording = processorRef.apvts.getRawParameterValue ("recTrigger")->load() > 0.5f;
        if (isRecording || donorFillLevel > 0.001f)
        {
            g.setColour (isRecording ? juce::Colour (0xffff4040) : juce::Colour (0xff44ff44));
            g.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
            g.drawText (juce::String ((int) (donorFillLevel * 100)) + "%",
                        specArea.getRight() - 36, specArea.getY(), 34, 14,
                        juce::Justification::centredRight);
        }
    }

    // Fill bar at bottom of display
    auto barArea = displayBounds.withTrimmedTop (displayBounds.getHeight() - 18).reduced (10, 4);
    g.setColour (juce::Colour (0xff0a2a0a));
    g.fillRoundedRectangle (barArea.toFloat(), 3.0f);
    if (donorFillLevel > 0.001f)
    {
        auto filled = barArea.withWidth ((int) (barArea.getWidth() * donorFillLevel));
        g.setColour (donorFillLevel >= 1.0f ? juce::Colours::red : juce::Colour (0xff44ff44));
        g.fillRoundedRectangle (filled.toFloat(), 3.0f);
    }

    // ── Footswitch labels ─────────────────────────────────────────────────────
    const int recCx  = recButton.getBounds().getCentreX();
    const int engCx  = engageButton.getBounds().getCentreX();
    const int recBot = recButton.getBottom();
    const int engBot = engageButton.getBottom();

    // REC label + sublabel
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (13.0f).withStyle ("Bold"));
    g.drawText ("REC",    recCx - 50, recBot + 4,  100, 18, juce::Justification::centred);
    g.setColour (juce::Colour (0xff555555));
    g.setFont (juce::FontOptions (7.5f));
    g.drawText ("CLICK TO RECORD", recCx - 55, recBot + 22, 110, 12, juce::Justification::centred);

    // ENGAGE label + sublabel
    const bool isEngaged = processorRef.apvts.getRawParameterValue ("engage")->load() > 0.5f;
    g.setColour (isEngaged ? juce::Colour (0xff44ff44) : juce::Colours::white);
    g.setFont (juce::FontOptions (13.0f).withStyle ("Bold"));
    g.drawText ("ENGAGE",    engCx - 55, engBot + 4,  110, 18, juce::Justification::centred);
    g.setColour (juce::Colour (0xff555555));
    g.setFont (juce::FontOptions (7.5f));
    g.drawText ("FREEZE SPECTRUM", engCx - 60, engBot + 22, 120, 12, juce::Justification::centred);


    // ── MIDI utility row ────────────────────────────────────────────────────────
    {
        const bool isMidi = processorRef.apvts.getRawParameterValue ("midiMode")->load() > 0.5f;
        // modeButton label updates to reflect current mode
        const_cast<PluginEditor*>(this)->modeButton.setButtonText (isMidi ? "MIDI MODE" : "EFFECT MODE");

        if (isMidi)
        {
            // Root note display: right of the mode button
            const int root = (int) processorRef.apvts.getRawParameterValue ("rootNote")->load();
            g.setColour (juce::Colour (0xff44ffff));
            g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
            g.drawText ("ROOT: " + midiNoteToName (root),
                        W / 2 + 66, 498, 120, 22, juce::Justification::centredLeft);

            // Hint text
            g.setColour (juce::Colour (0xff2a4a4a));
            g.setFont (juce::FontOptions (7.5f));
            g.drawText ("AUTOMATE ROOT NOTE TO CHANGE KEY",
                        0, 522, W, 12, juce::Justification::centred);
        }
    }

    // ── Bottom branding ────────────────────────────────────────────────────────
    g.setColour (juce::Colour (0xff2a2a2a));
    g.setFont (juce::FontOptions (8.0f));
    g.drawText ("A M E N T  A U D I O  |  F R E E C O D E R  v 0 . 1 . 0", 0, H - 16, W, 14, juce::Justification::centred);
}

void PluginEditor::resized()
{
    const int W = getWidth();

    // ── Preset strip (below header, above sliders) ─────────────────────────────
    prevPresetButton.setBounds (20,      72, 28, 26);
    nextPresetButton.setBounds (418,     72, 28, 26);
    savePresetButton.setBounds (452,     72, 68, 26);
    // preset name drawn in paint() between x=52 and x=414

    // ── Top sliders ────────────────────────────────────────────────────────────
    morphSlider.setBounds  (20,      126, 220, 32);
    drywetSlider.setBounds (W - 240, 126, 220, 32);

    // ── Pads ───────────────────────────────────────────────────────────────────
    const int padW  = 110, padH = 82;
    const int pad1Y = 184, pad2Y = 280;
    const int leftX  = 40;
    const int rightX = W - padW - 40;

    grainSlider.setBounds   (leftX,  pad1Y, padW, padH);
    scatterSlider.setBounds (leftX,  pad2Y, padW, padH);
    formantSlider.setBounds (rightX, pad1Y, padW, padH);
    pitchSlider.setBounds   (rightX, pad2Y, padW, padH);

    // ── Centre display ─────────────────────────────────────────────────────────
    const int dispX = leftX + padW + 14;
    const int dispW = rightX - dispX - 14;
    displayBounds   = { dispX, pad1Y, dispW, pad2Y + padH - pad1Y };

    // ── Footswitches ────────────────────────────────────────────────────────────
    const int swSize = 88;
    const int swY    = 386;
    recButton.setBounds     (W / 4 - swSize / 2,     swY, swSize, swSize);
    engageButton.setBounds  (3 * W / 4 - swSize / 2, swY, swSize, swSize);
    reverseButton.setBounds (W / 2 - 38, swY + swSize / 2 - 11, 76, 22);

    // ── MIDI utility row ───────────────────────────────────────────────────────
    modeButton.setBounds (W / 2 - 60, 498, 120, 22);
    rootNoteSlider.setBounds (0, 0, 1, 1);  // hidden, just needs to exist for attachment

    // ── Inspector (tiny, top-right corner) ────────────────────────────────────
    inspectButton.setBounds (W - 18, 2, 16, 16);
}
