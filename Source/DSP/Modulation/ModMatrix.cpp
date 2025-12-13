#include "ModMatrix.h"

using namespace DeepMindDSP;

ModMatrix::ModMatrix()
{
}

void ModMatrix::setSlot(int slotIndex, int srcIdx, int dstIdx, float amt)
{
    if (slotIndex >= 0 && slotIndex < 8)
    {
        slots[slotIndex].sourceIndex = srcIdx;
        slots[slotIndex].destIndex = dstIdx;
        slots[slotIndex].amount = amt;
    }
}

void ModMatrix::process(const ModSources& src, ModDestinations& dst)
{
    // Reset destinations - modulation is additive per frame
    dst.osc1Pitch = 0.0f;
    dst.osc1Pwm = 0.0f;
    dst.osc2Pitch = 0.0f;
    dst.vcfCutoff = 0.0f;
    dst.vcfRes = 0.0f;

    for (int i = 0; i < 8; ++i)
    {
        const auto& slot = slots[i];
        if (slot.sourceIndex == 0 || slot.destIndex == 0) continue; // Skip inactive

        float sourceValue = 0.0f;

        // Fetch Source (Matches index in PluginProcessor/ModMatrixEditor)
        // 1=LFO1, 2=LFO2, 3=EnvMod, 4=Velocity, 5=ModWheel, 6=KeyTrack
        switch (slot.sourceIndex)
        {
            case 1: sourceValue = src.lfo1; break;
            case 2: sourceValue = src.lfo2; break;
            case 3: sourceValue = src.envMod; break;
            case 4: sourceValue = src.velocity; break;
            case 5: sourceValue = src.modWheel; break;
            case 6: sourceValue = src.keyTrack; break;
            case 7: sourceValue = src.envVcf; break; // Added
            case 8: sourceValue = src.envVca; break; // Added
            case 9: sourceValue = src.ctrlSeq; break;
        }

        float amount = sourceValue * slot.amount;

        // Apply to Destination
        // 1=Osc1 Pitch, 2=Osc1 PWM, 3=Osc2 Pitch, 4=VCF Freq, 5=VCF Res
        switch (slot.destIndex)
        {
            case 1: dst.osc1Pitch += amount; break;
            case 2: dst.osc1Pwm   += amount; break;
            case 3: dst.osc2Pitch += amount; break;
            case 4: dst.vcfCutoff += amount; break;
            case 5: dst.vcfRes    += amount; break;
        }
    }
}
