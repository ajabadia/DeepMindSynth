#pragma once
#include <JuceHeader.h>
#include "Voice/SynthVoice.h"
#include "DSP/Effects/FxChain.h"
#include "DSP/Arpeggiator/Arpeggiator.h"

class DeepMindSynthAudioProcessor  : public juce::AudioProcessor
{
public:
    DeepMindSynthAudioProcessor();
    ~DeepMindSynthAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    // Public for Editor access
    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

private:
    juce::Synthesiser synthesiser;
    DeepMindDSP::FxChain fxChain;
    DeepMindDSP::Arpeggiator arpeggiator; // New instance
    
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeepMindSynthAudioProcessor)
};
