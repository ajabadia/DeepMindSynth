#include "Arpeggiator.h"

using namespace DeepMindDSP;

Arpeggiator::Arpeggiator()
{
    // Define Default Patterns
    // Pattern 0: Up 4 (Basic)
    std::vector<PatternStep> p0;
    p0.push_back({0, 0, 127, 0.5f});
    p0.push_back({1, 0, 127, 0.5f});
    p0.push_back({2, 0, 127, 0.5f});
    p0.push_back({3, 0, 127, 0.5f});
    patterns.push_back(p0);

    // Pattern 1: UpDown 8
    std::vector<PatternStep> p1;
    p1 = {{0,0}, {1,0}, {2,0}, {3,0}, {3,0}, {2,0}, {1,0}, {0,0}};
    patterns.push_back(p1);

    // Pattern 2: Octave Jump
    std::vector<PatternStep> p2;
    p2 = {{0,0}, {0,1}, {1,0}, {1,1}, {2,0}, {2,1}, {3,0}, {3,1}};
    patterns.push_back(p2);
    
    // Pattern 3: Random-ish
    std::vector<PatternStep> p3;
    p3 = {{0,0}, {2,0}, {1,0}, {3,0}, {0,1}, {2,1}, {1,1}, {3,1}};
    patterns.push_back(p3);
    
    // Pattern 4: Rest Rhythms
    std::vector<PatternStep> p4;
    p4 = {{0,0}, {0,0,0}, {1,0}, {1,0,0}, {2,0}, {2,0,0}, {3,0}, {3,0,0}}; // Vel 0 = Rest logic needed
    patterns.push_back(p4);
    
    // More can be added...
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

void Arpeggiator::setPattern(int patternIndex)
{
    if (patternIndex >= 0 && patternIndex < patterns.size())
        currentPatternIndex = patternIndex;
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
    
    int numHeld = heldNotes.size();
    int note = -1;
    
    if (currentMode == ArpMode::Pattern)
    {
        const auto& pat = patterns[currentPatternIndex];
        auto step = pat[currentStep % pat.size()];
        
        // Map pattern note index to held notes
        // Wrap index if held notes < requested index
        int idx = step.noteIndex % numHeld;
        
        int baseNote = heldNotes[idx];
        note = baseNote + (step.octave * 12);
        
        // Velocity/Rest check
        if (step.velocity == 0) note = -1;
        
        currentStep++;
        // Do NOT reset currentStep based on heldNotes size for Pattern mode, 
        // rely on modulo of pattern size.
    }
    else
    {
        // ... existing simple logic or expanded logic for Up/Down/Random etc ..
        // Simple UP mode logic fallback
        int index = currentStep % numHeld;
        note = heldNotes[index];
        currentStep++;
    }
    
    return note;
}
