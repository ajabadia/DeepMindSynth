#pragma once
#include <JuceHeader.h>

class FxEditor : public juce::Component
{
public:
    FxEditor(juce::AudioProcessorValueTreeState& apvts)
    {
        auto setupSlider = [&](juce::Slider& s, juce::Label& l, juce::String name, juce::String id, 
                              std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& att)
        {
            addAndMakeVisible(l);
            l.setText(name, juce::dontSendNotification);
            l.setJustificationType(juce::Justification::centred);
            
            addAndMakeVisible(s);
            s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
            att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, id, s);
        };

        // --- CHORUS ---
        setupSlider(sldChorusMix, lblChorusMix, "Ch Mix", "fx_chorus_mix", attChorusMix);
        setupSlider(sldChorusRate, lblChorusRate, "Rate", "fx_chorus_rate", attChorusRate);
        setupSlider(sldChorusDepth, lblChorusDepth, "Depth", "fx_chorus_depth", attChorusDepth);

        // --- DELAY ---
        setupSlider(sldDelayMix, lblDelayMix, "Dly Mix", "fx_delay_mix", attDelayMix);
        setupSlider(sldDelayTime, lblDelayTime, "Time", "fx_delay_time", attDelayTime);
        setupSlider(sldDelayFb, lblDelayFb, "Fback", "fx_delay_feedback", attDelayFb);

        // --- REVERB ---
        setupSlider(sldReverbMix, lblReverbMix, "Rev Mix", "fx_reverb_mix", attReverbMix);
        setupSlider(sldReverbSize, lblReverbSize, "Size", "fx_reverb_size", attReverbSize);
        setupSlider(sldReverbDamp, lblReverbDamp, "Damp", "fx_reverb_damp", attReverbDamp);

        // --- DISTORTION ---
        setupSlider(sldDistDrive, lblDistDrive, "Drive", "fx_dist_drive", attDistDrive);
        setupSlider(sldDistTone, lblDistTone, "Tone", "fx_dist_tone", attDistTone);
        setupSlider(sldDistMix, lblDistMix, "Mix", "fx_dist_mix", attDistMix);
        addAndMakeVisible(cmbDistType);
        cmbDistType.addItem("Tube", 1);
        cmbDistType.addItem("Hard", 2);
        cmbDistType.addItem("Rect", 3);
        cmbDistType.addItem("Crush", 4);
        attDistType = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "fx_dist_type", cmbDistType);
        
        // --- PHASER ---
        setupSlider(sldPhaserMix, lblPhaserMix, "Mix", "fx_phaser_mix", attPhaserMix);
        setupSlider(sldPhaserRate, lblPhaserRate, "Rate", "fx_phaser_rate", attPhaserRate);
        setupSlider(sldPhaserDepth, lblPhaserDepth, "Depth", "fx_phaser_depth", attPhaserDepth);

        // --- EQ ---
        setupSlider(sldEqLow,  lblEqLow,  "Low", "fx_eq_low_gain", attEqLow);
        setupSlider(sldEqMid,  lblEqMid,  "Mid", "fx_eq_lm_gain",  attEqMid);
        setupSlider(sldEqHigh, lblEqHigh, "Hi",  "fx_eq_high_gain", attEqHigh);

        addAndMakeVisible(btnRouting);
        attRouting = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "fx_routing", btnRouting);

        setSize(1200, 260); // Wider for 6 modules
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xFF202225));
        g.setColour(juce::Colours::pink);
        g.drawRect(getLocalBounds(), 2);
        g.setFont(20.0f);
        g.drawText("EFFECTS CHAIN", getLocalBounds().removeFromTop(30), juce::Justification::centred);
        
        // Dividers
        g.setColour(juce::Colours::grey);
        int sectionW = getWidth() / 6; // 6 sections
        for(int i=1; i<6; ++i) g.drawVerticalLine(sectionW * i, 30, getHeight() - 10);
        
        g.setFont(14.0f);
        g.setColour(juce::Colours::white);
        g.drawText("DIST", 0, 30, sectionW, 20, juce::Justification::centred);
        g.drawText("PHASER", sectionW, 30, sectionW, 20, juce::Justification::centred);
        g.drawText("CHORUS", sectionW*2, 30, sectionW, 20, juce::Justification::centred);
        g.drawText("DELAY", sectionW*3, 30, sectionW, 20, juce::Justification::centred);
        g.drawText("REVERB", sectionW*4, 30, sectionW, 20, juce::Justification::centred);
        g.drawText("EQ",     sectionW*5, 30, sectionW, 20, juce::Justification::centred);
        
        // Draw routing lines/arrows potentially?
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(10);
        
        // Put Routing Button in Header
        btnRouting.setBounds(area.removeFromTop(40).removeFromLeft(120));

        int sectionW = area.getWidth() / 6;

        // Helper to layout a section
        auto layoutSection = [&](juce::Rectangle<int> r, 
                                 juce::Slider& s1, juce::Label& l1,
                                 juce::Slider& s2, juce::Label& l2,
                                 juce::Slider& s3, juce::Label& l3)
        {
            int itemW = r.getWidth() / 3;
            auto col1 = r.removeFromLeft(itemW).reduced(2);
            l1.setBounds(col1.removeFromTop(15)); s1.setBounds(col1);
            
            auto col2 = r.removeFromLeft(itemW).reduced(2);
            l2.setBounds(col2.removeFromTop(15)); s2.setBounds(col2);
            
            auto col3 = r.reduced(2);
            l3.setBounds(col3.removeFromTop(15)); s3.setBounds(col3);
        };
        
        // DIST
        auto distArea = area.removeFromLeft(sectionW);
        // Type Combo at bottom or top? Let's put at bottom
        cmbDistType.setBounds(distArea.removeFromBottom(24).reduced(5));
        layoutSection(distArea, sldDistDrive, lblDistDrive, sldDistTone, lblDistTone, sldDistMix, lblDistMix);

        // Phaser
        layoutSection(area.removeFromLeft(sectionW), 
                     sldPhaserMix, lblPhaserMix, 
                     sldPhaserRate, lblPhaserRate, 
                     sldPhaserDepth, lblPhaserDepth);

        // Chorus
        layoutSection(area.removeFromLeft(sectionW), 
                     sldChorusMix, lblChorusMix, 
                     sldChorusRate, lblChorusRate, 
                     sldChorusDepth, lblChorusDepth);

        // Delay
        layoutSection(area.removeFromLeft(sectionW), 
                     sldDelayMix, lblDelayMix, 
                     sldDelayTime, lblDelayTime, 
                     sldDelayFb, lblDelayFb);
                     
        // Reverb
        layoutSection(area.removeFromLeft(sectionW), 
                     sldReverbMix, lblReverbMix, 
                     sldReverbSize, lblReverbSize, 
                     sldReverbDamp, lblReverbDamp);
                     
        // EQ
        layoutSection(area, 
                     sldEqLow, lblEqLow, 
                     sldEqMid, lblEqMid, 
                     sldEqHigh, lblEqHigh);
    }


private:
    // Distortion
    juce::Label lblDistDrive, lblDistTone, lblDistMix;
    juce::Slider sldDistDrive, sldDistTone, sldDistMix;
    juce::ComboBox cmbDistType;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attDistDrive, attDistTone, attDistMix;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attDistType;

    // Phaser
    juce::Label lblPhaserMix, lblPhaserRate, lblPhaserDepth;
    juce::Slider sldPhaserMix, sldPhaserRate, sldPhaserDepth;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attPhaserMix, attPhaserRate, attPhaserDepth;

    // Chorus
    juce::Label lblChorusMix, lblChorusRate, lblChorusDepth;
    juce::Slider sldChorusMix, sldChorusRate, sldChorusDepth;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attChorusMix, attChorusRate, attChorusDepth;

    // Delay
    juce::Label lblDelayMix, lblDelayTime, lblDelayFb;
    juce::Slider sldDelayMix, sldDelayTime, sldDelayFb;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attDelayMix, attDelayTime, attDelayFb;

    // Reverb
    juce::Label lblReverbMix, lblReverbSize, lblReverbDamp;
    juce::Slider sldReverbMix, sldReverbSize, sldReverbDamp;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attReverbMix, attReverbSize, attReverbDamp;

    // EQ (Simplified UI: 3 Gains)
    juce::Label lblEqLow, lblEqMid, lblEqHigh;
    juce::Slider sldEqLow, sldEqMid, sldEqHigh;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attEqLow, attEqMid, attEqHigh;
    
    // Routing
    juce::ToggleButton btnRouting { "Parallel Mode" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attRouting;
};
