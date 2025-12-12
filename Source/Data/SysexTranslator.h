#pragma once
#include <JuceHeader.h>

namespace data
{
    class SysexTranslator
    {
    public:
        static bool parseSysex(const juce::MidiMessage& message, juce::AudioProcessorValueTreeState& apvts);
    
    private:
        static bool decodeDeepMindData(const juce::uint8* data, int size, juce::AudioProcessorValueTreeState& apvts);
    };
}
