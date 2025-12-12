#pragma once
#include <JuceHeader.h>

namespace DeepMindDSP
{
    class FxChain
    {
    public:
        FxChain();
        
        void prepare(const juce::dsp::ProcessSpec& spec);
        void reset();
        
        void process(juce::dsp::AudioBlock<float>& block);
        
        // Effect parameters
        void setChorusParams(float rate, float depth, float mix);
        void setDelayParams(float time, float feedback, float mix);
        void setReverbParams(float size, float damp, float mix);

    private:
        enum { chorusIndex, delayIndex, reverbIndex };
        
        // Delay implementation as part of the implementation or custom
        // Using juce::dsp::DelayLine for simplicity, but it needs a wrapper to be in ProcessorChain easily if we want purely that.
        // Or we can just run them sequentially in process().
        
        juce::dsp::Chorus<float> chorus;
        juce::dsp::DelayLine<float> delay { 48000 }; // Max delay size roughly 1 sec
        juce::dsp::Reverb reverb;
        
        // Mix levels
        float mixChorus = 0.0f;
        float mixDelay = 0.0f;
        float mixReverb = 0.0f;
        
        // Delay Params
        float delayTime = 0.5f;
        float delayFeedback = 0.0f;
        double sampleRate = 44100.0;
        
        juce::LinearSmoothedValue<float> smoothMixChorus { 0.0f };
    };
}
