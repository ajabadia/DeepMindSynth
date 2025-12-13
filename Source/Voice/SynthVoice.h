#pragma once
#include <JuceHeader.h>
#include <array>
#include "../DSP/Oscillators/DeepMindOsc.h"
#include "../DSP/Filters/MultiFilter.h"
#include "../DSP/Modulation/ModMatrix.h"
#include "../DSP/DriftGen.h"
#include "../DSP/Sequencing/ControlSequencer.h"

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
        static constexpr int MaxUnison = 12; // DeepMind 12 Hardware Limit
        DeepMindDSP::DeepMindOsc osc1[MaxUnison];
        DeepMindDSP::DeepMindOsc osc2[MaxUnison];
        DeepMindDSP::MultiFilter filter;
        DeepMindDSP::ModMatrix modMatrix;
        
        // Envelopes
        juce::ADSR envVca;
        juce::ADSR envVcf;
        juce::ADSR envMod;
        
        // LFOs (Manual Phase Accumulators)
        double lfo1Phase = 0.0;
        double lfo2Phase = 0.0;
        double currentSampleRate = 44100.0;
        
        float lfoOsc1Value = 0.0f;
        float lfoOsc2Value = 0.0f;
        
        // LFO Params
        float currentLfoOsc1Rate = 1.0f;
        float lfoOsc1Delay = 0.0f;
        float currentLfoOsc2Rate = 1.0f;
        float lfoOsc2Delay = 0.0f;
        
        int unisonMode = 1; // 1 = Off (1 voice), 2, 3, 4
        float currentUnisonDetune = 0.0f;
        
        // --- LFO Expansion ---
        enum class LfoShape { Sine, Triangle, Square, RampUp, RampDown, SampleHold, SampleGlide };
        LfoShape lfo1Shape = LfoShape::Sine;
        LfoShape lfo2Shape = LfoShape::Sine;
        
        float lfo1Random = 0.0f; // For S&H
        float lfo2Random = 0.0f;
        
        double noteSeconds = 0.0; // Time since Note On (for Delay)
        
        float currentVelocity = 0.0f;
        double currentBaseFrequency = 440.0;
        
        // Controllers
        float currentModWheel = 0.0f; // CC 1
        int currentNoteNumber = 60; // For KeyTracking
        
        float vcfKybdAmount = 0.0f; 
        
        // Envelope Curves (-1.0 to 1.0)
        float vcaCurve = 0.0f;
        float vcfCurve = 0.0f;
        float modCurve = 0.0f;
        
        float applyCurve(float value, float curve)
        {
            if (std::abs(curve) < 0.001f) return value;
            float factor = std::pow(4.0f, curve); 
            return std::pow(value, factor);
        }
        
        // Drift
        DeepMindDSP::DriftGen driftGen;
        float driftAmount = 0.0f;
        
        // Control Sequencer
        DeepMindDSP::ControlSequencer ctrlSeq;
        std::vector<std::atomic<float>*> seqStepParams; // Cache to avoid string allocs in process
    };
}
