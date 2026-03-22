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

    // Footswitch button — layered rubber dome with bevelled metal ring
    void drawButtonBackground (juce::Graphics& g, juce::Button& btn,
                                const juce::Colour&, bool, bool isDown) override
    {
        auto b  = btn.getLocalBounds().toFloat().reduced (4.0f);
        float cx = b.getCentreX(), cy = b.getCentreY();
        float r  = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
        const bool lit = btn.getToggleState();

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
    engageButton.setButtonText ("FREEZE");
    addAndMakeVisible (engageButton);

    phraseButton.setClickingTogglesState (true);
    phraseButton.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff0a1a0a));
    phraseButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff1a5a1a));
    phraseButton.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xff444444));
    phraseButton.setColour (juce::TextButton::textColourOnId,   juce::Colour (0xff44ff44));
    addAndMakeVisible (phraseButton);

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

    // Latch button
    latchButton.setClickingTogglesState (true);
    latchButton.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff0a1a0a));
    latchButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff0a2a2a));
    latchButton.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xff444444));
    latchButton.setColour (juce::TextButton::textColourOnId,   juce::Colour (0xff44ffff));
    latchButton.setVisible (false);   // shown only in MIDI mode
    addAndMakeVisible (latchButton);

    // Donor slot buttons
    for (int i = 0; i < SpectralEngine::kNumDonorSlots; ++i)
    {
        slotButtons[i]->setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff0a1a0a));
        slotButtons[i]->setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff1a5a1a));
        slotButtons[i]->setColour (juce::TextButton::textColourOffId,  juce::Colour (0xff444444));
        slotButtons[i]->setColour (juce::TextButton::textColourOnId,   juce::Colour (0xff44ff44));
        slotButtons[i]->onClick = [this, i] { processorRef.requestSlot (i); };
        addAndMakeVisible (slotButtons[i]);
    }

    // Export donor to WAV
    exportButton.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff0a0a1a));
    exportButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff1a1a5a));
    exportButton.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xff4466ff));
    exportButton.setColour (juce::TextButton::textColourOnId,   juce::Colour (0xff88aaff));
    exportButton.onClick = [this] { processorRef.exportActiveDonorSlotToWav(); };
    addAndMakeVisible (exportButton);

    // Import donor from file
    importButton.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff0a0a1a));
    importButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff1a1a5a));
    importButton.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xff4466ff));
    importButton.setColour (juce::TextButton::textColourOnId,   juce::Colour (0xff88aaff));
    importButton.onClick = [this] { processorRef.importDonorFromFile(); };
    addAndMakeVisible (importButton);

    // Auto-engage toggle
    autoEngageButton.setClickingTogglesState (true);
    autoEngageButton.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff0a1a0a));
    autoEngageButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff1a5a1a));
    autoEngageButton.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xff444444));
    autoEngageButton.setColour (juce::TextButton::textColourOnId,   juce::Colour (0xff44ff44));
    addAndMakeVisible (autoEngageButton);

    // Effect ADSR toggle
    effectAdsrButton.setClickingTogglesState (true);
    effectAdsrButton.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff0a1a0a));
    effectAdsrButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff1a5a1a));
    effectAdsrButton.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xff444444));
    effectAdsrButton.setColour (juce::TextButton::textColourOnId,   juce::Colour (0xff44ff44));
    addAndMakeVisible (effectAdsrButton);

    // Rec length: stepped 1/2/3/5s, drawn as labelled ticks in paint()
    recLengthSlider.setRange (1.0, 5.0, 1.0);
    recLengthSlider.setColour (juce::Slider::trackColourId,      juce::Colour (0xff0a2a0a));
    recLengthSlider.setColour (juce::Slider::thumbColourId,      juce::Colour (0xff44ff44));
    recLengthSlider.setColour (juce::Slider::backgroundColourId, juce::Colour (0xff080808));
    addAndMakeVisible (recLengthSlider);

    // ADSR knobs — same look as the pad knobs, hidden until MIDI mode is on
    for (auto* s : { &adsrAttackSlider, &adsrDecaySlider, &adsrSustainSlider, &adsrReleaseSlider })
    {
        s->setLookAndFeel (laf.get());
        s->setVisible (false);
        addAndMakeVisible (s);
    }
    adsrAttackSlider.setRange  (0.001, 5.0,  0.001);
    adsrDecaySlider.setRange   (0.001, 5.0,  0.001);
    adsrSustainSlider.setRange (0.0,   1.0,  0.01);
    adsrReleaseSlider.setRange (0.001, 10.0, 0.001);

    startTimerHz (15);
    setSize (540, 600);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
    for (auto* s : { &morphSlider, &drywetSlider, &grainSlider, &scatterSlider, &formantSlider, &pitchSlider })
        s->setLookAndFeel (nullptr);
    for (auto* s : { &adsrAttackSlider, &adsrDecaySlider, &adsrSustainSlider, &adsrReleaseSlider })
        s->setLookAndFeel (nullptr);
    recButton.setLookAndFeel (nullptr);
    engageButton.setLookAndFeel (nullptr);
}

void PluginEditor::timerCallback()
{
    const float newLevel = processorRef.getDonorFillLevel();
    if (std::abs (newLevel - donorFillLevel) > 0.001f)
        donorFillLevel = newLevel;

    // Fetch latest spectrum + tuner result
    processorRef.getSpectrumSnapshot (spectrumSnapshot);
    processorRef.getTunerResult (tunerResult);
    repaint (displayBounds);

    // Repaint footswitch area when engage state changes so the label colour updates
    repaint (engageButton.getBounds().expanded (0, 40));

    // Show/hide mode-specific controls
    const bool isMidi      = processorRef.apvts.getRawParameterValue ("midiMode")->load() > 0.5f;
    const bool effectAdsr  = processorRef.apvts.getRawParameterValue ("effectAdsr")->load() > 0.5f;
    const bool showAdsr    = isMidi || effectAdsr;
    for (auto* s : { &adsrAttackSlider, &adsrDecaySlider, &adsrSustainSlider, &adsrReleaseSlider })
        s->setVisible (showAdsr);
    latchButton.setVisible (isMidi);
    phraseButton.setVisible (!isMidi);
    effectAdsrButton.setVisible (!isMidi);  // ADSR toggle only makes sense in effect mode

    // Update slot button colours: active = bright green, has data = dim, empty = dark
    const int activeSlot = processorRef.getActiveSlot();
    for (int i = 0; i < SpectralEngine::kNumDonorSlots; ++i)
    {
        const bool isActive  = (i == activeSlot);
        const bool hasData   = processorRef.donorSlotHasData (i);
        slotButtons[i]->setColour (juce::TextButton::textColourOffId,
                                   isActive  ? juce::Colour (0xff44ff44) :
                                   hasData   ? juce::Colour (0xff2a7a2a) :
                                               juce::Colour (0xff2a2a2a));
        slotButtons[i]->setColour (juce::TextButton::buttonColourId,
                                   isActive  ? juce::Colour (0xff0d2a0d) :
                                               juce::Colour (0xff0a0a0a));
    }
}

//==============================================================================
static juce::String midiNoteToName (int note)
{
    // Standard MIDI: note 60 = C4 (middle C)
    static const char* names[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    return juce::String (names[note % 12]) + juce::String (note / 12 - 1);
}

// Horizontal brushed-metal scan lines — replaces dot grid
static void drawFaceplateTexture (juce::Graphics& g, juce::Rectangle<int> area)
{
    for (int y = area.getY(); y < area.getBottom(); y += 3)
    {
        const float a = (y % 9 == 0) ? 0.022f : 0.009f;
        g.setColour (juce::Colours::white.withAlpha (a));
        g.drawHorizontalLine (y, (float) area.getX(), (float) area.getRight());
    }
}

// Phillips-head screw — decorative corner element
static void drawScrew (juce::Graphics& g, float cx, float cy, float r)
{
    // Shadow
    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.fillEllipse (cx - r + 1.0f, cy - r + 1.5f, r * 2.0f, r * 2.0f);

    // Outer rim (bevelled)
    {
        juce::ColourGradient rim (
            juce::Colour (0xff4a4a4a), cx - r * 0.4f, cy - r * 0.7f,
            juce::Colour (0xff191919), cx + r * 0.3f, cy + r * 0.6f, false);
        g.setGradientFill (rim);
        g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
    }

    // Inner face
    {
        juce::ColourGradient face (
            juce::Colour (0xff2e2e2e), cx - r * 0.1f, cy - r * 0.6f,
            juce::Colour (0xff0e0e0e), cx + r * 0.1f, cy + r * 0.5f, false);
        g.setGradientFill (face);
        const float ir = r * 0.70f;
        g.fillEllipse (cx - ir, cy - ir, ir * 2.0f, ir * 2.0f);
    }

    // Phillips cross slots
    g.setColour (juce::Colour (0xff484848));
    const float sl = r * 0.46f, sw = r * 0.11f;
    g.fillRect (cx - sw, cy - sl, sw * 2.0f, sl * 2.0f);
    g.fillRect (cx - sl, cy - sw, sl * 2.0f, sw * 2.0f);

    // Specular dot
    g.setColour (juce::Colours::white.withAlpha (0.17f));
    g.fillEllipse (cx - r * 0.33f, cy - r * 0.62f, r * 0.36f, r * 0.26f);
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
    drawFaceplateTexture (g, { 0, 104, W, H - 104 });

    // Corner screws (4 corners — classic pedal enclosure look)
    const float screwR  = 7.0f;
    const float screwM  = 11.0f;
    drawScrew (g, screwM,       screwM,       screwR);
    drawScrew (g, (float)W - screwM, screwM,       screwR);
    drawScrew (g, screwM,       (float)H - screwM, screwR);
    drawScrew (g, (float)W - screwM, (float)H - screwM, screwR);

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

    // ── Centre display (sunken LCD panel look) ───────────────────────────────
    // Outer shadow bevel
    g.setColour (juce::Colours::black.withAlpha (0.60f));
    g.fillRoundedRectangle (displayBounds.expanded (2).toFloat(), 9.0f);
    // Panel body
    {
        juce::ColourGradient panel (
            juce::Colour (0xff050a05), (float) displayBounds.getX(), (float) displayBounds.getY(),
            juce::Colour (0xff030703), (float) displayBounds.getX(), (float) displayBounds.getBottom(), false);
        g.setGradientFill (panel);
        g.fillRoundedRectangle (displayBounds.toFloat(), 8.0f);
    }
    // Inner highlight (top edge — light catches the inset lip)
    g.setColour (juce::Colour (0xff44ff44).withAlpha (0.18f));
    g.drawRoundedRectangle (displayBounds.toFloat().reduced (0.5f), 8.0f, 1.0f);
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

    // Rec length labels: 1s / 2s / 3s / 5s ticks above slider
    {
        const int sliderX = recCx - 52;
        const int sliderW = 104;
        const int labelY  = recLengthSlider.getY() - 13;
        // 4 stops at positions 1,2,3,5 mapped into [1,5] range → normalized 0,0.25,0.5,1.0
        static const float stops[]  = { 0.0f, 0.25f, 0.5f, 1.0f };
        static const char* labels[] = { "1s", "2s", "3s", "5s" };
        g.setFont (juce::FontOptions (7.5f));
        for (int t = 0; t < 4; ++t)
        {
            const int tx = sliderX + (int) (stops[t] * sliderW);
            g.setColour (juce::Colour (0xff444444));
            g.fillRect (tx, recLengthSlider.getY() - 4, 1, 4);
            g.setColour (juce::Colour (0xff555555));
            g.drawText (labels[t], tx - 8, labelY, 16, 12, juce::Justification::centred);
        }

        // Current value highlight
        const float recSecs = (float) recLengthSlider.getValue();
        const juce::String recLabel = juce::String ((int) recSecs) + "s";
        g.setColour (juce::Colour (0xff44ff44));
        g.setFont (juce::FontOptions (8.5f).withStyle ("Bold"));
        g.drawText (recLabel, recCx - 20, recLengthSlider.getBottom() + 2, 40, 12, juce::Justification::centred);

        // AUTO button label
        g.setColour (juce::Colour (0xff555555));
        g.setFont (juce::FontOptions (7.0f));
        g.drawText ("AUTO ENGAGE", recCx - 40, autoEngageButton.getBottom() + 2, 80, 10, juce::Justification::centred);

        // LATCH label
        const bool isLatched = processorRef.apvts.getRawParameterValue ("latch")->load() > 0.5f;
        g.setColour (isLatched ? juce::Colour (0xff44ffff) : juce::Colour (0xff555555));
        g.setFont (juce::FontOptions (7.0f));
        g.drawText ("HOLD NOTES", latchButton.getX() - 4, latchButton.getBottom() + 2,
                    latchButton.getWidth() + 8, 10, juce::Justification::centred);
    }

    // FREEZE label + sublabel
    const bool isEngaged = processorRef.apvts.getRawParameterValue ("engage")->load() > 0.5f;
    g.setColour (isEngaged ? juce::Colour (0xff44ff44) : juce::Colours::white);
    g.setFont (juce::FontOptions (13.0f).withStyle ("Bold"));
    g.drawText ("FREEZE",    engCx - 55, engBot + 4,  110, 18, juce::Justification::centred);
    g.setColour (juce::Colour (0xff555555));
    g.setFont (juce::FontOptions (7.5f));
    g.drawText ("SPECTRAL FREEZE", engCx - 60, engBot + 22, 120, 12, juce::Justification::centred);

    // PHRASE button label (effect mode only)
    {
        const bool isMidi2    = processorRef.apvts.getRawParameterValue ("midiMode")->load() > 0.5f;
        const bool isPhrased  = processorRef.apvts.getRawParameterValue ("phraseEngage")->load() > 0.5f;
        if (!isMidi2)
        {
            g.setColour (isPhrased ? juce::Colour (0xff44ff44) : juce::Colour (0xff555555));
            g.setFont (juce::FontOptions (7.0f));
            g.drawText ("PHRASE LOOP",
                        phraseButton.getX() - 4, phraseButton.getBottom() + 2,
                        phraseButton.getWidth() + 8, 10, juce::Justification::centred);
        }
    }


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

            // Tuner: live pitch of input signal (left of the mode button)
            if (tunerResult.hasData)
            {
                const float cents = tunerResult.centsOffset;
                // Colour: green if close to a note, amber if slightly off, red if far
                const juce::Colour tunerCol = std::abs (cents) < 10.0f ? juce::Colour (0xff44ff44)
                                            : std::abs (cents) < 25.0f ? juce::Colour (0xffffaa00)
                                                                        : juce::Colour (0xffff4444);
                g.setColour (tunerCol);
                g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
                const juce::String centsStr = (cents >= 0.0f ? "+" : "") + juce::String ((int) cents) + "c";
                g.drawText (midiNoteToName (tunerResult.midiNote) + " " + centsStr,
                            10, 498, W / 2 - 70, 22, juce::Justification::centredRight);
                g.setColour (juce::Colour (0xff333333));
                g.setFont (juce::FontOptions (7.5f));
                g.drawText ("INPUT", 10, 510, W / 2 - 70, 10, juce::Justification::centredRight);
            }

            // ADSR labels below each knob (MIDI mode)
            g.setColour (juce::Colours::white);
            g.setFont (juce::FontOptions (8.5f).withStyle ("Bold"));
            const int kw = 44, gap = 16;
            const int rowX = (W - 4 * kw - 3 * gap) / 2;
            const int labelY = 526 + kw + 3;
            static const char* labels[] = { "ATK", "DEC", "SUS", "REL" };
            for (int i = 0; i < 4; ++i)
                g.drawText (labels[i], rowX + i * (kw + gap), labelY, kw, 12,
                            juce::Justification::centred);
        }
        else
        {
            // Effect mode — show ADSR labels if effectAdsr is on
            const bool showEffAdsr = processorRef.apvts.getRawParameterValue ("effectAdsr")->load() > 0.5f;
            if (showEffAdsr)
            {
                g.setColour (juce::Colours::white);
                g.setFont (juce::FontOptions (8.5f).withStyle ("Bold"));
                const int kw = 44, gap = 16;
                const int rowX = (W - 4 * kw - 3 * gap) / 2;
                const int labelY = 526 + kw + 3;
                static const char* effLabels[] = { "ATK", "DEC", "SUS", "REL" };
                for (int i = 0; i < 4; ++i)
                    g.drawText (effLabels[i], rowX + i * (kw + gap), labelY, kw, 12,
                                juce::Justification::centred);
            }

            // effectAdsrButton label
            g.setColour (showEffAdsr ? juce::Colour (0xff44ff44) : juce::Colour (0xff555555));
            g.setFont (juce::FontOptions (7.0f));
            g.drawText ("ENV SHAPE", effectAdsrButton.getX() - 6, effectAdsrButton.getBottom() + 2,
                        effectAdsrButton.getWidth() + 12, 10, juce::Justification::centred);
        }
    }

    // ── Bottom branding + diagnostics ──────────────────────────────────────────
    g.setColour (juce::Colour (0xff2a2a2a));
    g.setFont (juce::FontOptions (8.0f));
    g.drawText ("A M E N T  A U D I O  |  F R E E C O D E R  v 0 . 2 . 0", 0, H - 16, W - 120, 14, juce::Justification::centred);

    // Diagnostic readout: input channels | block size | block count
    g.setColour (juce::Colour (0xff333333));
    g.drawText ("CH:" + juce::String (processorRef.diagInputChannels.load())
                + " BS:" + juce::String (processorRef.diagBlockSize.load())
                + " #" + juce::String (processorRef.diagBlockCount.load() % 10000),
                W - 118, H - 16, 116, 14, juce::Justification::centredRight);
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

    // ── Donor slot buttons (A/B/C) + export below the display ──────────────────
    {
        const int btnW  = 36, btnH = 16, gap = 8;
        const int totalW = 3 * btnW + 2 * gap;
        const int startX = displayBounds.getCentreX() - totalW / 2;
        const int btnY   = displayBounds.getBottom() + 4;
        for (int i = 0; i < SpectralEngine::kNumDonorSlots; ++i)
            slotButtons[i]->setBounds (startX + i * (btnW + gap), btnY, btnW, btnH);
        // Import to the left of slot A, export to the right of slot C
        importButton.setBounds (startX - (btnW + gap), btnY, btnW, btnH);
        exportButton.setBounds (startX + 3 * (btnW + gap), btnY, btnW, btnH);
    }

    // ── Footswitches ────────────────────────────────────────────────────────────
    const int swSize = 88;
    const int swY    = 386;
    recButton.setBounds     (W / 4 - swSize / 2,     swY, swSize, swSize);
    engageButton.setBounds  (3 * W / 4 - swSize / 2, swY, swSize, swSize);
    reverseButton.setBounds (W / 2 - 38, swY + swSize / 2 - 11, 76, 22);
    phraseButton.setBounds  (W / 2 - 28, swY + swSize / 2 + 14, 56, 16);
    latchButton.setBounds   (W / 2 - 28, swY + swSize / 2 + 14, 56, 16);

    // ── Rec length + auto-engage (below REC button) ────────────────────────────
    {
        const int recCx = W / 4;
        recLengthSlider.setBounds  (recCx - 52, swY + swSize + 36, 104, 16);
        autoEngageButton.setBounds (recCx - 24, swY + swSize + 57, 48, 16);
        // ADSR toggle sits next to the FREEZE button (effect mode)
        effectAdsrButton.setBounds (3 * W / 4 - 24, swY + swSize + 36, 48, 16);
    }

    // ── MIDI utility row ───────────────────────────────────────────────────────
    modeButton.setBounds (W / 2 - 60, 498, 120, 22);
    rootNoteSlider.setBounds (0, 0, 1, 1);  // hidden, just needs to exist for attachment

    // ── ADSR row (visible in MIDI mode, sits below modeButton) ─────────────────
    // 4 knobs × 44px + 3 gaps × 16px = 224px, centred
    {
        const int kw = 44, gap = 16;
        const int rowX = (W - 4 * kw - 3 * gap) / 2;
        const int rowY = 526;
        adsrAttackSlider .setBounds (rowX + 0 * (kw + gap), rowY, kw, kw);
        adsrDecaySlider  .setBounds (rowX + 1 * (kw + gap), rowY, kw, kw);
        adsrSustainSlider.setBounds (rowX + 2 * (kw + gap), rowY, kw, kw);
        adsrReleaseSlider.setBounds (rowX + 3 * (kw + gap), rowY, kw, kw);
    }

    // ── Inspector (tiny, top-right corner) ────────────────────────────────────
    inspectButton.setBounds (W - 18, 2, 16, 16);
}
