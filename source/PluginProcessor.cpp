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

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Parameters that always apply regardless of mode
    spectralEngine.setMorph   (apvts.getRawParameterValue ("morph")->load());
    spectralEngine.setDryWet  (apvts.getRawParameterValue ("drywet")->load());
    spectralEngine.setGrain   (apvts.getRawParameterValue ("grain")->load());
    spectralEngine.setFormant (apvts.getRawParameterValue ("formant")->load());
    spectralEngine.setScatter (apvts.getRawParameterValue ("scatter")->load());
    spectralEngine.setReverse (apvts.getRawParameterValue ("reverse")->load() > 0.5f);

    if (apvts.getRawParameterValue ("recTrigger")->load() > 0.5f)
        spectralEngine.startRecording();
    else
        spectralEngine.stopRecording();

    const bool midiMode = apvts.getRawParameterValue ("midiMode")->load() > 0.5f;

    if (midiMode)
    {
        // MIDI instrument mode: ENGAGE and PITCH driven by incoming notes.
        // Last-note priority; noteOff only disengages if the held note is released.
        const int root = (int) apvts.getRawParameterValue ("rootNote")->load();
        for (const auto& meta : midiMessages)
        {
            const auto msg = meta.getMessage();
            if (msg.isNoteOn())
            {
                midiCurrentNote = msg.getNoteNumber();
                spectralEngine.setPitch ((float) (midiCurrentNote - root));
                spectralEngine.setEngage (true);
            }
            else if (msg.isNoteOff() && msg.getNoteNumber() == midiCurrentNote)
            {
                midiCurrentNote = -1;
                spectralEngine.setEngage (false);
            }
        }
    }
    else
    {
        // Effect mode: ENGAGE and PITCH driven by UI / automation as before
        midiCurrentNote = -1;
        spectralEngine.setEngage (apvts.getRawParameterValue ("engage")->load() > 0.5f);
        spectralEngine.setPitch  (apvts.getRawParameterValue ("pitch")->load());
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

    // Donor buffer
    const int donorLen = spectralEngine.getDonorLength();
    stream.writeInt (donorLen);
    if (donorLen > 0)
    {
        const auto& buf  = spectralEngine.getDonorBuffer();
        const int   numCh = buf.getNumChannels();
        stream.writeInt (numCh);
        for (int c = 0; c < numCh; ++c)
            stream.write (buf.getReadPointer (c), (size_t) donorLen * sizeof (float));
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

    // Donor buffer (optional — old presets without donor data load fine)
    if (stream.isExhausted()) return;
    const int donorLen = stream.readInt();
    if (donorLen <= 0 || donorLen > SpectralEngine::kMaxDonorSamples || stream.isExhausted()) return;

    const int numCh = stream.readInt();
    if (numCh <= 0 || numCh > 2) return;

    juce::AudioBuffer<float> buf (numCh, donorLen);
    for (int c = 0; c < numCh; ++c)
        stream.read (buf.getWritePointer (c), (int) ((size_t) donorLen * sizeof (float)));

    spectralEngine.setDonorData (buf, donorLen);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
