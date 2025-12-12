#include "PresetManager.h"

namespace data
{
    PresetManager::PresetManager(juce::AudioProcessorValueTreeState& state)
        : apvts(state)
    {
        // Default location: User Documents/DeepMindSynth/Presets
        auto docs = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
        defaultDirectory = docs.getChildFile("DeepMindSynth").getChildFile("Presets");
        
        if (!defaultDirectory.exists())
            defaultDirectory.createDirectory();
    }

    void PresetManager::savePreset(const juce::String& name)
    {
        auto file = defaultDirectory.getChildFile(name).withFileExtension(".xml");
        auto state = apvts.copyState();
        std::unique_ptr<juce::XmlElement> xml(state.createXml());
        xml->writeTo(file);
    }

    void PresetManager::loadPreset(const juce::File& file)
    {
        if (file.existsAsFile())
        {
            auto xml = juce::XmlDocument::parse(file);
            if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
            {
                apvts.replaceState(juce::ValueTree::fromXml(*xml));
            }
        }
    }

    juce::File PresetManager::getPresetsDirectory() const
    {
        return defaultDirectory;
    }

    juce::StringArray PresetManager::findPresets() const
    {
        juce::StringArray results;
        auto files = defaultDirectory.findChildFiles(juce::File::findFiles, false, "*.xml");
        for (const auto& f : files)
            results.add(f.getFileNameWithoutExtension());
        return results;
    }
}
