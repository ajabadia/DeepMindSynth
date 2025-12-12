#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "GUI/DeepMindLookAndFeel.h"
#include "GUI/OverlayComponent.h"
#include "Data/PresetManager.h"

class DeepMindSynthAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    DeepMindSynthAudioProcessorEditor (DeepMindSynthAudioProcessor&);
    ~DeepMindSynthAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    DeepMindSynthAudioProcessor& audioProcessor;
    gui::DeepMindLookAndFeel lnf;
    OverlayComponent overlay;

    // UI Controls
    juce::MidiKeyboardComponent midiKeyboard;
    
    // --- OSCILLATORS ---
    juce::Slider param_dco1_range, param_dco1_pwm;
    juce::Slider param_dco2_pitch, param_dco2_tone, param_dco2_lvl;
    juce::Slider param_e_saw_lvl, param_e_pulse_lvl, param_e_noise_lvl; // 'e' for extra/mix
    
    // --- FILTERS ---
    juce::Slider param_vcf_freq, param_vcf_res, param_vcf_env, param_vcf_lfo, param_vcf_kybd;
    juce::Slider param_hpf_freq;
    
    // --- ENVELOPES (VCA only for main view) ---
    juce::Slider param_vca_a, param_vca_d, param_vca_s, param_vca_r;
    
    // --- LFOs ---
    juce::Slider param_lfo1_rate, param_lfo1_delay;
    juce::Slider param_lfo2_rate, param_lfo2_delay;

    // Attachments
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    
    std::unique_ptr<Attachment> att_dco1_range, att_dco1_pwm;
    std::unique_ptr<Attachment> att_dco2_pitch, att_dco2_tone, att_dco2_lvl;
    std::unique_ptr<Attachment> att_saw_lvl, att_pulse_lvl, att_noise_lvl;
    
    std::unique_ptr<Attachment> att_vcf_freq, att_vcf_res, att_vcf_env, att_vcf_lfo, att_vcf_kybd;
    std::unique_ptr<Attachment> att_hpf_freq;
    
    // VCF Type
    juce::ComboBox cmb_vcf_type;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> att_vcf_type;
    
    // Layout Helpers
    void drawSection(juce::Graphics& g, juce::Rectangle<int> bounds, juce::String title, juce::Colour headerColor);
    void drawControlLabel(juce::Graphics& g, juce::Rectangle<int> bounds, juce::String name);
    
    std::unique_ptr<Attachment> att_vca_a, att_vca_d, att_vca_s, att_vca_r;
    
    std::unique_ptr<Attachment> att_lfo1_rate, att_lfo1_delay;
    std::unique_ptr<Attachment> att_lfo2_rate, att_lfo2_delay;
    
    // Overlay Buttons
    juce::TextButton btnArp { "ARP EDIT" };
    juce::TextButton btnMod { "MOD MATRIX" };
    juce::TextButton btnFx  { "FX EDIT" };
    juce::TextButton btnLoadSysex { "LOAD BANK" }; // Keep for legacy/manual load
    
    // Presets
    juce::ComboBox cmbPresets;
    juce::TextButton btnSavePreset { "SAVE" };
    data::PresetManager presetManager { audioProcessor.apvts };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeepMindSynthAudioProcessorEditor)
};
