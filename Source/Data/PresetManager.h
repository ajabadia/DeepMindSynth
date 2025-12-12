#pragma once
#include <JuceHeader.h>

namespace data
{
    class PresetManager
    {
    public:
        PresetManager(juce::AudioProcessorValueTreeState& apvts);

        void savePreset(const juce::String& name);
        void loadPreset(const juce::File& file);
        
        juce::File getPresetsDirectory() const;
        juce::StringArray findPresets() const; // Returns list of file names

    private:
        juce::AudioProcessorValueTreeState& apvts;
        juce::File defaultDirectory;
    };
}
