#include "SynthVoice.h"
#include <cmath>

using namespace voice;

SynthVoice::SynthVoice()
{
    // Initialize LFOs
    // lfoOsc1.initialise([](float x) { return std::sin(x); }); // Sine
    // lfoOsc2.initialise([](float x) { return std::sin(x); }); // Sine
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
        
        // LFOs don't need prepare, just sample rate
        currentSampleRate = newRate;
        
        // lfoOsc1.prepare(spec);
        // lfoOsc2.prepare(spec);
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
    
    // 1. Update Modulators (Manual Phase Accumulator)
    DeepMindDSP::ModSources modSrc;
    
    // Calculate Increments (rad/sample)
    double twoPi = juce::MathConstants<double>::twoPi;
    double inc1 = (currentLfoOsc1Rate * twoPi) / currentSampleRate;
    double inc2 = (currentLfoOsc2Rate * twoPi) / currentSampleRate;
    
    modSrc.lfo1 = (float)std::sin(lfo1Phase);
    modSrc.lfo2 = (float)std::sin(lfo2Phase);
    
    // Advance LFO phase for the whole block
    lfo1Phase += inc1 * numSamples;
    lfo2Phase += inc2 * numSamples;
    
    // Wrap Phase
    if (lfo1Phase > twoPi) lfo1Phase = std::fmod(lfo1Phase, twoPi);
    if (lfo2Phase > twoPi) lfo2Phase = std::fmod(lfo2Phase, twoPi);
    
    /* Original loop approach (too slow for control rate mod if we don't need FM)
    // Advance LFO phase for the rest of samples
    for(int k=1; k<numSamples; ++k) {
        lfoOsc1.processSample(0.0f);
        lfoOsc2.processSample(0.0f);
    } 
    */
    
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
    
    // Pitch Osc 2
    if (modDst.osc2Pitch != 0.0f)
    {
        float pitchRatio = std::pow(2.0f, (modDst.osc2Pitch * 12.0f) / 12.0f); 
        osc2.setFrequency(static_cast<float>(currentBaseFrequency) * pitchRatio);
    }
    else
    {
        osc2.setFrequency(currentBaseFrequency);
    }
    
    // Cutoff
    float modulatedCutoff = filter.getCutoff() + (modDst.vcfCutoff * 5000.0f); 
    if (modulatedCutoff < 20.0f) modulatedCutoff = 20.0f;
    if (modulatedCutoff > 20000.0f) modulatedCutoff = 20000.0f;
    filter.setCutoff(modulatedCutoff);
    
    // 3. Process Audio (Block)
    osc1.processBlock(subBlock);
    osc2.processBlock(subBlock);
    
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
    if (l1r) currentLfoOsc1Rate = *l1r;
    if (l1d) lfoOsc1Delay = *l1d;
    
    auto* l2r = apvts->getRawParameterValue("lfo2_rate");
    auto* l2d = apvts->getRawParameterValue("lfo2_delay");
    if (l2r) currentLfoOsc2Rate = *l2r;
    if (l2d) lfoOsc2Delay = *l2d;
    
} // <--- Added closing brace

void SynthVoice::pitchWheelMoved(int newPitchWheelValue)
{
    // TODO: Implement pitch bend
}

void SynthVoice::controllerMoved(int controllerNumber, int newControllerValue)
{
    // TODO: Implement other controllers
}
