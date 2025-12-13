#pragma once
#include <JuceHeader.h>
#include <vector>

namespace data
{
    class SysexTranslator
    {
    public:
        // Returns empty vector if invalid or not a program dump
        static std::vector<int> parseSysex(const juce::MidiMessage& message);
    
    private:
        static std::vector<int> decodePackedData(const juce::uint8* data, int size);
    };
}

