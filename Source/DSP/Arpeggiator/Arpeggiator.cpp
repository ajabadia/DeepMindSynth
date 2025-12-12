#include "Arpeggiator.h"

using namespace DeepMindDSP;

Arpeggiator::Arpeggiator()
{
}

Arpeggiator::~Arpeggiator()
{
}

void Arpeggiator::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    reset();
}

void Arpeggiator::reset()
{
    heldNotes.clear();
    currentArpNote = -1;
    sampleCounter = 0;
    currentStep = 0;
    // Recalculate timing
    samplesPerStep = (int)(sampleRate / rateHz);
}

void Arpeggiator::setMode(ArpMode mode)
{
    currentMode = mode;
}

void Arpeggiator::setRate(float rate)
{
    rateHz = rate;
    if (rateHz < 0.1f) rateHz = 0.1f;
    samplesPerStep = (int)(sampleRate / rateHz);
}

void Arpeggiator::setBypass(bool bypass)
{
    isBypassed = bypass;
    if (isBypassed)
    {
        // Panic/Clear notes if turning off?
        // Ideally send NoteOffs for any generated note
    }
}

void Arpeggiator::setOctaveRange(int range)
{
    octaveRange = range;
}

void Arpeggiator::processBlock(juce::MidiBuffer& midiMessages, int numSamples)
{
    if (isBypassed) return;

    // 1. Analyze incoming MIDI (Capture NoteOns/Offs)
    // We iterate the buffer, update our 'heldNotes' set, and CLEAR the buffer 
    // so the synth doesn't hear the original chords (unless in Chord mode).
    // Note: iterating and modifying the same buffer is tricky. 
    // Best practice: Read input to temp, Clear input, Write output.

    juce::MidiBuffer input = midiMessages; // Copy? Ideally iterate and remove.
    midiMessages.clear(); // We take control of the output

    for (const auto metadata : input)
    {
        auto msg = metadata.getMessage();
        handleMidiEvent(msg);
    }

    // 2. Clock Step Logic
    if (heldNotes.size() > 0)
    {
        int samplesRemaining = numSamples;
        int currentOffset = 0;

        while (samplesRemaining > 0)
        {
            if (sampleCounter >= samplesPerStep)
            {
                // Trigger Next Step
                sampleCounter = 0;
                
                // Old Note Off
                if (currentArpNote != -1)
                {
                    midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentArpNote, (float)0.0f), currentOffset);
                }

                // New Note On
                int note = getNextNote();
                if (note != -1)
                {
                    midiMessages.addEvent(juce::MidiMessage::noteOn(1, note, (float)1.0f), currentOffset);
                    currentArpNote = note;
                }
            }

            sampleCounter++;
            samplesRemaining--;
            currentOffset++;
        }
    }
    else
    {
        // If keys released, kill arp note
        if (currentArpNote != -1)
        {
             midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentArpNote, (float)0.0f), 0);
             currentArpNote = -1;
        }
         sampleCounter = 0; // Reset phase?
    }
}

void Arpeggiator::handleMidiEvent(const juce::MidiMessage& m)
{
    if (m.isNoteOn())
    {
        heldNotes.add(m.getNoteNumber());
    }
    else if (m.isNoteOff())
    {
        heldNotes.removeValue(m.getNoteNumber());
    }
}

int Arpeggiator::getNextNote()
{
    if (heldNotes.size() == 0) return -1;
    
    // Simple UP mode logic
    int index = currentStep % heldNotes.size();
    int note = heldNotes[index];
    
    currentStep++;
    return note;
}
