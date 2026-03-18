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

        g.setColour (juce::Colour (0xff0a2a0a));
        g.fillRoundedRectangle ((float) x, cy - th * 0.5f, (float) width, th, 2.5f);

        g.setColour (juce::Colour (0xff22aa22));
        g.fillRoundedRectangle ((float) x, cy - th * 0.5f, sliderPos - (float) x, th, 2.5f);

        const float tw = 12.0f, th2 = 28.0f;
        g.setColour (juce::Colour (0xff44ff44));
        g.fillRoundedRectangle (sliderPos - tw * 0.5f, cy - th2 * 0.5f, tw, th2, 3.0f);

        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.fillRoundedRectangle (sliderPos - tw * 0.5f + 2, cy - th2 * 0.5f + 3, tw - 4, th2 * 0.35f, 2.0f);
    }

    // Pad slider (GRAIN / SCATTER / FORMANT — drawn as illuminated rectangle)
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float, float, float, juce::Slider& slider) override
    {
        const float prop = (float) (slider.getValue() - slider.getMinimum())
                         / (float) (slider.getMaximum() - slider.getMinimum());

        auto b = juce::Rectangle<float> ((float) x + 3, (float) y + 3,
                                         (float) width - 6, (float) height - 6);

        // Outer glow
        if (prop > 0.01f)
        {
            g.setColour (juce::Colour (0xff44ff44).withAlpha (prop * 0.22f));
            g.fillRoundedRectangle (b.expanded (3.0f), 9.0f);
        }

        // Pad body
        g.setColour (juce::Colour (0xff0b160b));
        g.fillRoundedRectangle (b, 6.0f);

        // Fill level (bottom to top)
        auto fill = b.withTrimmedTop (b.getHeight() * (1.0f - prop));
        g.setColour (juce::Colour (0xff163d16));
        g.fillRoundedRectangle (fill, 6.0f);

        // Border
        g.setColour (juce::Colour (0xff44ff44).withAlpha (0.7f));
        g.drawRoundedRectangle (b, 6.0f, 1.5f);

        // Value — semitones for pitch pad, percentage for others
        g.setColour (juce::Colour (0xff44ff44));
        g.setFont (juce::FontOptions (15.0f).withStyle ("Bold"));
        juce::String valStr;
        if (slider.getMaximum() > 1.0)  // pitch pad
        {
            const double v = slider.getValue();
            valStr = (v >= 0.0 ? "+" : "") + juce::String (v, 1);
        }
        else
        {
            valStr = juce::String ((int) (prop * 100));
        }
        g.drawText (valStr, b.reduced (4.0f), juce::Justification::centred);
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
      laf (std::make_unique<FreecoderLookAndFeel>())
{
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

    startTimerHz (15);
    setSize (540, 500);
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
    {
        donorFillLevel = newLevel;
        repaint (displayBounds);
    }

    // Repaint footswitch area when engage state changes so the label colour updates
    repaint (engageButton.getBounds().expanded (0, 40));
}

//==============================================================================
static void drawDots (juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour (juce::Colour (0xff1b1b1b));
    for (int dy = area.getY() + 6; dy < area.getBottom(); dy += 12)
        for (int dx = area.getX() + 6; dx < area.getRight(); dx += 12)
            g.fillEllipse ((float) dx - 1.5f, (float) dy - 1.5f, 3.0f, 3.0f);
}

static void drawDimPad (juce::Graphics& g, juce::Rectangle<int> b)
{
    auto bf = b.toFloat().reduced (3.0f);
    g.setColour (juce::Colour (0xff0b160b));
    g.fillRoundedRectangle (bf, 6.0f);
    g.setColour (juce::Colour (0xff163316));
    g.drawRoundedRectangle (bf, 6.0f, 1.0f);
}

void PluginEditor::paint (juce::Graphics& g)
{
    const int W = getWidth(), H = getHeight();

    // ── Background ──────────────────────────────────────────────────────────
    g.fillAll (juce::Colour (0xff0f0f0f));
    drawDots (g, { 0, 68, W, H - 68 });

    // ── Header panel ────────────────────────────────────────────────────────
    g.setColour (juce::Colour (0xff141414));
    g.fillRect (0, 0, W, 68);
    g.setColour (juce::Colour (0xff44ff44).withAlpha (0.35f));
    g.drawLine (0, 67, (float) W, 67, 1.0f);

    // Dot + title
    g.setColour (juce::Colour (0xff44ff44));
    g.fillEllipse (46, 16, 11, 11);
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (26.0f).withStyle ("Bold"));
    g.drawText ("FREECODER", 64, 8, 300, 32, juce::Justification::centredLeft);

    // Subtitle
    g.setColour (juce::Colour (0xff44ff44));
    g.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
    g.drawText ("SPECTRAL MORPHING WORKSTATION", 64, 40, 340, 14, juce::Justification::centredLeft);

    // Branding (top-right)
    g.setColour (juce::Colour (0xff383838));
    g.setFont (juce::FontOptions (7.5f));
    g.drawText ("AMENT | AUDIO", W - 110, 52, 100, 12, juce::Justification::centredRight);

    // ── Slider section ──────────────────────────────────────────────────────
    // Left stacked labels (MORPH slider)
    auto drawSliderLabels = [&] (int lx, int ly, const juce::String& name)
    {
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
        g.drawText (name, lx, ly, 100, 14, juce::Justification::centredLeft);
    };

    drawSliderLabels (20,     72, "morph");
    drawSliderLabels (W - 120, 72, "dry/wet");

    // Value readouts
    g.setColour (juce::Colour (0xff44ff44));
    g.setFont (juce::FontOptions (8.5f));
    g.drawText (juce::String (morphSlider.getValue(),  2), 20,     125, 80, 12, juce::Justification::centredLeft);
    g.drawText (juce::String (drywetSlider.getValue(), 2), W - 120, 125, 80, 12, juce::Justification::centredRight);

    // Morph endpoint labels: PHRASE (0) on left, SPECTRAL (1) on right
    g.setColour (juce::Colour (0xff333333));
    g.setFont (juce::FontOptions (7.0f));
    g.drawText ("phrase",   20,      139, 80,  10, juce::Justification::centredLeft);
    g.drawText ("spectral", 20,      139, 218, 10, juce::Justification::centredRight);

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

    // Waveform
    {
        const int dlen = processorRef.getDonorLength();
        if (dlen > 0)
        {
            const auto& buf  = processorRef.getDonorBuffer();
            auto waveArea    = displayBounds.reduced (8, 8).withTrimmedTop (16).withTrimmedBottom (20);
            const float midY = waveArea.getCentreY();
            const float half = waveArea.getHeight() * 0.45f;
            const int   ww   = waveArea.getWidth();

            const bool isRecording = processorRef.apvts.getRawParameterValue ("recTrigger")->load() > 0.5f;
            g.setColour (isRecording ? juce::Colour (0xff4a1a1a) : juce::Colour (0xff1a3d1a));

            for (int px = 0; px < ww; ++px)
            {
                const int s0 = (int) ((float) px       / (float) ww * (float) dlen);
                const int s1 = (int) ((float) (px + 1) / (float) ww * (float) dlen);
                float mn = 0.0f, mx = 0.0f;
                for (int s = s0; s < s1 && s < dlen; ++s)
                {
                    const float v = buf.getSample (0, s);
                    mn = juce::jmin (mn, v);
                    mx = juce::jmax (mx, v);
                }
                const float y1 = midY - mx * half;
                const float y2 = juce::jmax (y1 + 1.0f, midY - mn * half);
                g.drawLine ((float) (waveArea.getX() + px), y1,
                            (float) (waveArea.getX() + px), y2, 1.0f);
            }

            // Percentage overlay (small, top-right corner of waveform area)
            g.setColour (isRecording ? juce::Colour (0xffff4040) : juce::Colour (0xff44ff44));
            g.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
            g.drawText (juce::String ((int) (donorFillLevel * 100)) + "%",
                        waveArea.getRight() - 36, waveArea.getY(), 34, 14,
                        juce::Justification::centredRight);
        }
        else
        {
            // No donor yet — just show placeholder
            g.setColour (juce::Colour (0xff1a3d1a));
            g.setFont (juce::FontOptions (28.0f).withStyle ("Bold"));
            g.drawText ("--", displayBounds.reduced (8, 16).toFloat(), juce::Justification::centred);
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

    // "PLAY PHRASE" connecting line
    const int lineY = recBot + 15;
    g.setColour (juce::Colour (0xff2a2a2a));
    g.drawLine ((float) recCx, (float) lineY, (float) engCx, (float) lineY, 1.0f);
    g.setColour (juce::Colour (0xff383838));
    g.setFont (juce::FontOptions (7.0f));
    g.drawText ("PLAY PHRASE", recCx, lineY - 6, engCx - recCx, 12, juce::Justification::centred);

    // ── Bottom branding ────────────────────────────────────────────────────────
    g.setColour (juce::Colour (0xff2a2a2a));
    g.setFont (juce::FontOptions (8.0f));
    g.drawText ("A M E N T  A U D I O  |  F R E E C O D E R  v 0 . 1 . 0", 0, H - 16, W, 14, juce::Justification::centred);
}

void PluginEditor::resized()
{
    const int W = getWidth();

    // ── Top sliders ────────────────────────────────────────────────────────────
    morphSlider.setBounds  (20,      90, 220, 32);
    drywetSlider.setBounds (W - 240, 90, 220, 32);

    // ── Pads ───────────────────────────────────────────────────────────────────
    const int padW  = 110, padH = 82;
    const int pad1Y = 148, pad2Y = 244;
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
    const int swY    = 350;
    recButton.setBounds     (W / 4 - swSize / 2,     swY, swSize, swSize);
    engageButton.setBounds  (3 * W / 4 - swSize / 2, swY, swSize, swSize);
    reverseButton.setBounds (W / 2 - 38, swY + swSize / 2 - 11, 76, 22);

    // ── Inspector (tiny, top-right corner) ────────────────────────────────────
    inspectButton.setBounds (W - 18, 2, 16, 16);
}
