#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Data/SysexTranslator.h" 
#include "Data/MidiManager.h" // Explicit include to fix incomplete type
#include "Data/DeepMindParameters.h"

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
    
    // Defer slighty to ensure devices are ready? No, constructor is fine usually.
    // We need to access the StandalonePluginHolder.
    
    // Note: This only compiles if we are building the Standalone target source specifically,
    // or if JUCE_STANDALONE_APPLICATION is defined (which it is for standalone builds).
    
    /* 
       We cannot easily access StandalonePluginHolder instance from here without linking headers 
       or using magic singletons that might not be init yet.
       However, we can just use MidiInput::getAvailableDevices() and set it on the *DeviceManager*?
       Wait, the DeviceManager belongs to the StandaloneFilterWindow... 
       Actually, standard JUCE AudioProcessor doesn't own the DeviceManager.
       The Standalone wrapper does.
       
       Hack: The user can select it in Options.
       But for a "It sounded before" fix, maybe we should just ensure logic is correct.
       The user said "Test works". That suggests Audio driver is fine.
       If Screen Keyboard doesn't work, MIDI INPUT device is NOT the issue for *Screen Keyboard*.
       
       So the issue is likely internal (Arp or processing).
    */
#endif

    // Hardcoded SysEx load removed in favor of manual import via GUI.
    // Voices and Sound setup below


    // Add voices to synthesiser
    // Add voices to synthesiser (Default to 12)
    for (int i = 0; i < 12; ++i)
        synthesiser.addVoice(new voice::SynthVoice());
        
    synthesiser.addSound(new voice::SynthSound());
    
    synthesiser.addSound(new voice::SynthSound());
    
    
    midiManager = std::make_unique<data::MidiManager>(apvts);
    oscManager = std::make_unique<data::OscManager>(apvts);
    oscManager->connect(8000); // Port 8000
    
    apvts.addParameterListener("polyphony_mode", this);
    // Initial update
    // updatePolyphony(); // Calling virtual/complex methods in constructor is risky? 
    // Just ensure default 12 voices (set in loop above to 8. Update to 12).
}

DeepMindSynthAudioProcessor::~DeepMindSynthAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout DeepMindSynthAudioProcessor::createParameterLayout()
{
    return DeepMindParams::createParameterLayout();
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
    arpeggiator.prepare(spec);
    // arpeggiator.setBypass(false); // DISABLED: Let parameters control bypass (default Off)
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
    
    // 1. Handle MIDI Input (Note On/Off handled by synth, CCs by Manager)
    // Debug: Track Last Note
    for (const auto metadata : midiMessages)
    {
        if (metadata.getMessage().isNoteOn())
            lastNoteTriggered = metadata.getMessage().getNoteNumber();
    }

    midiManager->processMidiBuffer(midiMessages);
    
    // 1.5 Chord Memory (Expand Notes)
    chordMemory.process(midiMessages);
    
    keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);

    // --- Audio Input Handling (Multi-FX Mode) ---
    auto* extGain = apvts.getRawParameterValue("ext_audio_gain");
    float inputGain = extGain ? extGain->load() : 0.0f;
    
    if (inputGain > 0.001f)
    {
        // Apply Gain to Input
        buffer.applyGain(inputGain);
    }
    else
    {
        // Standard Synth Mode: Clear garbage/input
        buffer.clear();
    }

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
    
    auto* arpPat = apvts.getRawParameterValue("arp_pattern");
    if (arpPat) arpeggiator.setPattern((int)*arpPat);

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
    
    // Ensure we don't silence the synth if input gain is 0 (which is handled above).
    // Synth renders ADDITIVELY to buffer.
    
    juce::dsp::AudioBlock<float> block(buffer);
    fxChain.process(block);
    
    // Send Outgoing MIDI (CC/NRPN from UI)
    midiManager->processOutgoingMidi(midiMessages);
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

// --- Listener ---
void DeepMindSynthAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "polyphony_mode")
    {
        juce::MessageManager::callAsync([this]() { updatePolyphony(); });
    }
}

void DeepMindSynthAudioProcessor::updatePolyphony()
{
    int idx = (int)*apvts.getRawParameterValue("polyphony_mode");
    int target = 12;
    
    switch(idx) {
        case 0: target=12; break; // Poly
        case 1: target=6; break; // U2
        case 2: target=4; break; // U3
        case 3: target=3; break; // U4
        case 4: target=2; break; // U6
        case 5: target=1; break; // U12
        case 6: target=1; break; // Mono
        case 7: target=1; break; // Mono-2
        case 8: target=1; break; // Mono-3
        case 9: target=1; break; // Mono-4
        case 10: target=1; break; // Mono-6
        case 11: target=6; break; // Poly-6
        case 12: target=8; break; // Poly-8
        default: target=12; break;
    }
    
    if (synthesiser.getNumVoices() == target) return;
    
    suspendProcessing(true);
    synthesiser.clearVoices();
    for(int i=0; i<target; ++i)
        synthesiser.addVoice(new voice::SynthVoice());
        
    if (getSampleRate() > 0)
        synthesiser.setCurrentPlaybackSampleRate(getSampleRate());
        
    suspendProcessing(false);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DeepMindSynthAudioProcessor();
}
