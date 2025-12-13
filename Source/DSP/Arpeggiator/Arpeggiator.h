#pragma once
#include <JuceHeader.h>

namespace DeepMindDSP
{
    enum class ArpMode
    {
        Up,
        Down,
        UpDown,
        Random,
        Chord,
        Pattern
    };

    class Arpeggiator
    {
    public:
        Arpeggiator();
        ~Arpeggiator();

        void prepare(const juce::dsp::ProcessSpec& spec);
        void reset();

        struct PatternStep
        {
            int noteIndex = 0; // 0=Lowest Held, 1=Second, etc.
            int octave = 0;    // 0=Base, 1=+1 Oct, etc.
            int velocity = 127; // 0=Rest?
            float gate = 0.5f; // Step duration multiplier
        };

        void setPattern(int patternIndex);

        // Process incoming MIDI and generate arpeggiated MIDI
        void processBlock(juce::MidiBuffer& midiMessages, int numSamples);
        
        // Parameters
        void setMode(ArpMode mode);
        void setRate(float rate); // Hz or Sync division
        void setBypass(bool bypass);
        void setOctaveRange(int range);

    private:
        double sampleRate = 44100.0;
        bool isBypassed = true;
        ArpMode currentMode = ArpMode::Up;
        int octaveRange = 1;
        float rateHz = 4.0f; // Default 1/16th approx at 120bpm
        
        // Internal state
        int currentStep = 0;
        int samplesPerStep = 0;
        int sampleCounter = 0;
        
        // Note buffer (Notes held by keyboard)
        juce::SortedSet<int> heldNotes;
        
        // Currently playing note
        int currentArpNote = -1;
        
        void handleMidiEvent(const juce::MidiMessage& m);
        int getNextNote();
        
        // Pattern Data
        std::vector<std::vector<PatternStep>> patterns;
        int currentPatternIndex = 0;
    };
}
