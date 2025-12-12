#pragma once
#include <JuceHeader.h>

namespace DeepMindDSP
{
    struct ModSources
    {
        float lfo1 = 0.0f;
        float lfo2 = 0.0f;
        float envMod = 0.0f;
        float velocity = 0.0f;
        float modWheel = 0.0f;
        float keyTrack = 0.0f; // Added
    };

    struct ModDestinations
    {
        float osc1Pitch = 0.0f;
        float osc1Pwm = 0.0f;
        float osc2Pitch = 0.0f; // Added
        float vcfCutoff = 0.0f;
        float vcfRes = 0.0f;    // Added
    };

    struct ModSlot
    {
        int sourceIndex = 0; // 0=None, 1=LFO1, etc.
        int destIndex = 0;   // 0=None, 1=Osc1Pitch, etc.
        float amount = 0.0f;
    };

    class ModMatrix
    {
    public:
        ModMatrix();
        
        void process(const ModSources& src, ModDestinations& dst);
        
        // Update a slot
        void setSlot(int slotIndex, int srcIdx, int dstIdx, float amt);

    private:
        ModSlot slots[8];
    };
}
