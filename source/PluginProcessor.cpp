#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> ("morph",    "Morph",    0.0f, 1.0f, 0.5f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("grain",    "Grain",    0.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("formant",  "Formant",  0.0f, 1.0f, 0.5f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("scatter",  "Scatter",  0.0f, 1.0f, 0.3f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("drywet",   "Dry/Wet",  0.0f, 1.0f, 0.8f));
    layout.add (std::make_unique<juce::AudioParameterBool>  ("recTrigger", "Record",  false));
    layout.add (std::make_unique<juce::AudioParameterBool>  ("engage",     "Engage",  false));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("pitch",      "Pitch",   -12.0f, 12.0f, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterBool>  ("reverse",    "Reverse", false));
    layout.add (std::make_unique<juce::AudioParameterBool>  ("midiMode",   "MIDI Mode", false));
    layout.add (std::make_unique<juce::AudioParameterInt>   ("rootNote",   "Root Note", 0, 127, 60));
    // Record length: 1, 2, 3, or 5 seconds (stepped)
    layout.add (std::make_unique<juce::AudioParameterFloat> ("recLength",   "Rec Length", 1.0f, 5.0f, 5.0f));
    layout.add (std::make_unique<juce::AudioParameterBool>  ("autoEngage",  "Auto Engage", true));
    layout.add (std::make_unique<juce::AudioParameterBool>  ("latch",        "Latch",       false));
    layout.add (std::make_unique<juce::AudioParameterBool>  ("phraseEngage", "Phrase Loop",  false));
    layout.add (std::make_unique<juce::AudioParameterBool>  ("effectAdsr",   "Effect ADSR",  false));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("adsrAttack",  "Attack",  0.001f, 5.0f,  0.01f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("adsrDecay",   "Decay",   0.001f, 5.0f,  0.20f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("adsrSustain", "Sustain", 0.0f,   1.0f,  1.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("adsrRelease", "Release", 0.001f, 10.0f, 0.50f));

    return layout;
}

//==============================================================================
PluginProcessor::PluginProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts (*this, nullptr, "Parameters", createParameterLayout()),
       presetManager (apvts, spectralEngine)
{
}

PluginProcessor::~PluginProcessor()
{
}

//==============================================================================
const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PluginProcessor::getTailLengthSeconds() const
{
    // ENGAGE creates an indefinite spectral sustain; report a generous tail
    // so DAWs don't cut playback before the effect has decayed
    return spectralEngine.isEngaged() ? 30.0 : 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return 1;
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String PluginProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void PluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    spectralEngine.prepare (sampleRate, samplesPerBlock);
    setLatencySamples (SpectralEngine::getLatencySamples());
}

void PluginProcessor::releaseResources()
{
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    const auto in = layouts.getMainInputChannelSet();
    // Must have matching input — donor recording always needs audio input
    if (in != out)
        return false;

    return true;
  #endif
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    diagInputChannels.store (totalNumInputChannels);
    diagBlockSize.store     (buffer.getNumSamples());
    diagBlockCount.fetch_add (1);

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Parameters that always apply regardless of mode
    // Rec length: snap raw value to nearest of {1,2,3,5} seconds
    {
        const float raw  = apvts.getRawParameterValue ("recLength")->load();
        const float snapped = raw <= 1.5f ? 1.0f : raw <= 2.5f ? 2.0f : raw <= 4.0f ? 3.0f : 5.0f;
        const int   samples = juce::jlimit (1, SpectralEngine::kMaxDonorSamples,
                                            (int) (snapped * getSampleRate()));
        spectralEngine.setRecordLimitSamples (samples);
    }

    spectralEngine.setMorph   (apvts.getRawParameterValue ("morph")->load());
    spectralEngine.setDryWet  (apvts.getRawParameterValue ("drywet")->load());
    spectralEngine.setGrain   (apvts.getRawParameterValue ("grain")->load());
    spectralEngine.setFormant (apvts.getRawParameterValue ("formant")->load());
    spectralEngine.setScatter (apvts.getRawParameterValue ("scatter")->load());
    spectralEngine.setReverse (apvts.getRawParameterValue ("reverse")->load() > 0.5f);

    // Edge-triggered REC: only act on button transitions, not every block.
    // This prevents startRecording() from re-clearing the donor buffer when the
    // buffer auto-fills while the button is still held on.
    const bool recNow = apvts.getRawParameterValue ("recTrigger")->load() > 0.5f;
    if (recNow && !prevRecTrigger)
        spectralEngine.startRecording();
    else if (!recNow && prevRecTrigger)
        spectralEngine.stopRecording();
    prevRecTrigger = recNow;

    // Donor slot switching — requested by UI, applied at block boundary
    {
        const int newSlot = requestedSlot.load();
        if (newSlot != currentSlot)
        {
            spectralEngine.setActiveSlot (newSlot);
            currentSlot = newSlot;
        }
    }

    // Auto-engage: fires once when recording stops, if the autoEngage param is on and in effect mode
    if (spectralEngine.consumeAutoEngagePending())
    {
        const bool autoEngage = apvts.getRawParameterValue ("autoEngage")->load() > 0.5f;
        const bool isMidi     = apvts.getRawParameterValue ("midiMode")->load() > 0.5f;
        if (autoEngage && !isMidi)
        {
            spectralEngine.setEngage (true);
            juce::MessageManager::callAsync ([this]
            {
                if (auto* p = apvts.getParameter ("engage"))
                    p->setValueNotifyingHost (1.0f);
            });
        }
    }

    const bool midiMode = apvts.getRawParameterValue ("midiMode")->load() > 0.5f;
    spectralEngine.setMidiMode (midiMode);

    if (midiMode)
    {
        // Push ADSR params every block (cheap — just sets a struct)
        spectralEngine.setAdsrParams (
            apvts.getRawParameterValue ("adsrAttack") ->load(),
            apvts.getRawParameterValue ("adsrDecay")  ->load(),
            apvts.getRawParameterValue ("adsrSustain")->load(),
            apvts.getRawParameterValue ("adsrRelease")->load());

        // Polyphonic MIDI instrument mode: each note gets its own voice + ADSR.
        const int  root    = (int) apvts.getRawParameterValue ("rootNote")->load();
        const bool latched = apvts.getRawParameterValue ("latch")->load() > 0.5f;
        for (const auto& meta : midiMessages)
        {
            const auto msg = meta.getMessage();
            if (msg.isNoteOn())
                spectralEngine.triggerMidiNoteOn (msg.getNoteNumber(),
                                                   (float) (msg.getNoteNumber() - root),
                                                   msg.getFloatVelocity());
            else if (msg.isNoteOff() && !latched)
                spectralEngine.triggerMidiNoteOff (msg.getNoteNumber());
        }
    }
    else
    {
        // Effect mode: FREEZE and PHRASE are independent; PITCH applies to phrase loop
        const bool effectAdsr = apvts.getRawParameterValue ("effectAdsr")->load() > 0.5f;
        spectralEngine.setEffectAdsr   (effectAdsr);
        if (effectAdsr)
            spectralEngine.setAdsrParams (
                apvts.getRawParameterValue ("adsrAttack") ->load(),
                apvts.getRawParameterValue ("adsrDecay")  ->load(),
                apvts.getRawParameterValue ("adsrSustain")->load(),
                apvts.getRawParameterValue ("adsrRelease")->load());
        spectralEngine.setEngage       (apvts.getRawParameterValue ("engage")->load()       > 0.5f);
        spectralEngine.setPhraseEngage (apvts.getRawParameterValue ("phraseEngage")->load() > 0.5f);
        spectralEngine.setPitch        (apvts.getRawParameterValue ("pitch")->load());
    }

    spectralEngine.process (buffer);
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

//==============================================================================
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream (destData, false);

    // APVTS XML
    auto state = apvts.copyState();
    auto xml = state.createXml();
    auto xmlStr = xml->toString (juce::XmlElement::TextFormat().singleLine());
    stream.writeInt (xmlStr.getNumBytesAsUTF8());
    stream.write (xmlStr.toUTF8(), (size_t) xmlStr.getNumBytesAsUTF8());

    // Active slot index + all 3 donor slots
    stream.writeInt (spectralEngine.getActiveSlot());
    for (int s = 0; s < SpectralEngine::kNumDonorSlots; ++s)
    {
        if (spectralEngine.donorSlotHasData (s))
        {
            // For the active slot, use the live working buffer (most up-to-date)
            const bool isActive = (s == spectralEngine.getActiveSlot());
            const auto& buf   = isActive ? spectralEngine.getDonorBuffer()
                                         : spectralEngine.getDonorSlotBuffer (s);
            const int   len   = isActive ? spectralEngine.getDonorLength()
                                         : spectralEngine.getDonorSlotLength (s);
            stream.writeInt (len);
            stream.writeInt (buf.getNumChannels());
            for (int c = 0; c < buf.getNumChannels(); ++c)
                stream.write (buf.getReadPointer (c), (size_t) len * sizeof (float));
        }
        else
        {
            stream.writeInt (0); // empty slot marker
        }
    }
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::MemoryInputStream stream (data, (size_t) sizeInBytes, false);

    // APVTS XML
    const int xmlLen = stream.readInt();
    if (xmlLen > 0 && xmlLen < sizeInBytes)
    {
        juce::MemoryBlock xmlBlock;
        stream.readIntoMemoryBlock (xmlBlock, xmlLen);
        juce::String xmlStr (static_cast<const char*> (xmlBlock.getData()), (size_t) xmlLen);
        if (auto xml = juce::XmlDocument::parse (xmlStr))
            if (xml->hasTagName (apvts.state.getType()))
                apvts.replaceState (juce::ValueTree::fromXml (*xml));
    }

    // Donor slots (optional — old single-slot presets fall through gracefully)
    if (stream.isExhausted()) return;
    const int savedActiveSlot = stream.readInt();

    for (int s = 0; s < SpectralEngine::kNumDonorSlots; ++s)
    {
        if (stream.isExhausted()) break;
        const int len = stream.readInt();
        if (len <= 0) continue;  // empty slot
        if (len > SpectralEngine::kMaxDonorSamples || stream.isExhausted()) break;

        const int numCh = stream.readInt();
        if (numCh <= 0 || numCh > 2) break;

        juce::AudioBuffer<float> buf (numCh, len);
        for (int c = 0; c < numCh; ++c)
            stream.read (buf.getWritePointer (c), (int) ((size_t) len * sizeof (float)));

        // Load into the slot: switch to it, call setDonorData, then restore active slot
        spectralEngine.setActiveSlot (s);
        spectralEngine.setDonorData (buf, len);
    }

    // Restore the active slot last
    spectralEngine.setActiveSlot (juce::jlimit (0, SpectralEngine::kNumDonorSlots - 1, savedActiveSlot));
    currentSlot = spectralEngine.getActiveSlot();
    requestedSlot.store (currentSlot);
}

//==============================================================================
void PluginProcessor::exportActiveDonorSlotToWav()
{
    const int len   = spectralEngine.getDonorLength();
    const int numCh = spectralEngine.getDonorBuffer().getNumChannels();
    if (len <= 0 || numCh <= 0) return;

    // Snapshot the donor buffer from the message thread (stable when not recording)
    juce::AudioBuffer<float> snapshot (numCh, len);
    for (int c = 0; c < numCh; ++c)
        snapshot.copyFrom (c, 0, spectralEngine.getDonorBuffer(), c, 0, len);

    const double sr = getSampleRate() > 0.0 ? getSampleRate() : 44100.0;

    auto chooser = std::make_shared<juce::FileChooser> (
        "Export donor audio as WAV",
        juce::File::getSpecialLocation (juce::File::userDesktopDirectory)
            .getChildFile ("freecoder_donor.wav"),
        "*.wav");

    chooser->launchAsync (
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [chooser, snapshot = std::move (snapshot), sr] (const juce::FileChooser& fc) mutable
        {
            const auto results = fc.getResults();
            if (results.isEmpty()) return;

            auto dest = results[0];
            if (dest.getFileExtension().toLowerCase() != ".wav")
                dest = dest.withFileExtension ("wav");

            juce::WavAudioFormat wavFmt;
            std::unique_ptr<juce::AudioFormatWriter> writer (
                wavFmt.createWriterFor (
                    new juce::FileOutputStream (dest),
                    sr,
                    (unsigned int) snapshot.getNumChannels(),
                    32,   // 32-bit float PCM
                    {},
                    0));
            if (writer)
                writer->writeFromAudioSampleBuffer (snapshot, 0, snapshot.getNumSamples());
        });
}

//==============================================================================
void PluginProcessor::importDonorFromFile()
{
    auto chooser = std::make_shared<juce::FileChooser> (
        "Import audio as donor",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
        "*.wav;*.aiff;*.aif;*.flac;*.ogg;*.mp3");

    chooser->launchAsync (
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser] (const juce::FileChooser& fc) mutable
        {
            const auto results = fc.getResults();
            if (results.isEmpty()) return;

            juce::AudioFormatManager fmt;
            fmt.registerBasicFormats();

            std::unique_ptr<juce::AudioFormatReader> reader (fmt.createReaderFor (results[0]));
            if (!reader) return;

            const int maxSamples = SpectralEngine::kMaxDonorSamples;
            const int numSamples = (int) juce::jmin ((juce::int64) maxSamples, reader->lengthInSamples);
            const int numCh      = (int) juce::jmin ((unsigned int) 2, reader->numChannels);

            juce::AudioBuffer<float> buf (numCh, numSamples);
            reader->read (&buf, 0, numSamples, 0, true, numCh > 1);

            // importDonorData: loads into working buffer + snapshots to active slot
            // Called from message thread — best-effort (not RT-safe, but stable when idle)
            spectralEngine.importDonorData (buf, numSamples);

            // Kick auto-engage if the param is on and we're in effect mode
            const bool autoEngage = apvts.getRawParameterValue ("autoEngage")->load() > 0.5f;
            const bool isMidi     = apvts.getRawParameterValue ("midiMode")->load() > 0.5f;
            if (autoEngage && !isMidi)
            {
                spectralEngine.setEngage (true);
                juce::MessageManager::callAsync ([this]
                {
                    if (auto* p = apvts.getParameter ("engage"))
                        p->setValueNotifyingHost (1.0f);
                });
            }
        });
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
