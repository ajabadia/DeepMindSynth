#include "FxChain.h"

using namespace DeepMindDSP;

FxChain::FxChain()
{
}

void FxChain::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    chorus.prepare(spec);
    delay.prepare(spec);
    reverb.prepare(spec);
    
    delay.setMaximumDelayInSamples(spec.sampleRate * 2.0); // 2 seconds max
}

void FxChain::reset()
{
    chorus.reset();
    delay.reset();
    reverb.reset();
}

void FxChain::process(juce::dsp::AudioBlock<float>& block)
{
    // 1. Chorus
    if (mixChorus > 0.0f)
    {
        // Simple dry/wet implementation for demo (JUCE Chorus handles mix internally actually, but let's be explicit if needed)
        // Actually juce::Chorus has setMix.
        juce::dsp::ProcessContextReplacing<float> context(block);
        chorus.process(context);
    }
    
    // 2. Delay (Simple Feedback Delay)
    if (mixDelay > 0.0f)
    {
        auto numSamples = block.getNumSamples();
        auto numChannels = block.getNumChannels();
        
        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            auto* samples = block.getChannelPointer(ch);
            for (size_t i = 0; i < numSamples; ++i)
            {
                float in = samples[i];
                float delayed = delay.popSample((int)ch);
                
                // Feedback
                delay.pushSample((int)ch, in + (delayed * delayFeedback));
                
                // Mix
                samples[i] = (in * (1.0f - mixDelay)) + (delayed * mixDelay);
            }
        }
    }

    // 3. Reverb
    if (mixReverb > 0.0f)
    {
        juce::dsp::ProcessContextReplacing<float> context(block);
        reverb.process(context);
    }
}

void FxChain::setChorusParams(float rate, float depth, float mix)
{
    chorus.setRate(rate);
    chorus.setDepth(depth);
    chorus.setMix(mix);
    mixChorus = mix; // Used to skip if 0
}

void FxChain::setDelayParams(float time, float feedback, float mix)
{
    delay.setDelay(time * (float)sampleRate); 
    delayFeedback = feedback;
    mixDelay = mix;
}

void FxChain::setReverbParams(float size, float damp, float mix)
{
    juce::dsp::Reverb::Parameters params;
    params.roomSize = size;
    params.damping = damp;
    params.wetLevel = mix;
    params.dryLevel = 1.0f - mix;
    params.width = 1.0f;
    reverb.setParameters(params);
    mixReverb = mix;
}
