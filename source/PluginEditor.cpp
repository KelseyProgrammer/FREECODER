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
    prevPresetButton.onClick = [this] { presetManager.previousPreset(); isDirty = false; repaint(); };
    nextPresetButton.onClick = [this] { presetManager.nextPreset();     isDirty = false; repaint(); };
    savePresetButton.onClick = [this] { presetManager.promptSavePreset (this); isDirty = false; };
    for (auto* s : { &morphSlider, &drywetSlider })
    {
        s->setLookAndFeel (laf.get());
        s->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s->onValueChange = [this] { repaint(); };
        addAndMakeVisible (s);
    }

    for (auto* s : { &grainSlider, &scatterSlider, &formantSlider, &pitchSlider })
    {
        s->setLookAndFeel (laf.get());
        s->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible (s);
    }
    pitchSlider.setRange (-12.0, 12.0, 0.1);  // semitones, overrides attachment default display

    addAndMakeVisible (recButton);
    addAndMakeVisible (reverseButton);
    addAndMakeVisible (phraseButton);
    addAndMakeVisible (engageButton);

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
    rootNoteSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    rootNoteSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    rootNoteSlider.setColour (juce::Slider::trackColourId,      juce::Colour (0xff0a2a2a));
    rootNoteSlider.setColour (juce::Slider::thumbColourId,      juce::Colour (0xff44ffff));
    rootNoteSlider.setColour (juce::Slider::backgroundColourId, juce::Colour (0xff080808));
    rootNoteSlider.setVisible (false);   // shown only in MIDI mode — timerCallback toggles
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

    // Effect ADSR toggle
    effectAdsrButton.setClickingTogglesState (true);
    effectAdsrButton.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff0a1a0a));
    effectAdsrButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff1a5a1a));
    effectAdsrButton.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xff444444));
    effectAdsrButton.setColour (juce::TextButton::textColourOnId,   juce::Colour (0xff44ff44));
    addAndMakeVisible (effectAdsrButton);

    // Rec length: stepped 1/2/3/5s, drawn as labelled ticks in paint()
    recLengthSlider.setRange (1.0, 5.0, 1.0);
    recLengthSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    recLengthSlider.setColour (juce::Slider::trackColourId,      juce::Colour (0xff0a2a0a));
    recLengthSlider.setColour (juce::Slider::thumbColourId,      juce::Colour (0xff44ff44));
    recLengthSlider.setColour (juce::Slider::backgroundColourId, juce::Colour (0xff080808));
    addAndMakeVisible (recLengthSlider);

    // ADSR knobs — same look as the pad knobs, hidden until MIDI mode is on
    for (auto* s : { &adsrAttackSlider, &adsrDecaySlider, &adsrSustainSlider, &adsrReleaseSlider })
    {
        s->setLookAndFeel (laf.get());
        s->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s->setVisible (false);
        addAndMakeVisible (s);
    }
    adsrAttackSlider.setRange  (0.001, 5.0,  0.001);
    adsrDecaySlider.setRange   (0.001, 5.0,  0.001);
    adsrSustainSlider.setRange (0.0,   1.0,  0.01);
    adsrReleaseSlider.setRange (0.001, 10.0, 0.001);

    // Load logo SVG from binary data
    {
        auto xml = juce::XmlDocument::parse (
            juce::String::fromUTF8 (BinaryData::freecoder_logo_svg,
                                    BinaryData::freecoder_logo_svgSize));
        if (xml != nullptr)
            logoDrawable = juce::Drawable::createFromSVG (*xml);
    }

    processorRef.apvts.state.addListener (this);
    startTimerHz (15);
    setResizable (true, true);
    setResizeLimits (420, 480, 900, 1000);
    setSize (processorRef.savedEditorWidth, processorRef.savedEditorHeight);
}

PluginEditor::~PluginEditor()
{
    processorRef.apvts.state.removeListener (this);
    stopTimer();
    for (auto* s : { &morphSlider, &drywetSlider, &grainSlider, &scatterSlider, &formantSlider, &pitchSlider })
        s->setLookAndFeel (nullptr);
    for (auto* s : { &adsrAttackSlider, &adsrDecaySlider, &adsrSustainSlider, &adsrReleaseSlider })
        s->setLookAndFeel (nullptr);
}

void PluginEditor::valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&)
{
    isDirty = true;
}

void PluginEditor::timerCallback()
{
    const float newLevel = processorRef.getDonorFillLevel();
    if (std::abs (newLevel - donorFillLevel) > 0.001f)
        donorFillLevel = newLevel;

    // Fetch latest spectrum + tuner + waveform
    processorRef.getSpectrumSnapshot (spectrumSnapshot);
    processorRef.getTunerResult (tunerResult);
    processorRef.getWaveformSnapshot (waveformSnapshot);
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
    effectAdsrButton.setVisible (!isMidi);  // ADSR toggle only makes sense in effect mode
    phraseButton.setEnabled (!isMidi);      // PHRASE loop doesn't apply in MIDI mode — voices handle playback
    rootNoteSlider.setVisible (isMidi);

    // Update slot button colours: active = bright green, has data = dim, empty = dark
    const int activeSlot = processorRef.getActiveSlot();
    for (int i = 0; i < SpectralEngine::kNumDonorSlots; ++i)
    {
        const bool isActive  = (i == activeSlot);
        const bool hasData   = processorRef.donorSlotHasData (i);
        slotButtons[i]->setColour (juce::TextButton::textColourOffId,
                                   isActive  ? juce::Colour (0xff44ff44) :
                                   hasData   ? juce::Colour (0xff00cc00) :
                                               juce::Colour (0xff666666));
        slotButtons[i]->setColour (juce::TextButton::buttonColourId,
                                   isActive  ? juce::Colour (0xff0d2a0d) :
                                   hasData   ? juce::Colour (0xff081408) :
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
    const int headerH  = juce::roundToInt (H * 0.1133f);
    const int presetH  = juce::roundToInt (H * 0.060f);
    const int contentY = headerH + presetH;
    const float fS = juce::jlimit (0.85f, 1.4f, (float) W / 540.0f);

    // ── Background (vertical gradient) ──────────────────────────────────────
    {
        juce::ColourGradient bg (juce::Colour (0xff131313), 0.0f, 0.0f,
                                  juce::Colour (0xff080808), 0.0f, (float) H, false);
        g.setGradientFill (bg);
        g.fillAll();
    }
    drawFaceplateTexture (g, { 0, contentY, W, H - contentY });

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
        g.fillRect (0, 0, W, headerH);
    }
    // Thin green separator
    g.setColour (juce::Colour (0xff44ff44).withAlpha (0.45f));
    g.drawLine (0.0f, (float)(headerH - 1), (float) W, (float)(headerH - 1), 1.0f);
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

    // Wordmark — SVG logo (falls back to text if not loaded)
    if (logoDrawable != nullptr)
    {
        const int logoX = 52, logoY = 4;
        const int logoW = juce::roundToInt (W * 0.62f);
        const int logoH = juce::roundToInt (logoW * 80.0f / 400.0f);  // preserve 400:80 aspect
        logoDrawable->drawWithin (g, juce::Rectangle<float> ((float) logoX, (float) logoY,
                                                              (float) logoW, (float) logoH),
                                  juce::RectanglePlacement::xLeft | juce::RectanglePlacement::yTop
                                      | juce::RectanglePlacement::onlyReduceInSize, 1.0f);
    }
    else
    {
        g.setColour (juce::Colour (0xff44ff44).withAlpha (0.07f));
        g.setFont (juce::FontOptions (30.0f * fS).withStyle ("Bold"));
        g.drawText ("FREECODER", 60, 6, 310, 36, juce::Justification::centredLeft);
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (26.0f * fS).withStyle ("Bold"));
        g.drawText ("FREECODER", 62, 8, 308, 32, juce::Justification::centredLeft);
        g.setColour (juce::Colour (0xff44ff44).withAlpha (0.85f));
        g.setFont (juce::FontOptions (9.0f * fS).withStyle ("Bold"));
        g.drawText ("SPECTRAL  MORPHING  WORKSTATION", 62, 41, 350, 13, juce::Justification::centredLeft);
    }

    // Branding (top-right)
    g.setColour (juce::Colour (0xff333333));
    g.setFont (juce::FontOptions (7.5f * fS));
    g.drawText ("AMENT  |  AUDIO", W - 112, 52, 104, 12, juce::Justification::centredRight);

    // ── Preset strip ────────────────────────────────────────────────────────
    g.setColour (juce::Colour (0xff44ff44).withAlpha (0.15f));
    g.fillRect (0, headerH, W, presetH);
    g.setColour (juce::Colour (0xff44ff44).withAlpha (0.20f));
    g.drawLine (0.0f, (float) contentY, (float) W, (float) contentY, 1.0f);

    g.setColour (juce::Colour (0xff44ff44));
    g.setFont (juce::FontOptions (12.0f * fS).withStyle ("Bold"));
    const juce::String presetLabel = presetManager.getCurrentPresetName() + (isDirty ? " *" : "");
    g.drawText (presetLabel, 52, headerH + 5, W - 160, 26, juce::Justification::centred);

    // ── Slider section ──────────────────────────────────────────────────────
    const int mX = morphSlider.getX(),  mW = morphSlider.getWidth();
    const int dX = drywetSlider.getX(), dW = drywetSlider.getWidth();

    // MORPH label row: "< PHRASE" left · "MORPH" centre · "SPECTRAL >" right — one line
    const int labelRowY = morphSlider.getY() - 20;
    g.setFont (juce::FontOptions (11.0f * fS).withStyle ("Bold"));
    g.setColour (juce::Colour (0xff666666));
    g.drawText ("< PHRASE",   mX, labelRowY, mW, 18, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xffcccccc));
    g.drawText ("MORPH",      mX, labelRowY, mW, 18, juce::Justification::centred);
    g.setColour (juce::Colour (0xff666666));
    g.drawText ("SPECTRAL >", mX, labelRowY, mW, 18, juce::Justification::centredRight);

    // DRY / WET label
    g.setColour (juce::Colour (0xffcccccc));
    g.drawText ("DRY / WET", dX, drywetSlider.getY() - 20, dW, 18, juce::Justification::centredRight);

    // Value readouts — centred under each slider
    g.setColour (juce::Colour (0xff44ff44));
    g.drawText (juce::String (morphSlider.getValue(),  2), mX, morphSlider.getBottom() + 4,  mW, 16, juce::Justification::centred);
    g.drawText (juce::String (drywetSlider.getValue(), 2), dX, drywetSlider.getBottom() + 4, dW, 16, juce::Justification::centred);

    // ── Pad labels — centred below each knob (industry standard) ────────────
    g.setColour (juce::Colour (0xffaaaaaa));
    g.setFont (juce::FontOptions (11.0f * fS).withStyle ("Bold"));
    const int lblGap = 3;
    const int lblH   = juce::roundToInt (16.0f * fS);
    g.drawText ("GRAIN",   grainSlider.getX(),   grainSlider.getBottom()   + lblGap, grainSlider.getWidth(),   lblH, juce::Justification::centred);
    g.drawText ("SCATTER", scatterSlider.getX(), scatterSlider.getBottom() + lblGap, scatterSlider.getWidth(), lblH, juce::Justification::centred);
    g.drawText ("FORMANT", formantSlider.getX(), formantSlider.getBottom() + lblGap, formantSlider.getWidth(), lblH, juce::Justification::centred);
    g.drawText ("PITCH",   pitchSlider.getX(),   pitchSlider.getBottom()   + lblGap, pitchSlider.getWidth(),   lblH, juce::Justification::centred);

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
    g.setColour (juce::Colour (0xff2a7a2a));
    g.setFont (juce::FontOptions (9.0f * fS).withStyle ("Bold"));
    g.drawText ("DONOR",
                displayBounds.getX(), displayBounds.getY() + 6,
                displayBounds.getWidth(), 14, juce::Justification::centred);

    // ── Spectrum + Waveform display ─────────────────────────────────────────
    {
        auto fullArea = displayBounds.reduced (8, 8).withTrimmedTop (16).withTrimmedBottom (20);
        const int waveH   = fullArea.getHeight() * 2 / 5;
        const int splitY  = fullArea.getBottom() - waveH;
        auto specArea = fullArea.withBottom (splitY - 2);
        auto waveArea = fullArea.withTop    (splitY + 2);
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

        // ── dB grid lines (subtle horizontal guides) ────────────────────────
        g.setColour (juce::Colour (0xff1a2a1a));
        for (float db : { -12.0f, -24.0f, -36.0f, -48.0f })
        {
            const float norm = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
            const float gy   = botY - norm * bh;
            g.drawHorizontalLine (juce::roundToInt (gy), bx, bx + bw);
            g.setColour (juce::Colour (0xff223322));
            g.setFont (juce::FontOptions (7.0f * fS));
            g.drawText (juce::String ((int) db) + "dB", (int) bx + 2, (int) gy - 8, 32, 8, juce::Justification::centredLeft);
            g.setColour (juce::Colour (0xff1a2a1a));
        }

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
            // No donor yet — workflow hint
            g.setColour (juce::Colour (0xff2a5a2a));
            g.setFont (juce::FontOptions (11.0f * fS).withStyle ("Bold"));
            g.drawText ("TAP  REC  TO  CAPTURE  A  DONOR", specArea.toFloat(), juce::Justification::centred);
        }

        // Recording indicator / fill % (top-right corner of display)
        const bool isRecording = processorRef.apvts.getRawParameterValue ("recTrigger")->load() > 0.5f;
        if (isRecording || donorFillLevel > 0.001f)
        {
            g.setColour (isRecording ? juce::Colour (0xffff4040) : juce::Colour (0xff44ff44));
            g.setFont (juce::FontOptions (11.0f * fS).withStyle ("Bold"));
            g.drawText (juce::String ((int) (donorFillLevel * 100)) + "%",
                        specArea.getRight() - 36, specArea.getY(), 34, 14,
                        juce::Justification::centredRight);
        }

        // ── Waveform overview ────────────────────────────────────────────────
        // Thin separator between spectrum and waveform
        g.setColour (juce::Colour (0xff1a3d1a));
        g.drawHorizontalLine (splitY, (float) fullArea.getX(), (float) fullArea.getRight());

        {
            const float wx   = (float) waveArea.getX();
            const float wy   = (float) waveArea.getY();
            const float ww   = (float) waveArea.getWidth();
            const float wh   = (float) waveArea.getHeight();
            const float cy   = wy + wh * 0.5f;
            const float halfH = wh * 0.44f;

            if (waveformSnapshot.hasData)
            {
                const int nPts = SpectralEngine::kWavePoints;

                // Symmetric filled waveform (top then bottom mirror)
                juce::Path wavePath;
                wavePath.startNewSubPath (wx, cy);
                for (int i = 0; i < nPts; ++i)
                {
                    const float px = wx + (float) i / (float) nPts * ww;
                    wavePath.lineTo (px, cy - waveformSnapshot.peaks[(size_t) i] * halfH);
                }
                wavePath.lineTo (wx + ww, cy);
                for (int i = nPts - 1; i >= 0; --i)
                {
                    const float px = wx + (float) i / (float) nPts * ww;
                    wavePath.lineTo (px, cy + waveformSnapshot.peaks[(size_t) i] * halfH);
                }
                wavePath.closeSubPath();

                g.setColour (juce::Colour (0xff163d16));
                g.fillPath (wavePath);
                g.setColour (juce::Colour (0xff44ff44).withAlpha (0.6f));
                g.strokePath (wavePath, juce::PathStrokeType (1.0f));

                // Playhead — white vertical bar when phrase is engaged
                const bool phraseOn = processorRef.apvts.getRawParameterValue ("phraseEngage")->load() > 0.5f;
                if (phraseOn)
                {
                    const float phX = wx + juce::jlimit (0.0f, 1.0f, waveformSnapshot.playhead) * ww;
                    g.setColour (juce::Colours::white.withAlpha (0.75f));
                    g.drawLine (phX, wy, phX, wy + wh, 1.5f);
                }
            }
            else
            {
                // No donor yet — dim placeholder
                g.setColour (juce::Colour (0xff0a1a0a));
                g.fillRect (waveArea);
            }
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

    // ── Section divider above footswitch row ─────────────────────────────────
    {
        const int sepY = recButton.getY() - juce::roundToInt (H * 0.022f);
        g.setColour (juce::Colour (0xff2a4a2a));
        g.drawHorizontalLine (sepY, 16.0f, (float)(W - 16));
        g.setColour (juce::Colour (0xff1a2a1a));
        g.drawHorizontalLine (sepY + 1, 16.0f, (float)(W - 16));
    }

    // ── Footswitch labels (4-button row) ──────────────────────────────────────
    const int recCx_p = recButton.getBounds().getCentreX();
    const int revCx_p = reverseButton.getBounds().getCentreX();
    const int phCx_p  = phraseButton.getBounds().getCentreX();
    const int engCx_p = engageButton.getBounds().getCentreX();
    const int recBot  = recButton.getBottom();
    const int revBot  = reverseButton.getBottom();
    const int phBot   = phraseButton.getBottom();
    const int engBot  = engageButton.getBottom();
    const bool showSubs = (W >= 460);

    // REC label + sublabel
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (13.0f * fS).withStyle ("Bold"));
    g.drawText ("REC", recCx_p - 50, recBot + 4, 100, 18, juce::Justification::centred);
    if (showSubs)
    {
        g.setColour (juce::Colour (0xff888888));
        g.setFont (juce::FontOptions (11.0f * fS));
        g.drawText ("CAPTURE DONOR", recCx_p - juce::roundToInt (W * 0.10f), recBot + 24, juce::roundToInt (W * 0.20f), 18, juce::Justification::centred);
    }

    // REVERSE label + sublabel
    {
        const bool isReversed = processorRef.apvts.getRawParameterValue ("reverse")->load() > 0.5f;
        g.setColour (isReversed ? juce::Colour (0xff44ff44) : juce::Colours::white);
        g.setFont (juce::FontOptions (13.0f * fS).withStyle ("Bold"));
        g.drawText ("REV", revCx_p - 50, revBot + 4, 100, 18, juce::Justification::centred);
        if (showSubs)
        {
            g.setColour (juce::Colour (0xff888888));
            g.setFont (juce::FontOptions (11.0f * fS));
            g.drawText ("REVERSE LOOP", revCx_p - juce::roundToInt (W * 0.10f), revBot + 24, juce::roundToInt (W * 0.20f), 18, juce::Justification::centred);
        }
    }

    // PHRASE label + sublabel
    {
        const bool isMidiForPh = processorRef.apvts.getRawParameterValue ("midiMode")->load() > 0.5f;
        const bool isPhrased   = processorRef.apvts.getRawParameterValue ("phraseEngage")->load() > 0.5f;
        g.setColour (isPhrased ? juce::Colour (0xff44ff44) : juce::Colours::white);
        g.setFont (juce::FontOptions (13.0f * fS).withStyle ("Bold"));
        g.drawText ("PHRASE", phCx_p - 50, phBot + 4, 100, 18, juce::Justification::centred);
        if (showSubs)
        {
            g.setColour (juce::Colour (0xff888888));
            g.setFont (juce::FontOptions (11.0f * fS));
            g.drawText (isMidiForPh ? "LATCH NOTES" : "PHRASE LOOP", phCx_p - juce::roundToInt (W * 0.10f), phBot + 24, juce::roundToInt (W * 0.20f), 18, juce::Justification::centred);
        }
    }

    // FREEZE label + sublabel
    {
        const bool isEngaged = processorRef.apvts.getRawParameterValue ("engage")->load() > 0.5f;
        g.setColour (isEngaged ? juce::Colour (0xff44ff44) : juce::Colours::white);
        g.setFont (juce::FontOptions (13.0f * fS).withStyle ("Bold"));
        g.drawText ("FREEZE", engCx_p - 55, engBot + 4, 110, 18, juce::Justification::centred);
        if (showSubs)
        {
            g.setColour (juce::Colour (0xff888888));
            g.setFont (juce::FontOptions (11.0f * fS));
            g.drawText ("SPECTRAL FREEZE", engCx_p - juce::roundToInt (W * 0.11f), engBot + 24, juce::roundToInt (W * 0.22f), 18, juce::Justification::centred);
        }
    }

    // Rec length labels: 1s / 2s / 3s / 5s below slider, current value highlighted
    {
        const int recCx = recCx_p;
        const int sliderX = recCx - 52;
        const int sliderW = 104;
        const int labelY  = recLengthSlider.getBottom() + 4;
        static const float stops[]  = { 0.0f, 0.25f, 0.5f, 1.0f };
        static const float vals[]   = { 1.0f, 2.0f, 3.0f, 5.0f };
        static const char* labels[] = { "1s", "2s", "3s", "5s" };
        const float recSecs = (float) recLengthSlider.getValue();
        g.setFont (juce::FontOptions (10.0f * fS).withStyle ("Bold"));
        for (int t = 0; t < 4; ++t)
        {
            const int tx = sliderX + (int) (stops[t] * sliderW);
            const bool isCurrent = (std::abs (vals[t] - recSecs) < 0.1f);
            g.setColour (isCurrent ? juce::Colour (0xff44ff44) : juce::Colour (0xff666666));
            g.drawText (labels[t], tx - 12, labelY, 24, 16, juce::Justification::centred);
        }

        // LATCH label — only in MIDI mode
        const bool isLatched = processorRef.apvts.getRawParameterValue ("latch")->load() > 0.5f;
        const bool isMidiForLatch = processorRef.apvts.getRawParameterValue ("midiMode")->load() > 0.5f;
        if (isMidiForLatch)
        {
            g.setColour (isLatched ? juce::Colour (0xff44ffff) : juce::Colour (0xff777777));
            g.setFont (juce::FontOptions (8.5f * fS));
            g.drawText ("HOLD NOTES", latchButton.getX() - 4, latchButton.getBottom() + 2,
                        latchButton.getWidth() + 8, 12, juce::Justification::centred);
        }
    }


    // ── MIDI utility row ────────────────────────────────────────────────────────
    {
        const bool isMidi = processorRef.apvts.getRawParameterValue ("midiMode")->load() > 0.5f;
        // modeButton label updates to reflect current mode
        const_cast<PluginEditor*>(this)->modeButton.setButtonText (isMidi ? "MIDI MODE" : "EFFECT MODE");

        const int modeButtonY = modeButton.getY();
        if (isMidi)
        {
            // Root note: "ROOT:" label, then slider, then live note name
            const int root = (int) processorRef.apvts.getRawParameterValue ("rootNote")->load();
            g.setColour (juce::Colour (0xff44ffff));
            g.setFont (juce::FontOptions (10.0f * fS).withStyle ("Bold"));
            g.drawText ("ROOT:", W / 2 + 62, modeButtonY, 38, 22, juce::Justification::centredLeft);
            // Note name sits right of the slider (slider ends at W/2+166), updates every repaint
            g.drawText (midiNoteToName (root), W / 2 + 170, modeButtonY, 44, 22, juce::Justification::centredLeft);

            // Tuner: live pitch of input signal (left of the mode button)
            if (tunerResult.hasData)
            {
                const float cents = tunerResult.centsOffset;
                const juce::Colour tunerCol = std::abs (cents) < 10.0f ? juce::Colour (0xff44ff44)
                                            : std::abs (cents) < 25.0f ? juce::Colour (0xffffaa00)
                                                                        : juce::Colour (0xffff4444);
                g.setColour (tunerCol);
                g.setFont (juce::FontOptions (10.0f * fS).withStyle ("Bold"));
                const juce::String centsStr = (cents >= 0.0f ? "+" : "") + juce::String ((int) cents) + "c";
                g.drawText (midiNoteToName (tunerResult.midiNote) + " " + centsStr,
                            10, modeButtonY, W / 2 - 70, 22, juce::Justification::centredRight);
                g.setColour (juce::Colour (0xff333333));
                g.setFont (juce::FontOptions (7.5f * fS));
                g.drawText ("INPUT", 10, modeButtonY + 12, W / 2 - 70, 10, juce::Justification::centredRight);
            }

            // ADSR letters below each knob (MIDI mode)
            g.setColour (juce::Colour (0xff44ff44));
            g.setFont (juce::FontOptions (13.0f * fS).withStyle ("Bold"));
            const int labelY = adsrAttackSlider.getBottom() + 2;
            static const char* labels[] = { "A", "D", "S", "R" };
            const juce::Slider* adsrSliders[] = { &adsrAttackSlider, &adsrDecaySlider, &adsrSustainSlider, &adsrReleaseSlider };
            for (int i = 0; i < 4; ++i)
                g.drawText (labels[i], adsrSliders[i]->getX(), labelY, adsrSliders[i]->getWidth(), juce::roundToInt (16.0f * fS),
                            juce::Justification::centred);
        }
        else
        {
            // Effect mode — show ADSR labels if effectAdsr is on
            const bool showEffAdsr = processorRef.apvts.getRawParameterValue ("effectAdsr")->load() > 0.5f;
            if (showEffAdsr)
            {
                g.setColour (juce::Colour (0xff44ff44));
                g.setFont (juce::FontOptions (13.0f * fS).withStyle ("Bold"));
                const int labelY = adsrAttackSlider.getBottom() + 2;
                static const char* effLabels[] = { "A", "D", "S", "R" };
                const juce::Slider* adsrSliders[] = { &adsrAttackSlider, &adsrDecaySlider, &adsrSustainSlider, &adsrReleaseSlider };
                for (int i = 0; i < 4; ++i)
                    g.drawText (effLabels[i], adsrSliders[i]->getX(), labelY, adsrSliders[i]->getWidth(), juce::roundToInt (16.0f * fS),
                                juce::Justification::centred);
            }

            // effectAdsrButton label
            g.setColour (showEffAdsr ? juce::Colour (0xff44ff44) : juce::Colour (0xff555555));
            g.setFont (juce::FontOptions (7.0f * fS));
            g.drawText ("ENV SHAPE", effectAdsrButton.getX() - 6, effectAdsrButton.getBottom() + 2,
                        effectAdsrButton.getWidth() + 12, 10, juce::Justification::centred);
        }
    }

    // ── Bottom branding + diagnostics ──────────────────────────────────────────
    g.setColour (juce::Colour (0xff2a2a2a));
    g.setFont (juce::FontOptions (8.0f * fS));
    g.drawText ("A M E N T  A U D I O  |  F R E E C O D E R  v 0 . 2 . 1 3", 0, H - 16, W - 120, 14, juce::Justification::centred);

    // Diagnostic readout: actual engine state (not params — params lag real state)
    const bool isRec    = processorRef.isDonorRecording();
    const bool hasDonor = processorRef.getDonorLength() > 0;
    const bool engOn    = processorRef.apvts.getRawParameterValue ("engage")->load() > 0.5f;
    juce::String stateStr = juce::String (isRec ? "REC " : (hasDonor ? "READY " : "")) + (engOn ? "ENG" : "");
    g.setColour (stateStr.isNotEmpty() ? juce::Colour (0xff44ff44) : juce::Colour (0xff222222));
    g.drawText (stateStr, 4, H - 16, 80, 14, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xff333333));
    g.drawText ("CH:" + juce::String (processorRef.diagInputChannels.load())
                + " BS:" + juce::String (processorRef.diagBlockSize.load()),
                W - 80, H - 16, 78, 14, juce::Justification::centredRight);
}

void PluginEditor::resized()
{
    const int W = getWidth();
    const int H = getHeight();
    processorRef.setEditorSize (W, H);

    const int headerH  = juce::roundToInt (H * 0.1133f);   // ~68 at 600
    const int presetH  = juce::roundToInt (H * 0.060f);    // ~36 at 600
    const int contentY = headerH + presetH;                 // ~104 at 600

    // ── Preset strip ────────────────────────────────────────────────────────────
    prevPresetButton.setBounds (20,       headerH + 5,  28,  26);
    nextPresetButton.setBounds (W - 100,  headerH + 5,  28,  26);
    savePresetButton.setBounds (W - 68,   headerH + 5,  58,  26);

    // ── Top sliders ─────────────────────────────────────────────────────────────
    const int sliderY  = contentY + juce::roundToInt (H * 0.0367f);  // ~22 gap
    const int sliderH2 = juce::roundToInt (H * 0.0533f);             // ~32
    const int sliderW  = juce::roundToInt (W * 0.407f);              // ~220
    morphSlider.setBounds  (20,               sliderY, sliderW, sliderH2);
    drywetSlider.setBounds (W - 20 - sliderW, sliderY, sliderW, sliderH2);

    // ── Pads ────────────────────────────────────────────────────────────────────
    const int padMarginX = juce::roundToInt (W * 0.048f);   // narrow gutter — labels go below knobs
    const int padW       = juce::roundToInt (W * 0.165f);   // knob width
    const int padTotalH  = juce::roundToInt (H * 0.150f);   // knob + label row
    const int padH       = juce::roundToInt (padTotalH * 0.82f);  // knob only (label in bottom 18%)
    const int pad1Y      = contentY + juce::roundToInt (H * 0.133f);
    const int pad2Y      = pad1Y   + juce::roundToInt (H * 0.175f);  // wider row gap to fit label
    const int leftX      = padMarginX;
    const int rightX     = W - padW - padMarginX;

    grainSlider.setBounds   (leftX,  pad1Y, padW, padH);
    scatterSlider.setBounds (leftX,  pad2Y, padW, padH);
    formantSlider.setBounds (rightX, pad1Y, padW, padH);
    pitchSlider.setBounds   (rightX, pad2Y, padW, padH);

    // ── Centre display ──────────────────────────────────────────────────────────
    const int dispGap = juce::roundToInt (W * 0.020f);
    const int dispX   = leftX + padW + dispGap;
    const int dispW   = rightX - dispX - dispGap;
    displayBounds     = { dispX, pad1Y, dispW, pad2Y + padTotalH - pad1Y };

    // ── Donor slot buttons below the display ────────────────────────────────────
    {
        const int btnW   = juce::roundToInt (W * 0.067f);  // ~36
        const int btnH   = 16;
        const int gap    = juce::roundToInt (W * 0.015f);  // ~8
        const int totalW = 3 * btnW + 2 * gap;
        const int startX = displayBounds.getCentreX() - totalW / 2;
        const int btnY   = displayBounds.getBottom() + 4;
        for (int i = 0; i < SpectralEngine::kNumDonorSlots; ++i)
            slotButtons[i]->setBounds (startX + i * (btnW + gap), btnY, btnW, btnH);
        importButton.setBounds (startX - (btnW + gap), btnY, btnW, btnH);
        exportButton.setBounds (startX + 3 * (btnW + gap), btnY, btnW, btnH);
    }

    // ── Footswitches (4-button row: REC / REVERSE / PHRASE / FREEZE) ─────────────
    const int swSize = juce::roundToInt (juce::jmin (W * 0.130f, H * 0.118f));  // ~70
    const int swY    = pad2Y + padTotalH + juce::roundToInt (H * 0.030f);
    const int recCx  = W / 5;
    const int revCx  = W * 2 / 5;
    const int phCx   = W * 3 / 5;
    const int engCx  = W * 4 / 5;
    recButton.setBounds     (recCx - swSize / 2, swY, swSize, swSize);
    reverseButton.setBounds (revCx - swSize / 2, swY, swSize, swSize);
    phraseButton.setBounds  (phCx  - swSize / 2, swY, swSize, swSize);
    engageButton.setBounds  (engCx - swSize / 2, swY, swSize, swSize);

    // ── Rec controls (below REC footswitch) ──────────────────────────────────────
    {
        const int belowSw = swY + swSize;
        recLengthSlider.setBounds  (recCx - 52, belowSw + juce::roundToInt (H * 0.085f), 104, 16);
        effectAdsrButton.setBounds (engCx - 24, belowSw + juce::roundToInt (H * 0.085f),  48, 16);
        latchButton.setBounds      (phCx  - 28, belowSw + juce::roundToInt (H * 0.085f),  56, 16);
    }

    // ── MIDI utility row ─────────────────────────────────────────────────────────
    const int modeY = swY + swSize + juce::roundToInt (H * 0.085f);
    modeButton.setBounds (W / 2 - 60, modeY, 120, 22);
    rootNoteSlider.setBounds (W / 2 + 66, modeY, 100, 22);   // right of mode button, visible in MIDI mode

    // ── ADSR row ─────────────────────────────────────────────────────────────────
    {
        const int kw  = juce::roundToInt (W * 0.061f);
        const int gap = juce::roundToInt (W * 0.022f);
        const int rowX = (W - 4 * kw - 3 * gap) / 2;
        const int rowY = modeY + 28;
        adsrAttackSlider .setBounds (rowX + 0 * (kw + gap), rowY, kw, kw);
        adsrDecaySlider  .setBounds (rowX + 1 * (kw + gap), rowY, kw, kw);
        adsrSustainSlider.setBounds (rowX + 2 * (kw + gap), rowY, kw, kw);
        adsrReleaseSlider.setBounds (rowX + 3 * (kw + gap), rowY, kw, kw);
    }

    // ── Inspector (tiny, top-right corner) ───────────────────────────────────────
    inspectButton.setBounds (W - 18, 2, 16, 16);
}
