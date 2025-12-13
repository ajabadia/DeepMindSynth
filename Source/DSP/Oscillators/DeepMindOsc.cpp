#include "DeepMindOsc.h"

using namespace DeepMindDSP;

DeepMindOsc::DeepMindOsc()
{
    // Initialize with band-limited waveforms if possible, or basic shapes
    // JUCE Oscillator uses a lookup table or function.
    // We will use standard mathematical functions here for simplicity, 
    // but in a real synth we'd use PolyBLEP.
    // For now, let's use the lambdas defined in the header or redefine them here for better logic.
    
    // Sawtooth
    oscSaw.initialise ([](float x)
    {
        return juce::jmap (x, -juce::MathConstants<float>::pi, juce::MathConstants<float>::pi, -1.0f, 1.0f);
    });

    // Pulse (Dynamic width handled in process)
    oscPulse.initialise ([](float x)
    {
        return x < 0.0f ? -1.0f : 1.0f; 
    });
}

DeepMindOsc::~DeepMindOsc()
{
}

void DeepMindOsc::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    oscSaw.prepare(spec);
    oscPulse.prepare(spec);
}

void DeepMindOsc::reset()
{
    oscSaw.reset();
    oscPulse.reset();
}

void DeepMindOsc::setType(int type)
{
    // DCO1 always has Saw + Pulse available mixed.
    // DCO2 is Pulse only (usually). 
    // We might reuse this class for both if we just turn down levels.
}

void DeepMindOsc::setShape(float shape)
{
    // Map shape to PWM for Pulse
    currentPwm = juce::jlimit(0.01f, 0.99f, shape);
}

void DeepMindOsc::setColor(float color)
{
    // Map color to Tone Mod or other nuances
}


void DeepMindOsc::processBlock(juce::dsp::AudioBlock<float>& block)
{
    for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
    {
        auto* samples = block.getChannelPointer(channel);
        
        // Creating a temporary block for oscillators allows them to process efficiently
        // But since we need to mix Saw and Pulse per sample with PWM, 
        // iterating sample-by-sample is safer for now to handle PWM modulation correctly.
        
        for (size_t i = 0; i < block.getNumSamples(); ++i)
        {
            float saw = oscSaw.processSample(0.0f);
            
            // Pulse with PWM:
            // Standard naive pulse is (phase < pwm ? 1 : -1).
            // JUCE Oscillator doesn't support dynamic PWM easily in the lambda without capturing state.
            // We will approximate it by modifying the output of a standard square wave or using a custom phasor.
            // For this iteration, let's use the Saw to derive Pulse.
            // Pulse = (Saw > PWM_Thresh) ? 1 : -1
            
            float pulse = (saw > (currentPwm * 2.0f - 1.0f)) ? 1.0f : -1.0f;
            
            // Mix
            float mixed = (saw * sawLevel) + (pulse * pulseLevel);
            
            samples[i] += mixed; // Add to existing (polyphony sum)
        }
    }
}

void DeepMindOsc::setFrequency(float frequency)
{
    currentFrequency = frequency;
    oscSaw.setFrequency(frequency);
    oscPulse.setFrequency(frequency); // Not used directly if we derive from Saw, but good practice
}

