#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Data/SysexTranslator.h" // Ensure this is included

DeepMindSynthAudioProcessor::DeepMindSynthAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                       ),
       apvts(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
#if JUCE_STANDALONE_APPLICATION
    // RPi Headless / Kiosk Mode Auto-Connect
    // Auto-select first available MIDI and Audio devices on startup
    /*
    auto& deviceManager = juce::StandalonePluginHolder::getInstance()->deviceManager;
    
    // 1. Audio
    // JUCE often auto-selects default audio, but we can force it if needed.
    // For now, assume default is improved by recent JUCE versions.
    
    // 2. MIDI Input (Enable first if none enabled)
    auto midiInputs = juce::MidiInput::getAvailableDevices();
    if (midiInputs.size() > 0)
    {
        if (!deviceManager.isMidiInputDeviceEnabled(midiInputs[0].identifier))
            deviceManager.setMidiInputDeviceEnabled(midiInputs[0].identifier, true);
    }
    */
#endif

    // Hardcoded SysEx load removed in favor of manual import via GUI.
    // Voices and Sound setup below


    // Add voices to synthesiser
    for (int i = 0; i < 8; ++i)
        synthesiser.addVoice(new voice::SynthVoice());
        
    synthesiser.addSound(new voice::SynthSound());
}

DeepMindSynthAudioProcessor::~DeepMindSynthAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout DeepMindSynthAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    auto addFloat = [&](juce::String id, juce::String name, float min, float max, float def) {
        layout.add(std::make_unique<juce::AudioParameterFloat>(id, name, min, max, def));
    };

    auto addChoice = [&](juce::String id, juce::String name, juce::StringArray choices, int defIndex) {
        layout.add(std::make_unique<juce::AudioParameterChoice>(id, name, choices, defIndex));
    };

    // --- OSCILLATORS ---
    addFloat("dco1_range", "DCO1 Range", 0.0f, 4.0f, 2.0f); // 16', 8', 4' etc (Discrete steps effectively)
    addFloat("dco1_pwm", "DCO1 PWM", 0.0f, 1.0f, 0.5f);
    addFloat("dco2_pitch", "DCO2 Pitch", -12.0f, 12.0f, 0.0f);
    addFloat("dco2_tone", "DCO2 Tone Mod", 0.0f, 1.0f, 0.0f);
    addFloat("dco2_lvl", "DCO2 Level", 0.0f, 1.0f, 1.0f); // Mix
    addFloat("saw_lvl", "Saw Level", 0.0f, 1.0f, 1.0f);   // DCO1 Mix
    addFloat("pulse_lvl", "Pulse Level", 0.0f, 1.0f, 0.0f); // DCO1 Mix
    addFloat("noise_lvl", "Noise Level", 0.0f, 1.0f, 0.0f);

    // --- FILTER ---
    addFloat("vcf_freq", "VCF Freq", 20.0f, 20000.0f, 20000.0f); // CC 29
    addFloat("vcf_res", "VCF Res", 0.0f, 1.0f, 0.0f);            // CC 30
    addFloat("vcf_env", "VCF Env Depth", -1.0f, 1.0f, 0.0f);
    addFloat("vcf_lfo", "VCF LFO Depth", 0.0f, 1.0f, 0.0f);
    addFloat("vcf_kybd", "VCF Kybd Track", 0.0f, 1.0f, 0.0f);
    addChoice("vcf_type", "Filter Type", {"Jupiter 24dB", "MS-20 LP", "Acid 303"}, 0);
    addFloat("hpf_freq", "HPF Freq", 20.0f, 2000.0f, 20.0f); // Bass cut

    // --- ENVELOPES (ADSR) ---
    // VCA
    addFloat("vca_attack", "VCA Attack", 0.0f, 10.0f, 0.01f);
    addFloat("vca_decay", "VCA Decay", 0.0f, 10.0f, 0.5f);
    addFloat("vca_sustain", "VCA Sustain", 0.0f, 1.0f, 1.0f);
    addFloat("vca_release", "VCA Release", 0.0f, 10.0f, 0.1f);
    
    // VCF
    addFloat("vcf_attack", "VCF Attack", 0.0f, 10.0f, 0.01f);
    addFloat("vcf_decay", "VCF Decay", 0.0f, 10.0f, 0.5f);
    addFloat("vcf_sustain", "VCF Sustain", 0.0f, 1.0f, 1.0f);
    addFloat("vcf_release", "VCF Release", 0.0f, 10.0f, 0.1f);
    
    // MOD
    addFloat("mod_attack", "Mod Attack", 0.0f, 10.0f, 0.01f);
    addFloat("mod_decay",  "Mod Decay", 0.0f, 10.0f, 0.5f);
    addFloat("mod_sustain","Mod Sustain", 0.0f, 1.0f, 1.0f);
    addFloat("mod_release","Mod Release", 0.0f, 10.0f, 0.1f);

    // --- LFOs ---
    addFloat("lfo1_rate", "LFO1 Rate", 0.01f, 50.0f, 1.0f); // CC 16
    addFloat("lfo1_delay", "LFO1 Delay", 0.0f, 5.0f, 0.0f);
    addFloat("lfo2_rate", "LFO2 Rate", 0.01f, 50.0f, 1.0f);
    addFloat("lfo2_delay", "LFO2 Delay", 0.0f, 5.0f, 0.0f);

    // --- EFFECTS ---
    // Chorus
    addFloat("fx_chorus_mix", "Chorus Mix", 0.0f, 1.0f, 0.0f);
    addFloat("fx_chorus_rate", "Chorus Rate", 0.1f, 10.0f, 1.0f);
    addFloat("fx_chorus_depth", "Chorus Depth", 0.0f, 1.0f, 0.5f);
    
    // Delay
    addFloat("fx_delay_mix", "Delay Mix", 0.0f, 1.0f, 0.0f);
    addFloat("fx_delay_time", "Delay Time", 0.0f, 1.0f, 0.5f); // 0-1 sec
    addFloat("fx_delay_feedback", "Delay Feedback", 0.0f, 0.95f, 0.0f);
    
    // Reverb
    addFloat("fx_reverb_mix", "Reverb Mix", 0.0f, 1.0f, 0.0f);
    addFloat("fx_reverb_size", "Reverb Size", 0.0f, 1.0f, 0.5f);
    addFloat("fx_reverb_damp", "Reverb Damp", 0.0f, 1.0f, 0.5f);

    // --- MOD MATRIX (8 Slots) ---
    juce::StringArray modSources = { "None", "LFO1", "LFO2", "EnvMod", "Velocity", "ModWheel", "KeyTrack" };
    juce::StringArray modDests = { "None", "Osc1 Pitch", "Osc1 PWM", "Osc2 Pitch", "VCF Freq", "VCF Res" };

    for (int i = 1; i <= 8; ++i)
    {
        juce::String prefix = "mod_slot_" + juce::String(i);
        addChoice(prefix + "_src", "Mod Slot " + juce::String(i) + " Src", modSources, 0); // Default None
        addChoice(prefix + "_dst", "Mod Slot " + juce::String(i) + " Dst", modDests, 0);   // Default None
        addFloat(prefix + "_amt",  "Mod Slot " + juce::String(i) + " Amt", -1.0f, 1.0f, 0.0f);
    }

    // --- ARPEGGIATOR ---
    addChoice("arp_mode", "Arp Mode", { "Up", "Down", "UpDown", "Random", "Chord", "Pattern" }, 0);
    addFloat("arp_rate", "Arp Rate", 0.0f, 1.0f, 0.5f); // Clock Divider or Hz
    addFloat("arp_gate", "Arp Gate", 0.0f, 1.0f, 0.5f);
    addFloat("arp_oct",  "Arp Octaves", 1.0f, 4.0f, 1.0f);
    addFloat("arp_on",   "Arp On", 0.0f, 1.0f, 0.0f);

    // --- GLOBAL ---
    addFloat("master_vol", "Master Volume", 0.0f, 1.0f, 0.8f);
    addFloat("portamento", "Portamento", 0.0f, 1.0f, 0.0f);
    addFloat("unison_detune", "Unison Detune", 0.0f, 1.0f, 0.0f); // CC 28
    addFloat("pan", "Pan", -1.0f, 1.0f, 0.0f); // CC 10

    return layout;
}

void DeepMindSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synthesiser.setCurrentPlaybackSampleRate(sampleRate);
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    
    fxChain.prepare(spec);
    arpeggiator.prepare(spec);
    arpeggiator.setBypass(false); // Enable for testing Phase 3.5
}

void DeepMindSynthAudioProcessor::releaseResources()
{
    arpeggiator.reset();
}

bool DeepMindSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return true;
}

void DeepMindSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    // Process On-Screen Keyboard
    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

    // Handle MIDI CCs matching DeepMind Spec
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        if (message.isController())
        {
            int ccNumber = message.getControllerNumber();
            float value = message.getControllerValue() / 127.0f; // Normalize 0-1
            
            // Map CC to Parameter ID (Hardcoded mapping from CSV)
            juce::String paramId = "";
            
            switch (ccNumber)
            {
                case 29: paramId = "vcf_freq"; break; // VCF Freq
                case 30: paramId = "vcf_res"; break;  // VCF Reso
                case 16: paramId = "lfo1_rate"; break; // LFO1 Rate
                case 21: paramId = "dco1_pwm"; break;  // OSC1 PWM
                case 28: paramId = "unison_detune"; break; // Unison
                case 10: paramId = "pan"; break; // Pan
                case 37: paramId = "arp_rate"; break; // Arp Rate
                // ... Add more mappings as needed
            }
            
            if (paramId.isNotEmpty())
            {
                auto* param = apvts.getParameter(paramId);
                if (param) param->setValueNotifyingHost(value); 
            }
        }
    }
    
    // Update Arpeggiator Parameters
    auto* arpOn = apvts.getRawParameterValue("arp_on");
    auto* arpMode = apvts.getRawParameterValue("arp_mode");
    auto* arpRate = apvts.getRawParameterValue("arp_rate");
    auto* arpOct = apvts.getRawParameterValue("arp_oct");
    
    if (arpOn) arpeggiator.setBypass(*arpOn < 0.5f);
    if (arpMode) arpeggiator.setMode(static_cast<DeepMindDSP::ArpMode>((int)*arpMode));
    if (arpRate) arpeggiator.setRate(4.0f + (*arpRate * 20.0f)); // Simple mapping 4Hz to 24Hz for verification
    if (arpOct) arpeggiator.setOctaveRange((int)*arpOct);

    // Process Arpeggiator (Generates new MIDI notes based on held chords)
    // It modifies 'midiMessages' in place (clears input, adds arp notes)
    // It modifies 'midiMessages' in place (clears input, adds arp notes)
    arpeggiator.processBlock(midiMessages, buffer.getNumSamples());

    // Update FX
    auto* chorusMix = apvts.getRawParameterValue("fx_chorus_mix");
    auto* chorusRate = apvts.getRawParameterValue("fx_chorus_rate");
    auto* chorusDepth = apvts.getRawParameterValue("fx_chorus_depth");
    
    auto* delayMix = apvts.getRawParameterValue("fx_delay_mix");
    auto* delayTime = apvts.getRawParameterValue("fx_delay_time");
    auto* delayFb = apvts.getRawParameterValue("fx_delay_feedback");

    auto* reverbMix = apvts.getRawParameterValue("fx_reverb_mix");
    auto* reverbSize = apvts.getRawParameterValue("fx_reverb_size");
    auto* reverbDamp = apvts.getRawParameterValue("fx_reverb_damp");
    
    if (chorusMix) fxChain.setChorusParams(
        chorusRate ? chorusRate->load() : 1.0f,
        chorusDepth ? chorusDepth->load() : 0.5f,
        chorusMix->load()
    );

    if (delayMix) fxChain.setDelayParams(
        delayTime ? delayTime->load() : 0.5f,
        delayFb ? delayFb->load() : 0.0f,
        delayMix->load()
    );
     
    if (reverbMix) fxChain.setReverbParams(
        reverbSize ? reverbSize->load() : 0.5f,
        reverbDamp ? reverbDamp->load() : 0.5f,
        reverbMix->load()
    );

    // Update Voice Parameters
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<voice::SynthVoice*>(synthesiser.getVoice(i)))
        {
            voice->updateParameters(&apvts);
        }
    }

    synthesiser.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    
    juce::dsp::AudioBlock<float> block(buffer);
    fxChain.process(block);
}

bool DeepMindSynthAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* DeepMindSynthAudioProcessor::createEditor()
{
    return new DeepMindSynthAudioProcessorEditor (*this);
}

void DeepMindSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Store parameters as XML
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void DeepMindSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Restore parameters from XML
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

// Standard boilerplate
const juce::String DeepMindSynthAudioProcessor::getName() const { return JucePlugin_Name; }
bool DeepMindSynthAudioProcessor::acceptsMidi() const { return true; }
bool DeepMindSynthAudioProcessor::producesMidi() const { return true; }
bool DeepMindSynthAudioProcessor::isMidiEffect() const { return false; }
double DeepMindSynthAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int DeepMindSynthAudioProcessor::getNumPrograms() { return 1; }
int DeepMindSynthAudioProcessor::getCurrentProgram() { return 0; }
void DeepMindSynthAudioProcessor::setCurrentProgram (int index) {}
const juce::String DeepMindSynthAudioProcessor::getProgramName (int index) { return {}; }
void DeepMindSynthAudioProcessor::changeProgramName (int index, const juce::String& newName) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DeepMindSynthAudioProcessor();
}
