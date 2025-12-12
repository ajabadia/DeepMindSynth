#pragma once
#include <JuceHeader.h>

namespace DeepMindDSP
{
    enum class FilterType
    {
        Jupiter, // Ladder 24dB
        MS20,    // StateVariable TPT + Drive
        Acid303  // Distorted Ladder
    };

    class MultiFilter
    {
    public:
        MultiFilter();
        ~MultiFilter();

        void prepare(const juce::dsp::ProcessSpec& spec);
        void reset();

        void process(juce::dsp::AudioBlock<float>& block);

        void setType(FilterType type);
        void setCutoff(float frequency);
        float getCutoff() const; // Getter added
        void setResonance(float resonance);
        void setDrive(float drive);

    private:
        FilterType currentType = FilterType::Jupiter;
        float currentCutoff = 1000.0f;
        
        // Filter Instances
        juce::dsp::LadderFilter<float> ladderFilter;
        juce::dsp::StateVariableTPTFilter<float> svFilter;
        
        // Saturation stages
        juce::dsp::WaveShaper<float> distortionSat; // For Acid/MS20 drive
    };
}
