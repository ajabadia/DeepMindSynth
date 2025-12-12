#pragma once
#include <JuceHeader.h>
#include "../DSP/Oscillators/DeepMindOsc.h"
#include "../DSP/Filters/MultiFilter.h"
#include "../DSP/Modulation/ModMatrix.h"

namespace voice
{
    struct SynthSound : public juce::SynthesiserSound
    {
        bool appliesToNote (int) override { return true; }
        bool appliesToChannel (int) override { return true; }
    };

    class SynthVoice : public juce::SynthesiserVoice
    {
    public:
        SynthVoice();
        
        bool canPlaySound(juce::SynthesiserSound*) override;
        void setCurrentPlaybackSampleRate (double newRate) override;
        void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition) override;
        void stopNote(float velocity, bool allowTailOff) override;
        void pitchWheelMoved(int newPitchWheelValue) override;
        void controllerMoved(int controllerNumber, int newControllerValue) override;
        void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;
        
        // Parameter update
        void updateParameters(juce::AudioProcessorValueTreeState* apvts);

    private:
        DeepMindDSP::DeepMindOsc osc1;
        DeepMindDSP::DeepMindOsc osc2;
        DeepMindDSP::MultiFilter filter;
        DeepMindDSP::ModMatrix modMatrix;
        
        // Envelopes
        juce::ADSR envVca;
        juce::ADSR envVcf;
        juce::ADSR envMod;
        
        // LFOs
        juce::dsp::Oscillator<float> lfo1;
        juce::dsp::Oscillator<float> lfo2;
        float lfo1Value = 0.0f;
        float lfo2Value = 0.0f;
        
        // LFO Params
        float lfo1Rate = 1.0f;
        float lfo1Delay = 0.0f;
        float lfo2Rate = 1.0f;
        float lfo2Delay = 0.0f;
        
        float currentVelocity = 0.0f;
        double currentBaseFrequency = 440.0;
    };
}
