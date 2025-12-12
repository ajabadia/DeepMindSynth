#include "SynthVoice.h"

using namespace voice;

SynthVoice::SynthVoice()
{
    // Initialize LFOs
    lfo1.initialise([](float x) { return std::sin(x); }); // Sine
    lfo2.initialise([](float x) { return std::sin(x); }); // Sine
}

void SynthVoice::setCurrentPlaybackSampleRate (double newRate)
{
    SynthesiserVoice::setCurrentPlaybackSampleRate(newRate);
    if (newRate > 0)
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = newRate;
        spec.maximumBlockSize = 512;
        spec.numChannels = 1;
        
        lfo1.prepare(spec);
        lfo2.prepare(spec);
        osc1.prepare(spec);
        osc2.prepare(spec);
        filter.prepare(spec); // Ensure filter is prepared too!
    }
}

bool SynthVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<juce::SynthesiserSound*>(sound) != nullptr;
}

void SynthVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition)
{
    currentVelocity = velocity;
    auto frequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    currentBaseFrequency = frequency;
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
    
    /* LFO LOGIC COMMENTED OUT FOR DEBUG
    // 1. Update Modulators (Control Rate: Start of Block)
    lfo1.setFrequency(lfo1Rate);
    lfo2.setFrequency(lfo2Rate);
    
    // Step LFOs 
    modSrc.lfo1 = lfo1.processSample(0.0f);
    modSrc.lfo2 = lfo2.processSample(0.0f);
    
    // Advance LFO phase for the rest of samples
    for(int k=1; k<numSamples; ++k) {
        lfo1.processSample(0.0f);
        lfo2.processSample(0.0f);
    }
    */
    
    // Fallback: No LFO
    DeepMindDSP::ModSources modSrc;
    modSrc.lfo1 = 0.0f;
    modSrc.lfo2 = 0.0f;

    // Envelope Mod (Sample one point)
    modSrc.envMod = envMod.getNextSample(); 
    for(int k=1; k<numSamples; ++k) envMod.getNextSample();
    
    modSrc.velocity = currentVelocity; 
    
    DeepMindDSP::ModDestinations modDst;
    modMatrix.process(modSrc, modDst);
    
    // 2. Apply Modulations
    // Pitch (Osc1 & Osc2)
    // Range: +/- 1 octave for 1.0 amount? Or 12 semitones.
    if (modDst.osc1Pitch != 0.0f)
    {
        float pitchRatio = std::pow(2.0f, (modDst.osc1Pitch * 12.0f) / 12.0f); 
        osc1.setFrequency(static_cast<float>(currentBaseFrequency) * pitchRatio);
    }
    else
    {
        osc1.setFrequency(currentBaseFrequency);
    }
    
    // Cutoff
    float modulatedCutoff = filter.getCutoff() + (modDst.vcfCutoff * 5000.0f); 
    if (modulatedCutoff < 20.0f) modulatedCutoff = 20.0f;
    if (modulatedCutoff > 20000.0f) modulatedCutoff = 20000.0f;
    filter.setCutoff(modulatedCutoff);
    
    // 3. Process Audio (Block)
    osc1.processBlock(subBlock);
    
    // 4. Filter
    filter.process(subBlock);
    
    // 5. VCA
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
    
    // --- LFOs ---
    auto* l1r = apvts->getRawParameterValue("lfo1_rate");
    auto* l1d = apvts->getRawParameterValue("lfo1_delay");
    if (l1r) lfo1Rate = *l1r;
    if (l1d) lfo1Delay = *l1d;
    
    auto* l2r = apvts->getRawParameterValue("lfo2_rate");
    auto* l2d = apvts->getRawParameterValue("lfo2_delay");
    if (l2r) lfo2Rate = *l2r;
    if (l2d) lfo2Delay = *l2d;
    
} // <--- Added closing brace

void SynthVoice::pitchWheelMoved(int newPitchWheelValue)
{
    // TODO: Implement pitch bend
}

void SynthVoice::controllerMoved(int controllerNumber, int newControllerValue)
{
    // TODO: Implement other controllers
}
