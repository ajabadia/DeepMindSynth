#pragma once
#include <JuceHeader.h>

namespace DeepMindDSP
{
    class DeepMindOsc
    {
    public:
        DeepMindOsc();
        ~DeepMindOsc();

        void prepare(const juce::dsp::ProcessSpec& spec);
        void reset();
        
        // Main processing block
        void processBlock(juce::dsp::AudioBlock<float>& block);

        // Parameters sets frequency
        void setFrequency(float frequency);
        float getFrequency() const { return currentFrequency; }

        // Parameters
        void setType(int type);
        void setShape(float shape);
        void setColor(float color);

    private:
        // Internal DSP objects
        juce::dsp::Oscillator<float> oscSaw { [](float x) { return x / juce::MathConstants<float>::pi; }}; // Naive phasor
        juce::dsp::Oscillator<float> oscPulse { [](float x) { return x < 0.0f ? -1.0f : 1.0f; }};
        
        // Parameters
        float sawLevel = 0.5f;
        float pulseLevel = 0.5f;
        float pwmDepth = 0.0f;
        float currentPwm = 0.5f; // Target width (0.0 - 1.0)
        float currentFrequency = 440.0f;
        
        float sampleRate = 44100.0f;
    };
}
