#include "PresetManager.h"

const char* const PresetManager::kPresetExtension = ".freecoder";

//==============================================================================
// Factory preset definitions (parameters only — donor buffer is preserved)
//==============================================================================
const std::vector<PresetManager::FactoryPreset>& PresetManager::factoryList()
{
    //                     name              morph  grain  formant  scatter  drywet   pitch
    static const std::vector<FactoryPreset> list
    {
        { "Init",           0.5f,  0.00f,  0.50f,  0.30f,  0.80f,   0.0f },
        { "Shimmer Pad",    1.0f,  0.00f,  0.70f,  0.10f,  1.00f,   0.0f },
        { "Grain Cloud",    0.5f,  0.90f,  0.30f,  0.80f,  0.90f,   0.0f },
        { "Formant Choir",  0.6f,  0.10f,  1.00f,  0.20f,  0.90f,   0.0f },
        { "Glitch Freeze",  1.0f,  0.40f,  0.20f,  1.00f,  1.00f,   0.0f },
        { "Deep Freeze",    1.0f,  0.00f,  0.80f,  0.00f,  1.00f,   0.0f },
        { "Subtle Texture", 0.3f,  0.20f,  0.40f,  0.30f,  0.60f,   0.0f },
        { "Octave Up",      0.7f,  0.00f,  0.60f,  0.10f,  0.90f,  12.0f },
        { "Octave Down",    0.7f,  0.00f,  0.60f,  0.10f,  0.90f, -12.0f },
        { "Scatter Storm",  0.4f,  0.70f,  0.20f,  1.00f,  0.80f,   0.0f },
    };
    return list;
}

int PresetManager::numFactory() const { return (int) factoryList().size(); }

//==============================================================================
juce::File PresetManager::getUserPresetsDir()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("Ament Audio")
               .getChildFile ("FREECODER")
               .getChildFile ("Presets");
}

//==============================================================================
PresetManager::PresetManager (juce::AudioProcessorValueTreeState& a, SpectralEngine& e)
    : apvts (a), engine (e)
{
    refreshUserPresets();
}

void PresetManager::refreshUserPresets()
{
    userPresets.clear();
    auto dir = getUserPresetsDir();
    if (dir.isDirectory())
    {
        auto files = dir.findChildFiles (juce::File::findFiles, false,
                                         "*" + juce::String (kPresetExtension));
        files.sort();
        for (auto& f : files)
            userPresets.add (f);
    }
}

int PresetManager::getNumPresets() const { return numFactory() + userPresets.size(); }

//==============================================================================
void PresetManager::nextPreset()
{
    loadPresetAtIndex ((currentIndex + 1) % getNumPresets());
}

void PresetManager::previousPreset()
{
    loadPresetAtIndex ((currentIndex - 1 + getNumPresets()) % getNumPresets());
}

void PresetManager::loadPresetAtIndex (int index)
{
    index = juce::jlimit (0, getNumPresets() - 1, index);
    currentIndex = index;

    if (index < numFactory())
        applyFactory (index);
    else
        applyUserFile (userPresets[index - numFactory()]);
}

//==============================================================================
void PresetManager::applyFactory (int i)
{
    const auto& p = factoryList()[(size_t) i];
    currentName   = p.name;

    // Build a ValueTree in the format APVTS expects and restore it
    juce::ValueTree state ("Parameters");
    auto addParam = [&] (const char* id, float value)
    {
        juce::ValueTree child ("PARAM");
        child.setProperty ("id",    juce::String (id), nullptr);
        child.setProperty ("value", value,             nullptr);
        state.appendChild (child, nullptr);
    };

    addParam ("morph",      p.morph);
    addParam ("grain",      p.grain);
    addParam ("formant",    p.formant);
    addParam ("scatter",    p.scatter);
    addParam ("drywet",     p.drywet);
    addParam ("recTrigger", 0.0f);
    addParam ("engage",     0.0f);
    addParam ("pitch",      p.pitch);
    addParam ("reverse",    0.0f);

    apvts.replaceState (state);
    // Donor buffer intentionally preserved — user keeps their recording
}

//==============================================================================
void PresetManager::applyUserFile (const juce::File& f)
{
    juce::MemoryBlock data;
    if (!f.loadFileAsData (data) || data.isEmpty()) return;

    currentName = f.getFileNameWithoutExtension();

    juce::MemoryInputStream stream (data.getData(), data.getSize(), false);

    // APVTS XML
    const int xmlLen = stream.readInt();
    if (xmlLen <= 0 || xmlLen >= (int) data.getSize()) return;

    juce::MemoryBlock xmlBlock;
    stream.readIntoMemoryBlock (xmlBlock, xmlLen);
    juce::String xmlStr (static_cast<const char*> (xmlBlock.getData()), (size_t) xmlLen);

    if (auto xml = juce::XmlDocument::parse (xmlStr))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));

    // Donor buffer (optional — old presets without donor load cleanly)
    if (stream.isExhausted()) return;
    const int donorLen = stream.readInt();
    if (donorLen <= 0 || donorLen > SpectralEngine::kMaxDonorSamples || stream.isExhausted()) return;

    const int numCh = stream.readInt();
    if (numCh <= 0 || numCh > 2) return;

    juce::AudioBuffer<float> buf (numCh, donorLen);
    for (int c = 0; c < numCh; ++c)
        stream.read (buf.getWritePointer (c), (int) ((size_t) donorLen * sizeof (float)));

    engine.setDonorData (buf, donorLen);
}

//==============================================================================
void PresetManager::saveToFile (const juce::File& f)
{
    juce::MemoryOutputStream stream;

    // APVTS XML
    auto state  = apvts.copyState();
    auto xml    = state.createXml();
    auto xmlStr = xml->toString (juce::XmlElement::TextFormat().singleLine());
    stream.writeInt ((int) xmlStr.getNumBytesAsUTF8());
    stream.write (xmlStr.toUTF8(), xmlStr.getNumBytesAsUTF8());

    // Donor buffer
    const int donorLen = engine.getDonorLength();
    stream.writeInt (donorLen);
    if (donorLen > 0)
    {
        const auto& buf   = engine.getDonorBuffer();
        const int   numCh = buf.getNumChannels();
        stream.writeInt (numCh);
        for (int c = 0; c < numCh; ++c)
            stream.write (buf.getReadPointer (c), (size_t) donorLen * sizeof (float));
    }

    f.getParentDirectory().createDirectory();
    f.replaceWithData (stream.getData(), stream.getDataSize());
}

void PresetManager::promptSavePreset (juce::Component* parent)
{
    juce::ignoreUnused (parent);
    auto dir = getUserPresetsDir();
    dir.createDirectory();

    fileChooser = std::make_unique<juce::FileChooser> (
        "Save Preset",
        dir.getChildFile (currentName + kPresetExtension),
        "*" + juce::String (kPresetExtension));

    fileChooser->launchAsync (
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& chooser)
        {
            auto result = chooser.getResult();
            if (result == juce::File{}) return;

            auto f = result.withFileExtension (kPresetExtension);
            saveToFile (f);
            currentName = f.getFileNameWithoutExtension();
            refreshUserPresets();

            // Update index to point at the newly saved preset
            for (int i = 0; i < userPresets.size(); ++i)
                if (userPresets[i] == f)
                    currentIndex = numFactory() + i;
        });
}
