#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "GUI/ArpEditor.h"
#include "GUI/ModMatrixEditor.h"
#include "GUI/FxEditor.h"
#include "Data/SysexTranslator.h"
#include "Data/SysexInjector.h"

DeepMindSynthAudioProcessorEditor::DeepMindSynthAudioProcessorEditor (DeepMindSynthAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      midiKeyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    juce::LookAndFeel::setDefaultLookAndFeel(&lnf);
    setSize (1200, 800);
    
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

    // --- Unison Overlay Controls ---
    addAndMakeVisible(cmb_poly_mode);
    juce::StringArray polyModes = { 
        "Poly", "Unison-2", "Unison-3", "Unison-4", "Unison-6", "Unison-12", 
        "Mono", "Mono-2", "Mono-3", "Mono-4", "Mono-6", "Poly-6", "Poly-8" 
    };
    for(int i=0; i<polyModes.size(); ++i)
        cmb_poly_mode.addItem(polyModes[i], i+1);
        
    att_poly_mode = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "polyphony_mode", cmb_poly_mode);
    
    setupSlider(param_unison_detune, "unison_detune", att_unison_detune);
    
    setupSlider(param_drift, "drift", att_drift);

    // --- OSCILLATORS ---
    setupSlider(param_dco1_range, "dco1_range", att_dco1_range);
    setupSlider(param_dco1_pwm,   "dco1_pwm",   att_dco1_pwm);
    setupSlider(param_dco1_pulse_en, "dco1_pulse_en", att_dco1_pulse_en);
    setupSlider(param_dco1_saw_en,   "dco1_saw_en",   att_dco1_saw_en);
    
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
    // --- ENVELOPES (Switched) ---
    addAndMakeVisible(btnEnvVca);
    addAndMakeVisible(btnEnvVcf);
    addAndMakeVisible(btnEnvMod);
    
    btnEnvVca.setClickingTogglesState(true);
    btnEnvVcf.setClickingTogglesState(true);
    btnEnvMod.setClickingTogglesState(true);
    
    btnEnvVca.setRadioGroupId(101);
    btnEnvVcf.setRadioGroupId(101);
    btnEnvMod.setRadioGroupId(101);
    
    btnEnvVca.setToggleState(true, juce::dontSendNotification);
    
    auto updateEnvUI = [this](){
        updateEnvelopeAttachments();
    };
    
    btnEnvVca.onClick = [this, updateEnvUI](){ currentEnvSelection=0; updateEnvUI(); };
    btnEnvVcf.onClick = [this, updateEnvUI](){ currentEnvSelection=1; updateEnvUI(); };
    btnEnvMod.onClick = [this, updateEnvUI](){ currentEnvSelection=2; updateEnvUI(); };
    
    // Sliders (No Attachments yet)
    addAndMakeVisible(param_env_a); param_env_a.setSliderStyle(juce::Slider::LinearVertical); param_env_a.setTextBoxStyle(juce::Slider::NoTextBox, false, 0,0);
    addAndMakeVisible(param_env_d); param_env_d.setSliderStyle(juce::Slider::LinearVertical); param_env_d.setTextBoxStyle(juce::Slider::NoTextBox, false, 0,0);
    addAndMakeVisible(param_env_s); param_env_s.setSliderStyle(juce::Slider::LinearVertical); param_env_s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0,0);
    addAndMakeVisible(param_env_r); param_env_r.setSliderStyle(juce::Slider::LinearVertical); param_env_r.setTextBoxStyle(juce::Slider::NoTextBox, false, 0,0);
    addAndMakeVisible(param_env_curve); param_env_curve.setSliderStyle(juce::Slider::LinearVertical); param_env_curve.setTextBoxStyle(juce::Slider::NoTextBox, false, 0,0);
    
    updateEnvelopeAttachments();

    // --- LFOs ---
    setupSlider(param_lfo1_rate,  "lfo1_rate",  att_lfo1_rate);
    setupSlider(param_lfo1_delay, "lfo1_delay", att_lfo1_delay);
    setupSlider(param_lfo2_rate,  "lfo2_rate",  att_lfo2_rate);
    setupSlider(param_lfo2_delay, "lfo2_delay", att_lfo2_delay);
    
    addAndMakeVisible(cmb_lfo1_shape);
    addAndMakeVisible(cmb_lfo2_shape);
    juce::StringArray lfoShapes = { "Sine", "Triangle", "Square", "Saw", "Inv Saw", "S&H", "S&G" };
    for(int i=0; i<lfoShapes.size(); ++i) {
        cmb_lfo1_shape.addItem(lfoShapes[i], i+1);
        cmb_lfo2_shape.addItem(lfoShapes[i], i+1);
    }
    att_lfo1_shape = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "lfo1_shape", cmb_lfo1_shape);
    att_lfo2_shape = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "lfo2_shape", cmb_lfo2_shape);

    // --- KEYBOARD ---
    addAndMakeVisible(midiKeyboard);
    midiKeyboard.setAvailableRange(36, 96); // 61 Keys (C2 to C7) for fuller look
    
    // --- BUTTONS ---
    addAndMakeVisible(btnArp);
    // addAndMakeVisible(btnMod);
    // addAndMakeVisible(btnFx);
    addAndMakeVisible(btnLoadSysex); 
    
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
    
    /*
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
    */
    
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
                        // 1. Ask for Destination
                        auto* w = new juce::AlertWindow("Load SysEx to...", "Choose Destination Bank/Program", juce::AlertWindow::NoIcon);
                        w->addComboBox("bank", { "Bank A", "Bank B", "Bank C", "Bank D", "Bank E", "Bank F", "Bank G", "Bank H" }, "Bank A");
                        w->addTextEditor("prog", "0", "Program (0-127):");
                        
                        w->addButton("Load", 1, juce::KeyPress(juce::KeyPress::returnKey));
                        w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
                        
                        w->enterModalState(true, juce::ModalCallbackFunction::create([this, w, sysexData](int res){
                            if (res == 1)
                            {
                                int bank = w->getComboBoxComponent("bank")->getSelectedId() - 1; // 0-7
                                int prog = w->getTextEditorContents("prog").getIntValue();
                                prog = juce::jlimit(0, 127, prog);
                                
                                // 2. Process
                                juce::MidiMessage msg(sysexData.getData(), (int)sysexData.getSize());
                                
                                // Inject
                                auto newMsg = data::SysexInjector::injectTarget(msg, 0, bank, prog); // Default Device ID 0
                                
                                // Send
                                if (audioProcessor.midiManager)
                                {
                                    // Parse for UI update (optional, keeps UI in sync if we overwrite current)
                                    // ...
                                    
                                    // Send to Hardware
                                    // We need a direct 'sendSysex' or 'sendMidi' method exposed?
                                    // midiManager->processOutgoingMidi handles buffer...
                                    // But for SysEx bulk, we might need direct access.
                                    // For now, let's assume midiManager processes it or we send via `applySysex` which usually updates internal state.
                                    // Wait, if we want to send TO THE SYNTH, we need MIDI Output.
                                    // Current `midiManager` handles INPUT handling (applySysex updates plugin params).
                                    // If this is a CONTROLLER, we need to send the SysEx out.
                                    // Assuming standalone application has output device enabled?
                                    // We'll queue it.
                                    
                                    // NOTE: User asked to "Load Libraries" to the synth.
                                    // Currently `applySysex` updates the PLUGIN.
                                    // To update hardware, we output the message.
                                    audioProcessor.midiManager->sendSysexMessage(newMsg);
                                }
                                
                                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Sent", 
                                    "SysEx sent to Bank " + juce::String(char('A' + bank)) + " Program " + juce::String(prog));
                            }
                            delete w;
                        }));
                    }
                }
            });
    };
    
    // Overlay (Must be last to be on top)
    addAndMakeVisible(overlay);
    overlay.setVisible(false);
    
    // CPU Meter
    addAndMakeVisible(lblCpu);
    lblCpu.setColour(juce::Label::textColourId, juce::Colours::white);
    lblCpu.setFont(juce::Font(12.0f));
    lblCpu.setJustificationType(juce::Justification::right);
    
    startTimer(500); // 2Hz refresh
    
    // Chord
    addAndMakeVisible(btnChord);
    addAndMakeVisible(btnLearn);
    btnChord.setClickingTogglesState(true);
    btnLearn.setClickingTogglesState(true);
    
    addAndMakeVisible(btnSeqEdit);
    btnSeqEdit.onClick = [this](){
        if (ctrlSeqEditor) {
            // ctrlSeqEditor->setVisible(true); // Overlay handles visibility
            overlay.showComponent(ctrlSeqEditor.get()); 
        }
    };
    
    // Init Overlay Content
    ctrlSeqEditor = std::make_unique<ControlSeqEditor>(audioProcessor.apvts);
    addChildComponent(ctrlSeqEditor.get());
    
    btnChord.onClick = [this](){
        audioProcessor.chordMemory.setEnabled(btnChord.getToggleState());
    };
    
    btnLearn.onClick = [this](){
        // Exclusive Learn
        bool learn = btnLearn.getToggleState();
        audioProcessor.chordMemory.setLearnMode(learn);
        if (learn) btnChord.setToggleState(false, juce::dontSendNotification); // Auto disable chord play when learning
    };
}

void DeepMindSynthAudioProcessorEditor::timerCallback()
{
    float cpu = audioProcessor.getCpuUsage();
    int lastNote = audioProcessor.lastNoteTriggered.load();
    juce::String txt = "CPU: " + juce::String(cpu, 1) + "%";
    if (lastNote >= 0) txt += "  Note: " + juce::String(lastNote);
    
    lblCpu.setText(txt, juce::dontSendNotification);
    
    // Color warning
    if (cpu > 80.0f) lblCpu.setColour(juce::Label::textColourId, juce::Colours::red);
    else if (cpu > 50.0f) lblCpu.setColour(juce::Label::textColourId, juce::Colours::orange);
    else lblCpu.setColour(juce::Label::textColourId, juce::Colours::white);
}

DeepMindSynthAudioProcessorEditor::~DeepMindSynthAudioProcessorEditor()
{
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void DeepMindSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    // g.fillAll (juce::Colours::red); // DEBUG: RED SCREEN TO VERIFY BUILD
    g.fillAll (juce::Colour (0xFF202225)); // DeepMind Dark Grey
    g.setColour (juce::Colours::white);
    g.setFont (20.0f);
    g.drawText ("DeepMind 12 Clone", 20, 10, 200, 30, juce::Justification::left);
    
    // Draw Section Labels
    // Draw Section Labels
     g.setFont(14.0f);
     g.fillAll (juce::Colour(0xff252525)); // Dark Chassis
 
     // Define Section Colors
     juce::Colour colRed = juce::Colour(0xffd03030); // Voice
     juce::Colour colBlue = juce::Colour(0xff3090d0); // FX/Global
     juce::Colour colWhite = juce::Colour(0xffe0e0e0);
     
     auto area = getLocalBounds();
     auto keyboardArea = area.removeFromBottom(120); 
     
     // 2 Rows Layout Match
     auto topRow = area.removeFromTop(area.getHeight() * 0.55f);
     auto botRow = area;
     
     // --- TOP ROW ---
     auto lfo1Area = topRow.removeFromLeft(topRow.getWidth() * 0.12f);
     auto lfo2Area = topRow.removeFromLeft(topRow.getWidth() * 0.12f);
     auto dco1Area = topRow.removeFromLeft(topRow.getWidth() * 0.20f);
     auto dco2Area = topRow.removeFromLeft(topRow.getWidth() * 0.15f);
     auto vcfArea  = topRow.removeFromLeft(topRow.getWidth() * 0.20f);
     auto vcaArea  = topRow.removeFromLeft(topRow.getWidth() * 0.12f);
     auto hpfArea  = topRow;
     
     drawSection(g, lfo1Area.reduced(3), "LFO 1", colRed);
     drawSection(g, lfo2Area.reduced(3), "LFO 2", colRed);
     drawSection(g, dco1Area.reduced(3), "DCO 1", colRed);
     drawSection(g, dco2Area.reduced(3), "DCO 2", colRed);
     drawSection(g, vcfArea.reduced(3), "VCF", colRed);
     drawSection(g, vcaArea.reduced(3), "ENVELOPES", colRed);
     drawSection(g, hpfArea.reduced(3), "HPF", colBlue);
     
     // --- BOT ROW ---
     auto arpArea = botRow.removeFromLeft(120);
     auto leftControls = botRow.removeFromLeft(200); 
     auto screenArea = botRow.removeFromLeft(300);
     auto modArea = botRow.removeFromLeft(100);
     
     drawSection(g, arpArea.reduced(3), "ARP", colBlue);
     drawSection(g, leftControls.reduced(3), "MIX / UNISON", colRed);
     drawSection(g, screenArea.reduced(3), "PROGRAMMER", colRed);
     drawSection(g, modArea.reduced(3), "MOD MATRIX", colRed);
     // Remainder is FX
     drawSection(g, botRow.reduced(3), "EFFECTS", colBlue);

    // Draw Labels for Sliders (Hardware style printed on chassis)
    g.setColour(juce::Colours::white);
    
    // --- REPLICATE LAYOUT LOGIC FOR LABELS ---
    const int margin = 5;
    const int headerH = 25; 
    const int stripH = 5;
    
    auto getSectionBody = [&](juce::Rectangle<int> sectionRect) {
        return sectionRect.reduced(margin).withTrimmedTop(headerH).withTrimmedBottom(stripH);
    };

    // --- LABEL DRAWING ---
    // Re-use layout constants locally if needed, or rely on visual approximation
    // Actually, to align perfectly, we should use the same `getSectionBody` logic on the SAME rectangles.
    // Since variables like `dco1Area` are local to the top block, they might be out of scope if I put them in {} blocks?
    // No, `paint` has them in main scope.
    // BUT in Step 2781 I might have put them inside `paint` body.
    // However, I need to ensure the variables `dco1Area` etc are accessible here.
    
    // --- LFO 1 ---
    {
        auto r = getSectionBody(lfo1Area);
        r.removeFromTop(20); // Combo
        int w = r.getWidth() / 2;
        drawControlLabel(g, r.removeFromLeft(w), "RATE");
        drawControlLabel(g, r, "DELAY");
    }
    
    // --- LFO 2 ---
    {
        auto r = getSectionBody(lfo2Area);
        r.removeFromTop(20);
        int w = r.getWidth() / 2;
        drawControlLabel(g, r.removeFromLeft(w), "RATE");
        drawControlLabel(g, r, "DELAY");
    }

    // --- DCO 1 ---
    {
        auto r = getSectionBody(dco1Area);
        int w = r.getWidth() / 4;
        
        drawControlLabel(g, r.removeFromLeft(w), "RANGE"); 
        drawControlLabel(g, r.removeFromLeft(w), "PWM");
        drawControlLabel(g, r.removeFromLeft(w), "PULSE"); // Enable
        drawControlLabel(g, r.removeFromLeft(w), "SAW");   // Enable
    }
    
    // --- DCO 2 ---
    {
        auto r = getSectionBody(dco2Area);
         int w = r.getWidth() / 3;
        drawControlLabel(g, r.removeFromLeft(w), "PITCH");
        drawControlLabel(g, r.removeFromLeft(w), "TONE");
        drawControlLabel(g, r.removeFromLeft(w), "LEVEL");
    }
    
    // --- VCF ---
    {
        auto r = getSectionBody(vcfArea);
        r.removeFromBottom(20); // Combo
        int w = r.getWidth() / 5;
        drawControlLabel(g, r.removeFromLeft(w), "FREQ");
        drawControlLabel(g, r.removeFromLeft(w), "RES");
        drawControlLabel(g, r.removeFromLeft(w), "ENV");
        drawControlLabel(g, r.removeFromLeft(w), "LFO");
        drawControlLabel(g, r.removeFromLeft(w), "KYBD");
    }
    
    // --- ENVELOPES ---
    {
        auto r = getSectionBody(vcaArea);
        r.removeFromTop(20); // Buttons space
        int w = r.getWidth() / 5;
        drawControlLabel(g, r.removeFromLeft(w), "A");
        drawControlLabel(g, r.removeFromLeft(w), "D");
        drawControlLabel(g, r.removeFromLeft(w), "S");
        drawControlLabel(g, r.removeFromLeft(w), "R");
        drawControlLabel(g, r.removeFromLeft(w), "CRV");
    }
    
    // --- HPF ---
    {
         auto r = getSectionBody(hpfArea);
         drawControlLabel(g, r, "FREQ");
    }
    
    // --- MIXER (Left Controls) ---
    {
        auto r = getSectionBody(leftControls);
        int w = r.getWidth() / 4;
        drawControlLabel(g, r.removeFromLeft(w), "SAW");
        drawControlLabel(g, r.removeFromLeft(w), "PULSE");
        drawControlLabel(g, r.removeFromLeft(w), "NOISE");
        drawControlLabel(g, r.removeFromLeft(w), "DETUNE");
        drawControlLabel(g, r, "DRIFT");
    }
    
    // --- SCREEN ---
    // No specific labels needed per slider as they are obvious or labeled by group
}

void DeepMindSynthAudioProcessorEditor::drawControlLabel(juce::Graphics& g, juce::Rectangle<int> bounds, juce::String name)
{
    g.setFont(12.0f); // Slightly larger
    g.setColour(juce::Colours::white);
    
    auto labelArea = bounds.removeFromTop(20); 
    g.drawFittedText(name, labelArea, juce::Justification::centred, 1);
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


// Increased Keyboard Height
// --- LAYOUT ---
void DeepMindSynthAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    
    // 1. Keyboard (Full Width, Bottom)
    auto kbArea = area.removeFromBottom(120); 
    midiKeyboard.setBounds(kbArea);
    midiKeyboard.setKeyWidth(kbArea.getWidth() / 36.0f);
    
    // Layout Constants
    const int headerH = 25; 
    const int stripH = 5;
    const int margin = 4;
    
    auto getSectionBody = [&](juce::Rectangle<int> sectionRect) {
        return sectionRect.reduced(margin).withTrimmedTop(headerH).withTrimmedBottom(stripH);
    };

    // 2. Main Module 
    // Hardware-like Flow: LFOs -> DCOs -> VCF -> VCA -> HPF -> FX/Global
    // Layout: 2 Rows.
    // Top Row: LFO 1 | LFO 2 | DCO 1 | DCO 2 | VCF | VCA | HPF
    // Bot Row: Arp | Mod Matrix | Screen/Global | FX
    
    auto topRow = area.removeFromTop(area.getHeight() * 0.55f);
    auto botRow = area; 
    
    // --- TOP ROW ---
    // Total Width ~ 1000px.
    // Ratios: LFO(11%) LFO(11%) DCO1(19%) DCO2(14%) VCF(22%) VCA(16%) HPF(7%)
    
    auto lfo1Area = topRow.removeFromLeft(topRow.getWidth() * 0.11f);
    auto lfo2Area = topRow.removeFromLeft(topRow.getWidth() * 0.11f);
    auto dco1Area = topRow.removeFromLeft(topRow.getWidth() * 0.19f);
    auto dco2Area = topRow.removeFromLeft(topRow.getWidth() * 0.14f);
    auto vcfArea  = topRow.removeFromLeft(topRow.getWidth() * 0.22f); // Increased
    auto vcaArea  = topRow.removeFromLeft(topRow.getWidth() * 0.16f); // Increased
    auto hpfArea  = topRow; // Remainder (~7%)
    
    // --- LFO 1 ---
    {
        auto r = getSectionBody(lfo1Area);
        cmb_lfo1_shape.setBounds(r.removeFromTop(20).reduced(2));
        param_lfo1_rate.setBounds(r.removeFromLeft(r.getWidth()/2));
        param_lfo1_delay.setBounds(r);
    }
    
    // --- LFO 2 ---
    {
        auto r = getSectionBody(lfo2Area);
         cmb_lfo2_shape.setBounds(r.removeFromTop(20).reduced(2));
        param_lfo2_rate.setBounds(r.removeFromLeft(r.getWidth()/2));
        param_lfo2_delay.setBounds(r);
    }
    
    // --- DCO 1 ---
    {
        auto r = getSectionBody(dco1Area);
        int w = r.getWidth() / 4;
        param_dco1_range.setBounds(r.removeFromLeft(w));
        param_dco1_pwm.setBounds(r.removeFromLeft(w)); // Manual PWM
        // Pulse En / Saw En
        param_dco1_pulse_en.setBounds(r.removeFromLeft(w)); 
        param_dco1_saw_en.setBounds(r); 
    }
    
    // --- DCO 2 ---
    {
        auto r = getSectionBody(dco2Area);
        int w = r.getWidth() / 3;
        param_dco2_pitch.setBounds(r.removeFromLeft(w)); // Pitch
        param_dco2_tone.setBounds(r.removeFromLeft(w)); // Tone
        param_dco2_lvl.setBounds(r); // Level
    }
    
    // --- VCF ---
    {
        auto r = getSectionBody(vcfArea);
        cmb_vcf_type.setBounds(r.removeFromBottom(20).reduced(2));
        int w = r.getWidth() / 5;
        param_vcf_freq.setBounds(r.removeFromLeft(w));
        param_vcf_res.setBounds(r.removeFromLeft(w));
        param_vcf_env.setBounds(r.removeFromLeft(w));
        param_vcf_lfo.setBounds(r.removeFromLeft(w));
        param_vcf_kybd.setBounds(r);
    }
    
    // --- ENVELOPES ---
    {
        auto r = getSectionBody(vcaArea);
        auto btnRow = r.removeFromTop(20).reduced(2);
        int btnW = btnRow.getWidth() / 3;
        btnEnvVca.setBounds(btnRow.removeFromLeft(btnW));
        btnEnvVcf.setBounds(btnRow.removeFromLeft(btnW));
        btnEnvMod.setBounds(btnRow);
        
        int w = r.getWidth() / 5;
        param_env_a.setBounds(r.removeFromLeft(w));
        param_env_d.setBounds(r.removeFromLeft(w));
        param_env_s.setBounds(r.removeFromLeft(w));
        param_env_r.setBounds(r.removeFromLeft(w));
        param_env_curve.setBounds(r);
    }
    
    // --- HPF ---
    {
        auto r = getSectionBody(hpfArea);
        param_hpf_freq.setBounds(r);
    }
    
    // --- BOTTOM ROW ---
    // Arp | Screen/Global | Mod Matrix | FX
    
    auto arpArea = botRow.removeFromLeft(120);
    auto leftControls = botRow.removeFromLeft(200); // For Noise/Unison/Glides
    auto screenArea = botRow.removeFromLeft(300);
    auto modArea = botRow.removeFromLeft(100);
    auto fxArea = botRow; // Right side
    
    {
        auto r = getSectionBody(arpArea);
        auto top = r.removeFromTop(24).reduced(2);
        btnArp.setBounds(top);
        
        // Chord / Learn below Arp
        auto bot = r.removeFromBottom(24).reduced(2);
        btnChord.setBounds(bot.removeFromLeft(bot.getWidth()/3));
        btnLearn.setBounds(bot.removeFromLeft(bot.getWidth()/2)); // Split remaining
        btnSeqEdit.setBounds(bot);
    }
    
    // LEFT (Mix/Glide/Unison) - Reusing some leftover sliders
    // For now, put Noise Level here?
    // And DCO 1 Mix levels (Pulse/Saw)?
    // Wait, DCO 1 doesn't have levels in DeepMind hardware? It's on/off sliders typically, or mix faders.
    // Re-checking manual text: "26 sliders... Osc 1 generates saw... pulse... Osc 2 generates square... tone".
    // Usually there is a MIX section.
    // I left 'param_e_saw_lvl' etc out of top row.
    // Putting them in 'leftControls' aka MIXER.
    // Putting them in 'leftControls' aka MIXER.
    {
        auto r = getSectionBody(leftControls);
        int w = r.getWidth() / 5;
        param_e_saw_lvl.setBounds(r.removeFromLeft(w));
        param_e_pulse_lvl.setBounds(r.removeFromLeft(w));
        param_e_noise_lvl.setBounds(r.removeFromLeft(w));
        // Unison Slider here
        param_unison_detune.setBounds(r.removeFromLeft(w));
        // Drift
        param_drift.setBounds(r);
    }
    
    // SCREEN / GLOBAL
    {
        auto r = getSectionBody(screenArea);
        // Top: Preset, Bot: Buttons
        auto bot = r.removeFromBottom(30);
        // btnMod.setBounds(bot.removeFromLeft(80).reduced(2));
        // btnFx.setBounds(bot.removeFromLeft(80).reduced(2));
        btnLoadSysex.setBounds(bot.reduced(2));
        
        // Top
        cmbPresets.setBounds(r.removeFromTop(24).reduced(2));
        cmb_poly_mode.setBounds(r.removeFromTop(24).reduced(2)); 
        // Save
        btnSavePreset.setBounds(r.reduced(10));
        
        // CPU
        lblCpu.setBounds(r.removeFromBottom(15).reduced(2,0));
    }

    // Overlay
    overlay.setBounds(getLocalBounds());
}

void DeepMindSynthAudioProcessorEditor::updateEnvelopeAttachments()
{
    att_env_a = nullptr;
    att_env_d = nullptr;
    att_env_s = nullptr;
    att_env_r = nullptr;
    att_env_curve = nullptr;
    
    juce::String prefix = "";
    if (currentEnvSelection == 0) prefix = "vca_";
    else if (currentEnvSelection == 1) prefix = "vcf_";
    else prefix = "mod_";
    
    att_env_a = std::make_unique<Attachment>(audioProcessor.apvts, prefix + "attack", param_env_a);
    att_env_d = std::make_unique<Attachment>(audioProcessor.apvts, prefix + "decay", param_env_d);
    att_env_s = std::make_unique<Attachment>(audioProcessor.apvts, prefix + "sustain", param_env_s);
    att_env_r = std::make_unique<Attachment>(audioProcessor.apvts, prefix + "release", param_env_r);
    att_env_curve = std::make_unique<Attachment>(audioProcessor.apvts, prefix + "curve", param_env_curve);
}
