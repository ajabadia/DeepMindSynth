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
        for(auto& o : osc1) o.prepare(spec);
        for(auto& o : osc2) o.prepare(spec);
        filter.prepare(spec); 
        
        driftGen.prepare(newRate);
        ctrlSeq.prepare(newRate);
    }
}

bool SynthVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<juce::SynthesiserSound*>(sound) != nullptr;
}

void SynthVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition)
{
    currentVelocity = velocity;
    currentNoteNumber = midiNoteNumber;
    auto frequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    currentBaseFrequency = frequency;
    
    // Reset LFO State (Key Sync assumed ON)
    lfo1Phase = 0.0;
    lfo2Phase = 0.0;
    lfo1Random = 0.0f;
    lfo2Random = 0.0f;
    noteSeconds = 0.0;
    
    // Spread Logic
    // Manual: "+/- 50 cents spread over voices"
    // currentUnisonDetune is 0..1 representing 0..50 cents (0.5 semitones)
    float maxDetuneSemitones = currentUnisonDetune * 0.5f; 
    
    for (int i = 0; i < MaxUnison; ++i)
    {
        float ratio = 1.0f;
        if (i < unisonMode && unisonMode > 1)
        {
            // Generic Spread Formula (Even/Odd compatible)
            // Maps i=[0..N-1] to [-1..1]
            float norm = (float)i / (float)(unisonMode - 1); // 0.0 to 1.0
            float pan = norm * 2.0f - 1.0f;                  // -1.0 to 1.0
            
            float spread = pan * maxDetuneSemitones;
            ratio = std::pow(2.0f, spread / 12.0f);
        }
        
        osc1[i].setFrequency(frequency * ratio);
        osc2[i].setFrequency(frequency * ratio); 
    }
    
    envVca.noteOn();
    envVcf.noteOn();
    envMod.noteOn();
    ctrlSeq.reset();
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
    bool lfo1Wrap = false;
    bool lfo2Wrap = false;
    
    lfo1Phase += inc1 * numSamples;
    lfo2Phase += inc2 * numSamples;
    
    // Wrap Phase & Update S&H
    if (lfo1Phase > twoPi) { 
        lfo1Phase = std::fmod(lfo1Phase, twoPi); 
        lfo1Wrap = true; 
        lfo1Random = (juce::Random::getSystemRandom().nextFloat() * 2.0f) - 1.0f;
    }
    if (lfo2Phase > twoPi) { 
        lfo2Phase = std::fmod(lfo2Phase, twoPi); 
        lfo2Wrap = true; 
        lfo2Random = (juce::Random::getSystemRandom().nextFloat() * 2.0f) - 1.0f;
    }
    
    // LFO Wave Gen Helper
    auto getLfoVal = [&](double phase, LfoShape shape, float randomVal) -> float {
        float p = (float)phase;
        float pi = juce::MathConstants<float>::pi;
        
        switch(shape) {
            case LfoShape::Sine: return std::sin(p);
            case LfoShape::Triangle: return 1.0f - (2.0f * std::acos(std::sin(p)) / pi); // Or other tri approx
            case LfoShape::Square: return p < pi ? 1.0f : -1.0f;
            case LfoShape::RampUp: return (p / pi) - 1.0f; // -1 to 1
            case LfoShape::RampDown: return 1.0f - (p / pi); // 1 to -1
            case LfoShape::SampleHold: return randomVal;
            case LfoShape::SampleGlide: return randomVal; // TODO: Glide logic
            default: return std::sin(p);
        }
    };

    modSrc.lfo1 = getLfoVal(lfo1Phase, lfo1Shape, lfo1Random);
    modSrc.lfo2 = getLfoVal(lfo2Phase, lfo2Shape, lfo2Random);
    
    // LFO Delay Logic (Simple Gating)
    // If delay > 0 and time < delay, suppress LFO
    noteSeconds += (double)numSamples / currentSampleRate;
    
    if (lfoOsc1Delay > 0.0f && noteSeconds < lfoOsc1Delay) modSrc.lfo1 = 0.0f;
    if (lfoOsc2Delay > 0.0f && noteSeconds < lfoOsc2Delay) modSrc.lfo2 = 0.0f;
    
    // Fade In? (Optional improvement for later)
    
    /* Original loop approach (too slow for control rate mod if we don't need FM)
    // Advance LFO phase for the rest of samples
    for(int k=1; k<numSamples; ++k) {
        lfoOsc1.processSample(0.0f);
        lfoOsc2.processSample(0.0f);
    } 
    */
    
    // Envelope Mod (Sample one point)
    // 1. Pre-calculate VCA Envelope (Audio Rate) to buffer
    juce::AudioBuffer<float> vcaEnvBuf(1, numSamples);
    auto* vcaWrite = vcaEnvBuf.getWritePointer(0);
    
    for(int k=0; k<numSamples; ++k)
    {
        float val = envVca.getNextSample();
        vcaWrite[k] = applyCurve(val, vcaCurve);
    }
    
    // 2. Sample VCF/Mod Envelopes (Control Rate - Start of Block)
    float rawMod = envMod.getNextSample(); // Sample 0
    float rawVcf = envVcf.getNextSample(); // Sample 0
    
    // Advance rest of block to keep state sync
    for(int k=1; k<numSamples; ++k) {
        envMod.getNextSample();
        envVcf.getNextSample();
    }
    
    modSrc.envMod = applyCurve(rawMod, modCurve);
    modSrc.envVcf = applyCurve(rawVcf, vcfCurve);
    modSrc.envVca = vcaWrite[0]; // Use first sample for Mod Matrix
    
    // Control Sequencer
    float seqVal = ctrlSeq.getNextSample();
    // Advance internal state for accurate timing (assuming control rate update)
    for(int k=1; k<numSamples; ++k) ctrlSeq.getNextSample();
    modSrc.ctrlSeq = seqVal;
    
    modSrc.velocity = currentVelocity; 
    modSrc.modWheel = currentModWheel; 
    
    DeepMindDSP::ModDestinations modDst;
    modMatrix.process(modSrc, modDst);
    
    // 2. Apply Modulations
    // Pitch (Osc1 & Osc2)
    // We apply pitch mod to ALL unison voices equally (preserving relative detune)
    // To do this efficiently, we just multiply the current base freq.
    // However, startNote set specific freqs. 
    // We should re-calc freq per voice if we want full mod accuracy.
    // Simplified: modulation affects the ratio.
    
    float pitchRatio1 = 1.0f;
    if (modDst.osc1Pitch != 0.0f) pitchRatio1 = std::pow(2.0f, (modDst.osc1Pitch * 12.0f) / 12.0f); 

    float pitchRatio2 = 1.0f;
    if (modDst.osc2Pitch != 0.0f) pitchRatio2 = std::pow(2.0f, (modDst.osc2Pitch * 12.0f) / 12.0f);

    // Apply pitch updates + Unison Spread (Recalculate or store offsets? Simpler to store offsets)
    // Quick Fix: Re-run spread logic here or assume 'osc[i].setFrequency' overwrote it?
    // 'setFrequency' overwrites. So we MUST include spread here or store base per voice.
    // Optimization: Just apply mod to the *current* freq? No, osc state doesn't expose getFreq easily.
    // Let's re-calc spread ratios here. It's cheap.
    
    float maxDetuneSemitones = currentUnisonDetune * 0.5f; 

    for (int i=0; i < unisonMode; ++i)
    {
         float spread = 0.0f;
         if (unisonMode > 1)
         {
             float norm = (float)i / (float)(unisonMode - 1); 
             float pan = norm * 2.0f - 1.0f;
             spread = pan * maxDetuneSemitones;
         }
         
         float spreadRatio = std::pow(2.0f, spread / 12.0f);
         
         osc1[i].setFrequency(static_cast<float>(currentBaseFrequency) * pitchRatio1 * spreadRatio);
         osc2[i].setFrequency(static_cast<float>(currentBaseFrequency) * pitchRatio2 * spreadRatio);
    }
    
    // Apply Global Drift (Slop)
    // Drift affects Pitch (+/- 20 cents max) and slightly Filter (-5% max)
    float driftVal = driftGen.getNextSample() * driftAmount; 
    float driftPitchRatio = std::pow(2.0f, (driftVal * 0.2f) / 12.0f); // +/- 20 cents
    
    // Apply to all unison voices (could be per-voice if we had array of drifts, but 12 drifts is CPU heavy?)
    // Actually SynthVoice IS one voice (with unison stack). So one drift per KEY is correct.
    // Ideally each unison stack voice drifts differently, but 1 drift per Key is a good start.
    
    for(auto& o : osc1) o.setFrequency(o.getFrequency() * driftPitchRatio);
    for(auto& o : osc2) o.setFrequency(o.getFrequency() * driftPitchRatio);
    
    // Cutoff Drift
    float modulatedCutoff = filter.getCutoff() + (modDst.vcfCutoff * 5000.0f); 
    modulatedCutoff *= (1.0f + (driftVal * 0.05f)); // +/- 5% freq variation 
    
    // Apply Key Tracking
    if (vcfKybdAmount != 0.0f)
    {
        float keyTrackRatio = std::pow(2.0f, (float)(currentNoteNumber - 60) / 12.0f * vcfKybdAmount);
        modulatedCutoff *= keyTrackRatio;
    }
    
    if (modulatedCutoff < 20.0f) modulatedCutoff = 20.0f;
    if (modulatedCutoff > 20000.0f) modulatedCutoff = 20000.0f;
    filter.setCutoff(modulatedCutoff);
    
    // 3. Process Audio (Block)
    // Summing Buffer
    juce::AudioBuffer<float> sumBuffer(subBlock.getNumChannels(), subBlock.getNumSamples());
    sumBuffer.clear();
    juce::dsp::AudioBlock<float> sumBlock(sumBuffer);

    // Scaling Factor to prevent clipping with unison
    // Soft scaling: 1 osc = 1.0, 2 osc = 0.7, 4 osc = 0.5
    float gain = 1.0f / std::sqrt((float)unisonMode);

    for (int i=0; i < unisonMode; ++i)
    {
        // Temp buffer for this voice
        juce::AudioBuffer<float> voiceBuf(subBlock.getNumChannels(), subBlock.getNumSamples());
        voiceBuf.clear();
        juce::dsp::AudioBlock<float> voiceBlock(voiceBuf);
        
        osc1[i].processBlock(voiceBlock);
        osc2[i].processBlock(voiceBlock); // Note: Osc2 adds to Osc1 in buffer? Needs check.
        
        // DeepMindOsc::processBlock REPLACES or ADDS?
        // Checking DeepMindOsc.h/cpp... usually replaces.
        // Wait, 'osc1.processBlock' and 'osc2.processBlock' were called sequentially on 'subBlock'.
        // That implies they were overwriting each other or mixing?
        // Standard juce::dsp::Oscillator replaces.
        // If DeepMindOsc uses juce::dsp::Oscillator::process, it replaces.
        // !!! BUG IN PREVIOUS CODE !!!
        // Line 136/137: osc1.process(blk); osc2.process(blk); -> Osc2 overwrites Osc1.
        // I need to mix them.
        
        // Fix: Mix Osc1 and Osc2 into voiceBuf
        osc1[i].processBlock(voiceBlock); // Writes Osc1

        // Mix Osc2
        juce::AudioBuffer<float> osc2Buf(subBlock.getNumChannels(), subBlock.getNumSamples());
        osc2Buf.clear();
        juce::dsp::AudioBlock<float> osc2Block(osc2Buf);
        osc2[i].processBlock(osc2Block);

        // Add Osc2 to VoiceBuf
        for(int ch=0; ch<voiceBuf.getNumChannels(); ++ch)
            voiceBuf.addFrom(ch, 0, osc2Buf, ch, 0, osc2Buf.getNumSamples());

        // Add VoiceBuf to SumBuf
        for(int ch=0; ch<sumBuffer.getNumChannels(); ++ch)
            sumBuffer.addFrom(ch, 0, voiceBuf, ch, 0, voiceBuf.getNumSamples(), gain);
    }
    
    // Copy sum back to subBlock
    subBlock.copyFrom(sumBlock);
    
    // 4. Filter
    filter.process(subBlock);
    
    // 5. VCA
    // Multiply output by pre-calculated VCA buffer
    // envVca.applyEnvelopeToBuffer(outputBuffer, startSample, numSamples); // OLD
    
    auto* vcaRead = vcaEnvBuf.getReadPointer(0);
    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
    {
        juce::FloatVectorOperations::multiply(outputBuffer.getWritePointer(ch, startSample), 
                                              vcaRead, numSamples);
    }

    

    
    // Check if note finished
    if (!envVca.isActive())
        clearCurrentNote();
}

void SynthVoice::updateParameters(juce::AudioProcessorValueTreeState* apvts)
{
    if (apvts == nullptr) return;

    // --- Oscillators ---
    auto* dco1Pwm = apvts->getRawParameterValue("dco1_pwm");
    // Handled in Unison block below
    // if (dco1Pwm) osc1.setShape(*dco1Pwm);
    
    // --- Filters ---
    auto* cutoff = apvts->getRawParameterValue("vcf_freq");
    if (cutoff) filter.setCutoff(*cutoff);
    
    auto* res = apvts->getRawParameterValue("vcf_res");
    if (res) filter.setResonance(*res);
    
    auto* kybd = apvts->getRawParameterValue("vcf_kybd");
    if (kybd) vcfKybdAmount = *kybd;
    
    auto* type = apvts->getRawParameterValue("vcf_type");
    if (type) filter.setType(static_cast<DeepMindDSP::FilterType>((int)*type));

    // --- Envelopes (Using setParameters for ADSR) ---
    juce::ADSR::Parameters vcaParams;
    vcaParams.attack = *apvts->getRawParameterValue("vca_attack");
    vcaParams.decay = *apvts->getRawParameterValue("vca_decay");
    vcaParams.sustain = *apvts->getRawParameterValue("vca_sustain");
    vcaParams.release = *apvts->getRawParameterValue("vca_release");
    envVca.setParameters(vcaParams);
    
    auto* vcaC = apvts->getRawParameterValue("vca_curve");
    if (vcaC) vcaCurve = *vcaC;
    
    // VCF
    juce::ADSR::Parameters vcfParams;
    vcfParams.attack = *apvts->getRawParameterValue("vcf_attack");
    vcfParams.decay = *apvts->getRawParameterValue("vcf_decay");
    vcfParams.sustain = *apvts->getRawParameterValue("vcf_sustain");
    vcfParams.release = *apvts->getRawParameterValue("vcf_release");
    envVcf.setParameters(vcfParams);

    auto* vcfC = apvts->getRawParameterValue("vcf_curve");
    if (vcfC) vcfCurve = *vcfC;

    // MOD
    juce::ADSR::Parameters modParams;
    modParams.attack = *apvts->getRawParameterValue("mod_attack");
    modParams.decay = *apvts->getRawParameterValue("mod_decay");
    modParams.sustain = *apvts->getRawParameterValue("mod_sustain");
    modParams.release = *apvts->getRawParameterValue("mod_release");
    envMod.setParameters(modParams);

    auto* modC = apvts->getRawParameterValue("mod_curve");
    if (modC) modCurve = *modC;
    
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
    auto* l1s = apvts->getRawParameterValue("lfo1_shape");
    if (l1r) currentLfoOsc1Rate = *l1r;
    if (l1d) lfoOsc1Delay = *l1d;
    if (l1s) lfo1Shape = static_cast<LfoShape>((int)*l1s);
    
    auto* l2r = apvts->getRawParameterValue("lfo2_rate");
    auto* l2d = apvts->getRawParameterValue("lfo2_delay");
    auto* l2s = apvts->getRawParameterValue("lfo2_shape");
    if (l2r) currentLfoOsc2Rate = *l2r;
    if (l2d) lfoOsc2Delay = *l2d;
    if (l2s) lfo2Shape = static_cast<LfoShape>((int)*l2s);

    // --- Unison ---
    // --- Unison / Polyphony ---
    auto* pMode = apvts->getRawParameterValue("polyphony_mode");
    auto* uDet = apvts->getRawParameterValue("unison_detune");
    
    if (pMode) 
    {
        int idx = (int)*pMode;
        // Map Selection to Voice Count
        // 0:Poly, 1:U2, 2:U3, 3:U4, 4:U6, 5:U12, 6:Mono, 7:M2, 8:M3, 9:M4, 10:M6, 11:P6, 12:P8
        static const int voiceMap[] = { 1, 2, 3, 4, 6, 12, 1, 2, 3, 4, 6, 1, 1 };
        
        if (idx >= 0 && idx < 13)
            unisonMode = voiceMap[idx];
        else
            unisonMode = 1;
    }
    if (uDet) currentUnisonDetune = *uDet;
    
    // Drift
    auto* drift = apvts->getRawParameterValue("drift");
    if (drift) driftAmount = *drift;

    // Propagate shapes to all unison voices
    if (dco1Pwm) {
        for(auto& o : osc1) o.setShape(*dco1Pwm);
    }
    
    // Control Sequencer Params
    auto* seqRate = apvts->getRawParameterValue("seq_rate");
    auto* seqSlew = apvts->getRawParameterValue("seq_slew");
    auto* seqSteps = apvts->getRawParameterValue("seq_steps");
    auto* seqSwing = apvts->getRawParameterValue("seq_swing");
    
    if (seqRate) ctrlSeq.setRate(*seqRate);
    if (seqSlew) ctrlSeq.setSlew(*seqSlew);
    if (seqSteps) ctrlSeq.setLength((int)*seqSteps);
    if (seqSwing) ctrlSeq.setSwing(*seqSwing);
    
    // Cache pointers if empty
    if (seqStepParams.empty()) {
        for(int i=1; i<=32; ++i) {
            seqStepParams.push_back(apvts->getRawParameterValue("seq_step_" + juce::String(i)));
        }
    }
    
    // Update steps
    for(int i=0; i<32; ++i) {
        if (i < seqStepParams.size() && seqStepParams[i])
            ctrlSeq.setStepValue(i, *(seqStepParams[i]));
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
