#include "SysexTranslator.h"

namespace data
{
    std::vector<int> SysexTranslator::parseSysex(const juce::MidiMessage& message)
    {
        auto data = message.getSysExData();
        int size = message.getSysExDataSize();

        // 1. Validate Header
        // DeepMind Header: F0 00 20 32 20 0d ...
        // 00 20 32 = Behringer
        // 20 = DeepMind 12
        // 0d = Device ID (variable 0-15)
        
        if (size < 10) return {};
        if (data[0] != 0xF0) return {}; // Already handled by getSysExData? No, data excludes F0 usually? 
        // JUCE getSysExData() includes the entire raw message including F0/F7? 
        // Documentation says "pointer to the sysex data". Usually includes F0.
        // Let's verify standard JUCE behavior: getSysExData includes everything.
        
        if (data[1] != 0x00 || data[2] != 0x20 || data[3] != 0x32 || data[4] != 0x20) return {};
        
        // Byte 5 is Device ID. Ignore for now.
        // Byte 6 is Message Type.
        // 02 = Program Dump
        // 04 = Edit Buffer Dump
        juce::uint8 msgType = data[6];
        if (msgType != 0x02 && msgType != 0x04) return {};
        
        // Byte 7 is Protocol Version (e.g. 06).
        // For Program Dump (02): Byte 8=Bank, 9=Prog, Data starts at 10.
        // For Edit Buffer (04): Byte 8=Protocol?? No, "04 Edit Buffer... 06 Protocol... Data".
        // Data starts at 8? 
        // Manual:
        // Prog Dump Response: F0 ... 02 (Msg) 06 (Proto) 0b (Bank) pp (Prog) ... Data ... F7
        // Edit Buff Response: F0 ... 04 (Msg) 06 (Proto) ... Data ... F7
        
        int dataStart = 0;
        if (msgType == 0x02) dataStart = 10;
        else if (msgType == 0x04) dataStart = 8;
        
        // Data runs until F7 (last byte).
        int dataEnd = size - 1; 
        if (data[dataEnd] == 0xF7) {} // Valid
        else dataEnd = size; // Maybe stripped?
        
        if (dataEnd <= dataStart) return {};
        
        return decodePackedData(data + dataStart, dataEnd - dataStart);
    }

    std::vector<int> SysexTranslator::decodePackedData(const juce::uint8* input, int inputSize)
    {
        // Packed Format: 8 input bytes -> 7 output bytes.
        // Input Byte 0 is Guidemap.
        // Input Bytes 1-7 are Data (low 7 bits).
        
        std::vector<int> output;
        output.reserve(inputSize); // Approx
        
        int offset = 0;
        while (offset + 1 < inputSize) // Need at least header + 1
        {
            // Usually chunks of 8. If less than 8 left, process what we have.
            // But valid dumps should be multiples of 8 effectively?
            // "278 7-bit message bytes" matches 242 real bytes. 34 blocks of 8? 34*8 = 272. +6 extra.
            
            juce::uint8 header = input[offset];
            offset++;
            
            // Process up to 7 following bytes, or until end
            for (int i = 0; i < 7; ++i)
            {
                if (offset >= inputSize) break;
                
                juce::uint8 dataByte = input[offset];
                offset++;
                
                // Reconstruct MSB
                // Bit i of header is MSB for this data byte
                bool msb = (header >> i) & 1;
                
                int fullValue = dataByte | (msb ? 0x80 : 0x00);
                output.push_back(fullValue);
            }
        }
        
        return output;
    }
}
