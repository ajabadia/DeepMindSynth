#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "GUI/DeepMindLookAndFeel.h"
#include "GUI/OverlayComponent.h"
#include "GUI/ArpEditor.h"
// #include "GUI/ModMatrixEditor.h"
#include "GUI/ControlSeqEditor.h"
// #include "GUI/FxEditor.h"
#include "Data/PresetManager.h"

class DeepMindSynthAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    DeepMindSynthAudioProcessorEditor (DeepMindSynthAudioProcessor&);
    ~DeepMindSynthAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    
    void timerCallback() override;

private:
    DeepMindSynthAudioProcessor& audioProcessor;
    gui::DeepMindLookAndFeel lnf;
    OverlayComponent overlay;

    // UI Controls
    juce::MidiKeyboardComponent midiKeyboard;
    
    // --- OSCILLATORS ---
    juce::Slider param_dco1_range, param_dco1_pwm;
    juce::Slider param_dco1_pulse_en, param_dco1_saw_en; // On/Off buttons or Sliders
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
    std::unique_ptr<Attachment> att_dco1_pulse_en, att_dco1_saw_en;
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
    
    // --- ENVELOPES (Switched) ---
    juce::TextButton btnEnvVca { "VCA" };
    juce::TextButton btnEnvVcf { "VCF" };
    juce::TextButton btnEnvMod { "MOD" };
    int currentEnvSelection = 0; // 0=VCA, 1=VCF, 2=MOD
    
    juce::Slider param_env_a, param_env_d, param_env_s, param_env_r, param_env_curve;
    
    std::unique_ptr<Attachment> att_env_a, att_env_d, att_env_s, att_env_r, att_env_curve;
    
    void updateEnvelopeAttachments();
    
    // Legacy VCA attachments removed
    // std::unique_ptr<Attachment> att_vca_a, att_vca_d, att_vca_s, att_vca_r;
    
    std::unique_ptr<Attachment> att_lfo1_rate, att_lfo1_delay;
    std::unique_ptr<Attachment> att_lfo2_rate, att_lfo2_delay;
    
    // NEW LFO Shapes
    juce::ComboBox cmb_lfo1_shape, cmb_lfo2_shape;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> att_lfo1_shape, att_lfo2_shape;
    
    // Overlay Buttons
    juce::TextButton btnArp { "ARP EDIT" };
    // juce::TextButton btnMod { "MOD MATRIX" };
    // juce::TextButton btnFx  { "FX EDIT" };
    juce::TextButton btnLoadSysex { "LOAD BANK" }; // Keep for legacy/manual load
    juce::Label lblCpu;
    
    // Presets
    juce::ComboBox cmbPresets;
    juce::TextButton btnSavePreset { "SAVE" };
    data::PresetManager presetManager { audioProcessor.apvts };
    
    std::unique_ptr<ControlSeqEditor> ctrlSeqEditor;
    
    // Unison / Poly
    juce::ComboBox cmb_poly_mode;
    juce::Slider param_unison_detune;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> att_poly_mode;
    std::unique_ptr<Attachment> att_unison_detune;
    
    // Drift
    juce::Slider param_drift;
    std::unique_ptr<Attachment> att_drift;
    
    // Chord Memory
    juce::TextButton btnChord { "CHORD" };
    juce::TextButton btnLearn { "LEARN" }; // Set to ClickTogglesState
    juce::TextButton btnSeqEdit { "SEQ" }; // Opens Control Sequencer

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeepMindSynthAudioProcessorEditor)
};
