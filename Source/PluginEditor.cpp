#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "GUI/ArpEditor.h"
#include "GUI/ModMatrixEditor.h"
#include "GUI/ArpEditor.h"
#include "GUI/ModMatrixEditor.h"
#include "GUI/FxEditor.h"
#include "Data/SysexTranslator.h"

DeepMindSynthAudioProcessorEditor::DeepMindSynthAudioProcessorEditor (DeepMindSynthAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      midiKeyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    juce::LookAndFeel::setDefaultLookAndFeel(&lnf);
    setSize (1024, 768);
    
    // --- Helper to setup sliders ---
    auto setupSlider = [this](juce::Slider& slider, juce::String paramId, std::unique_ptr<Attachment>& attachment)
    {
        addAndMakeVisible(slider);
        slider.setSliderStyle(juce::Slider::LinearVertical);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0,0);
        attachment = std::make_unique<Attachment>(audioProcessor.apvts, paramId, slider);
    };

    // --- VCF Type Selector ---
    addAndMakeVisible(cmb_vcf_type);
    cmb_vcf_type.addItem("Jupiter 8 (Ladder 24dB)", 1);
    cmb_vcf_type.addItem("MS-20 (SVF + Dist)", 2);
    cmb_vcf_type.addItem("TB-303 (Acid)", 3);
    att_vcf_type = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "vcf_type", cmb_vcf_type);

    // --- OSCILLATORS ---
    setupSlider(param_dco1_range, "dco1_range", att_dco1_range);
    setupSlider(param_dco1_pwm,   "dco1_pwm",   att_dco1_pwm);
    
    setupSlider(param_dco2_pitch, "dco2_pitch", att_dco2_pitch);
    setupSlider(param_dco2_tone,  "dco2_tone",  att_dco2_tone);
    setupSlider(param_dco2_lvl,   "dco2_lvl",   att_dco2_lvl);
    
    setupSlider(param_e_saw_lvl,   "saw_lvl",   att_saw_lvl);
    setupSlider(param_e_pulse_lvl, "pulse_lvl", att_pulse_lvl);
    setupSlider(param_e_noise_lvl, "noise_lvl", att_noise_lvl);

    // --- FILTERS ---
    setupSlider(param_vcf_freq, "vcf_freq", att_vcf_freq);
    setupSlider(param_vcf_res,  "vcf_res",  att_vcf_res);
    setupSlider(param_vcf_env,  "vcf_env",  att_vcf_env);
    setupSlider(param_vcf_lfo,  "vcf_lfo",  att_vcf_lfo);
    setupSlider(param_vcf_kybd, "vcf_kybd", att_vcf_kybd);
    setupSlider(param_hpf_freq, "hpf_freq", att_hpf_freq);

    // --- ENVELOPES ---
    setupSlider(param_vca_a, "vca_attack",  att_vca_a);
    setupSlider(param_vca_d, "vca_decay",   att_vca_d);
    setupSlider(param_vca_s, "vca_sustain", att_vca_s);
    setupSlider(param_vca_r, "vca_release", att_vca_r);

    // --- LFOs ---
    setupSlider(param_lfo1_rate,  "lfo1_rate",  att_lfo1_rate);
    setupSlider(param_lfo1_delay, "lfo1_delay", att_lfo1_delay);
    setupSlider(param_lfo2_rate,  "lfo2_rate",  att_lfo2_rate);
    setupSlider(param_lfo2_delay, "lfo2_delay", att_lfo2_delay);

    // --- KEYBOARD ---
    addAndMakeVisible(midiKeyboard);
    midiKeyboard.setAvailableRange(24, 96);

    // --- BUTTONS ---
    addAndMakeVisible(btnArp);
    addAndMakeVisible(btnMod);
    addAndMakeVisible(btnFx);
    addAndMakeVisible(btnFx);
    addAndMakeVisible(btnLoadSysex); // Consider hiding or keeping as auxiliary
    
    addAndMakeVisible(cmbPresets);
    addAndMakeVisible(btnSavePreset);
    
    // Init Preset List
    cmbPresets.setTextWhenNothingSelected("Select Preset...");
    auto updatePresetList = [this]() {
        cmbPresets.clear();
        auto files = presetManager.findPresets();
        for (int i=0; i<files.size(); ++i)
            cmbPresets.addItem(files[i], i+1);
    };
    updatePresetList();
    
    cmbPresets.onChange = [this](){
        if (cmbPresets.getSelectedId() > 0)
        {
             juce::String name = cmbPresets.getText();
             auto file = presetManager.getPresetsDirectory().getChildFile(name).withFileExtension(".xml");
             presetManager.loadPreset(file);
        }
    };
    
    btnSavePreset.onClick = [this, updatePresetList](){
        // Use dedicated helper for custom text input
        auto* w = new juce::AlertWindow("Save Preset", "Enter name:", juce::AlertWindow::NoIcon);
        w->addTextEditor("name", "Init Patch", "Preset Name:");
        w->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
        w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        
        w->enterModalState(true, juce::ModalCallbackFunction::create([this, w, updatePresetList](int res){
            if (res == 1)
            {
                juce::String name = w->getTextEditorContents("name");
                presetManager.savePreset(name);
                updatePresetList();
                
                // Select it
                for(int i=0; i<cmbPresets.getNumItems(); ++i)
                    if (cmbPresets.getItemText(i) == name)
                        cmbPresets.setSelectedId(i+1, juce::dontSendNotification);
            }
            delete w; // CLOSE THE WINDOW
        }));
    };
    
    btnArp.onClick = [this](){ 
        if (overlay.isVisible()) {
            overlay.setVisible(false);
            overlay.clearContent();
        } else {
            overlay.setContent(std::make_unique<ArpEditor>(audioProcessor.apvts));
            overlay.setVisible(true);
        }
    };
    
    btnMod.onClick = [this](){
        if (overlay.isVisible()) {
            overlay.setVisible(false);
            overlay.clearContent();
        } else {
            overlay.setContent(std::make_unique<ModMatrixEditor>(audioProcessor.apvts));
            overlay.setVisible(true);
        }
    };

    btnFx.onClick = [this](){
        if (overlay.isVisible()) {
            overlay.setVisible(false);
            overlay.clearContent();
        } else {
            overlay.setContent(std::make_unique<FxEditor>(audioProcessor.apvts));
            overlay.setVisible(true);
        }
    };
    
    btnLoadSysex.onClick = [this](){
        
        auto fileChooser = std::make_shared<juce::FileChooser>("Select DeepMind SysEx File",
                                                              juce::File::getSpecialLocation(juce::File::userHomeDirectory),
                                                              "*.syx;*.sysex");
                                                              
        fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, fileChooser](const juce::FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file.existsAsFile())
                {
                    juce::MemoryBlock sysexData;
                    if (file.loadFileAsData(sysexData))
                    {
                        // Parse
                         juce::MidiMessage msg(sysexData.getData(), (int)sysexData.getSize());
                         bool success = data::SysexTranslator::parseSysex(msg, audioProcessor.apvts);
                         
                         // Feedback
                         if (success)
                         {
                             juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "SysEx Loaded", 
                                                                    "Success! Parsed DeepMind Bank.\n" + 
                                                                    juce::String(sysexData.getSize()) + " bytes.");
                         }
                         else
                         {
                             juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "SysEx Error", 
                                                                    "File loaded but Header ID (00 20 32) not found.\n"
                                                                    "Ensure this is a valid DeepMind 12 .syx file.");
                         }
                    }
                }
            });
    };
    
    // Overlay (Must be last to be on top)
    addAndMakeVisible(overlay);
    overlay.setVisible(false);
}

DeepMindSynthAudioProcessorEditor::~DeepMindSynthAudioProcessorEditor()
{
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void DeepMindSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xFF202225)); // DeepMind Dark Grey
    g.setColour (juce::Colours::white);
    g.setFont (20.0f);
    g.drawText ("DeepMind 12 Clone", 20, 10, 200, 30, juce::Justification::left);
    
    // Draw Section Labels
    g.setFont(14.0f);
    g.fillAll (juce::Colour(0xff252525)); // Dark Chassis

    // Define Section Colors
    juce::Colour colRed = juce::Colour(0xffd03030);
    juce::Colour colBlue = juce::Colour(0xff3090d0);
    juce::Colour colWhite = juce::Colour(0xffe0e0e0);
    
    auto area = getLocalBounds();
    auto keyboardArea = area.removeFromBottom(80); // Full Width Keyboard
    
    // Top Row: Arp, LFO, Screen/Global
    auto topRow = area.removeFromTop(area.getHeight() / 2);
    auto arpArea = topRow.removeFromLeft(200);
    auto lfoArea = topRow.removeFromLeft(300);
    auto screenArea = topRow; // Remainder (Mod/Fx/Presets)
    
    drawSection(g, arpArea.reduced(5), "ARP / SEQ", colBlue);
    drawSection(g, lfoArea.reduced(5), "LFO 1 & 2", colRed);
    drawSection(g, screenArea.reduced(5), "PROGRAMMER", colRed);
    
    // Bottom Row: Osc, VCF, VCA, HPF, Env
    auto botRow = area;
    auto oscArea = botRow.removeFromLeft(350);
    auto vcfArea = botRow.removeFromLeft(220);
    auto vcaArea = botRow.removeFromLeft(80);
    auto hpfArea = botRow.removeFromLeft(80);
    auto envArea = botRow; // Remainder
    
    drawSection(g, oscArea.reduced(5), "OSC 1 & 2", colRed);
    drawSection(g, vcfArea.reduced(5), "VCF", colRed);
    drawSection(g, vcaArea.reduced(5), "VCA", colRed);
    drawSection(g, hpfArea.reduced(5), "HPF", colBlue);
    drawSection(g, envArea.reduced(5), "ENVELOPES", colWhite);

    // Draw Labels for Sliders (Hardware style printed on chassis)
    g.setColour(juce::Colours::white);
    g.setFont(10.0f);
    
    // Helper to draw label above slider rect (approximated based on known layout)
    // In a real app, we'd store the rects. For now, we rely on the grid logic matching resized().
}

void DeepMindSynthAudioProcessorEditor::drawSection(juce::Graphics& g, juce::Rectangle<int> bounds, juce::String title, juce::Colour headerColor)
{
    // Background
    g.setColour(juce::Colour(0xff303030));
    g.fillRect(bounds);
    g.setColour(juce::Colours::black);
    g.drawRect(bounds, 1);
    
    // Header
    auto headerRect = bounds.removeFromTop(20);
    g.setColour(headerColor);
    g.fillRect(headerRect);
    
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawFittedText(title, headerRect, juce::Justification::centred, 1);
    
    // Red Strip at bottom (DeepMind style)
    auto strip = bounds.removeFromBottom(4);
    g.setColour(headerColor);
    g.fillRect(strip);
}


void DeepMindSynthAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    
    // 1. Keyboard (Full Width, Bottom)
    midiKeyboard.setBounds(area.removeFromBottom(80));
    
    // Layout Constants
    const int margin = 5;
    const int headerH = 25; // Space for the section title
    const int stripH = 5;
    
    // Helper to get usable section area
    auto getSectionBody = [&](juce::Rectangle<int> sectionRect) {
        return sectionRect.reduced(margin).withTrimmedTop(headerH).withTrimmedBottom(stripH);
    };

    // Calculate Section Rects (Must match Paint logic!)
    auto topRow = area.removeFromTop(area.getHeight() / 2);
    auto arpArea = topRow.removeFromLeft(200);
    auto lfoArea = topRow.removeFromLeft(300);
    auto screenArea = topRow; 
    
    auto botRow = area; // Remaining area above keyboard
    auto oscArea = botRow.removeFromLeft(350);
    auto vcfArea = botRow.removeFromLeft(220);
    auto vcaArea = botRow.removeFromLeft(80);
    auto hpfArea = botRow.removeFromLeft(80);
    auto envArea = botRow; 

    // --- OSC SECTION ---
    {
        auto r = getSectionBody(oscArea);
        // DCO1 (2 sliders) | DCO2 (3 sliders) | MIX (3 sliders) = 8 sliders
        int w = r.getWidth() / 8;
        param_dco1_range.setBounds(r.removeFromLeft(w));
        param_dco1_pwm.setBounds(r.removeFromLeft(w));
        
        param_dco2_pitch.setBounds(r.removeFromLeft(w));
        param_dco2_tone.setBounds(r.removeFromLeft(w));
        param_dco2_lvl.setBounds(r.removeFromLeft(w));
        
        param_e_saw_lvl.setBounds(r.removeFromLeft(w));
        param_e_pulse_lvl.setBounds(r.removeFromLeft(w));
        param_e_noise_lvl.setBounds(r.removeFromLeft(w));
    }
    
    // --- VCF SECTION ---
    {
        auto r = getSectionBody(vcfArea);
         // Freq, Res, Env, Lfo, Kybd + Type Selector
        auto typeRect = r.removeFromBottom(20);
        cmb_vcf_type.setBounds(typeRect);
        
        int w = r.getWidth() / 5;
        param_vcf_freq.setBounds(r.removeFromLeft(w));
        param_vcf_res.setBounds(r.removeFromLeft(w));
        param_vcf_env.setBounds(r.removeFromLeft(w));
        param_vcf_lfo.setBounds(r.removeFromLeft(w));
        param_vcf_kybd.setBounds(r.removeFromLeft(w));
    }
    
    // --- VCA SECTION ---
    {
         // Placeholder
    }
    
    // --- HPF SECTION ---
    {
        auto r = getSectionBody(hpfArea);
        param_hpf_freq.setBounds(r);
    }
    
    // --- ENVELOPES SECTION ---
    {
        auto r = getSectionBody(envArea);
        // VCA ADSR
        int w = r.getWidth() / 4;
        param_vca_a.setBounds(r.removeFromLeft(w));
        param_vca_d.setBounds(r.removeFromLeft(w));
        param_vca_s.setBounds(r.removeFromLeft(w));
        param_vca_r.setBounds(r.removeFromLeft(w));
    }

    // --- LFO SECTION ---
    {
        auto r = getSectionBody(lfoArea);
        // LFO1 (Rate, Delay) | LFO2 (Rate, Delay)
        auto lfo1 = r.removeFromLeft(r.getWidth()/2);
        auto lfo2 = r;
        
        int w1 = lfo1.getWidth() / 2;
        param_lfo1_rate.setBounds(lfo1.removeFromLeft(w1));
        param_lfo1_delay.setBounds(lfo1);
        
        int w2 = lfo2.getWidth() / 2;
        param_lfo2_rate.setBounds(lfo2.removeFromLeft(w2));
        param_lfo2_delay.setBounds(lfo2);
    }

    // --- ARP SECTION ---
    {
        auto r = getSectionBody(arpArea);
        btnArp.setBounds(r.reduced(10, 20)); 
    }
    
    // --- SCREEN / PROGRAMMER ---
    {
        auto r = getSectionBody(screenArea);
        // Preset stuff on left, Mod/Fx/Sys buttons on bottom
        
        auto buttons = r.removeFromBottom(30);
        btnMod.setBounds(buttons.removeFromLeft(80));
        btnFx.setBounds(buttons.removeFromLeft(80));
        btnLoadSysex.setBounds(buttons.removeFromLeft(80));
        
        auto presets = r.removeFromTop(30);
        btnSavePreset.setBounds(presets.removeFromRight(60));
        cmbPresets.setBounds(presets);
    }
    
    // Overlay (Full Screen)
    overlay.setBounds(getLocalBounds());
}
