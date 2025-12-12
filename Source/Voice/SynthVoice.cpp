#include "SynthVoice.h"

using namespace voice;

SynthVoice::SynthVoice()
{
}

bool SynthVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<juce::SynthesiserSound*>(sound) != nullptr;
}

void SynthVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition)
{
    currentVelocity = velocity;
    auto frequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    osc1.setFrequency(frequency);
    osc2.setFrequency(frequency); // Usually with some detune
    
    envVca.noteOn();
    envVcf.noteOn();
    envMod.noteOn();
}

void SynthVoice::stopNote(float velocity, bool allowTailOff)
{
    envVca.noteOff();
    envVcf.noteOff();
    envMod.noteOff();
    
    if (!allowTailOff || !envVca.isActive())
        clearCurrentNote();
}

void SynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!isVoiceActive()) return;

    juce::dsp::AudioBlock<float> block(outputBuffer);
    auto subBlock = block.getSubBlock(startSample, numSamples);
    
    // 1. Update Modulations (LFO, Envelopes)
    // Needs current values. Ideally LFOs process here.
    // Simplifying: Use current env value.
    DeepMindDSP::ModSources modSrc;
    modSrc.lfo1 = 0.0f; 
    modSrc.lfo2 = 0.0f;
    modSrc.envMod = envMod.getNextSample(); 
    modSrc.velocity = currentVelocity; // Correct
    modSrc.modWheel = 0.0f; 
    modSrc.keyTrack = 0.0f; 
    
    DeepMindDSP::ModDestinations modDst;
    modMatrix.process(modSrc, modDst);
    
    // 2. Process Oscillators
    osc1.processBlock(subBlock);
    
    // 3. Process Filter
    // Base cutoff + mod
    float modulatedCutoff = filter.getCutoff() + (modDst.vcfCutoff * 5000.0f); // Scale 0-1 to Hz range
    if (modulatedCutoff < 20.0f) modulatedCutoff = 20.0f;
    if (modulatedCutoff > 20000.0f) modulatedCutoff = 20000.0f;

    filter.setCutoff(modulatedCutoff);
    filter.process(subBlock);
    
    // 4. Process VCA (Apply envelope)
    envVca.applyEnvelopeToBuffer(outputBuffer, startSample, numSamples);
    
    // Check if note finished
    if (!envVca.isActive())
        clearCurrentNote();
}

void SynthVoice::updateParameters(juce::AudioProcessorValueTreeState* apvts)
{
    if (apvts == nullptr) return;

    // --- Oscillators ---
    auto* dco1Pwm = apvts->getRawParameterValue("dco1_pwm");
    if (dco1Pwm) osc1.setShape(*dco1Pwm);
    
    // --- Filters ---
    auto* cutoff = apvts->getRawParameterValue("vcf_freq");
    if (cutoff) filter.setCutoff(*cutoff);
    
    auto* res = apvts->getRawParameterValue("vcf_res");
    if (res) filter.setResonance(*res);
    
    auto* type = apvts->getRawParameterValue("vcf_type");
    if (type) filter.setType(static_cast<DeepMindDSP::FilterType>((int)*type));

    // --- Envelopes (Using setParameters for ADSR) ---
    juce::ADSR::Parameters vcaParams;
    vcaParams.attack = *apvts->getRawParameterValue("vca_attack");
    vcaParams.decay = *apvts->getRawParameterValue("vca_decay");
    vcaParams.sustain = *apvts->getRawParameterValue("vca_sustain");
    vcaParams.release = *apvts->getRawParameterValue("vca_release");
    envVca.setParameters(vcaParams);
    
    // --- Mod Matrix ---
    // Iterate 8 slots
    for (int i = 0; i < 8; ++i)
    {
        juce::String prefix = "mod_slot_" + juce::String(i + 1);
        auto* src = apvts->getRawParameterValue(prefix + "_src");
        auto* dst = apvts->getRawParameterValue(prefix + "_dst");
        auto* amt = apvts->getRawParameterValue(prefix + "_amt");
        
        if (src && dst && amt)
        {
            modMatrix.setSlot(i, (int)*src, (int)*dst, *amt);
        }
    }
} // <--- Added closing brace

void SynthVoice::pitchWheelMoved(int newPitchWheelValue)
{
    // TODO: Implement pitch bend
}

void SynthVoice::controllerMoved(int controllerNumber, int newControllerValue)
{
    // TODO: Implement other controllers
}
