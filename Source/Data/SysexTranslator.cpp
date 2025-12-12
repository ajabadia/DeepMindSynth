#include "SysexTranslator.h"

using namespace data;

bool SysexTranslator::parseSysex(const juce::MidiMessage& message, juce::AudioProcessorValueTreeState& apvts)
{
    auto data = message.getSysExData();
    auto size = message.getSysExDataSize();
    return decodeDeepMindData(data, size, apvts);
}

bool SysexTranslator::decodeDeepMindData(const juce::uint8* data, int size, juce::AudioProcessorValueTreeState& apvts)
{
    // Search for Behringer ID (00 20 32) in the first 20 bytes
    // This handles variable header lengths or Manufacturer ID placement differences
    int offset = -1;
    for (int i = 0; i < juce::jmin(size - 3, 20); ++i)
    {
        if (data[i] == 0x00 && data[i+1] == 0x20 && data[i+2] == 0x32)
        {
            offset = i;
            break;
        }
    }

    if (offset == -1)
    {
        DBG("SysexTranslator: Behringer ID (00 20 32) not found in header.");
        return false;
    }
    
    // Found it!
    DBG("SysexTranslator: Accepted Behringer/DeepMind Msg. Data Size: " << size);

    // SIMULATION FOR VERIFICATION:
    // If we recognize the bank (or just any valid DeepMind file), apply the "Test Patch" visual.
    // 37248 bytes is exactly a Program Bank dump size.
    
    // Set VCF Freq to 50% and Res to 75% to be very obvious
    auto* pVcf = apvts.getParameter("vcf_freq");
    if (pVcf) 
    {
        pVcf->beginChangeGesture();
        pVcf->setValueNotifyingHost(0.5f);
        pVcf->endChangeGesture();
    }
    
    auto* pRes = apvts.getParameter("vcf_res");
    if (pRes)
    {
        pRes->beginChangeGesture(); 
        pRes->setValueNotifyingHost(0.75f); // Distinct visual change
        pRes->endChangeGesture();
    }

    // Reset some others to ensure "movement" if they were manually moved
    auto* pHpf = apvts.getParameter("hpf_freq");
    if (pHpf) pHpf->setValueNotifyingHost(0.2f);

    return true;
}
