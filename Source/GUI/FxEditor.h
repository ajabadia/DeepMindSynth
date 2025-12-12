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

        setSize(600, 300);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xFF202225));
        g.setColour(juce::Colours::pink);
        g.drawRect(getLocalBounds(), 2);
        g.setFont(20.0f);
        g.drawText("EFFECTS CHAIN (DeepMind Style)", getLocalBounds().removeFromTop(40), juce::Justification::centred);
        
        // Dividers
        g.setColour(juce::Colours::grey);
        int sectionW = getWidth() / 3;
        g.drawVerticalLine(sectionW, 40, getHeight() - 10);
        g.drawVerticalLine(sectionW * 2, 40, getHeight() - 10);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(10);
        area.removeFromTop(40);

        int sectionW = area.getWidth() / 3;
        int sliderH = area.getHeight() / 2; // Two rows? Or just fit 3 in grid

        // Helper to layout a section
        auto layoutSection = [&](juce::Rectangle<int> r, 
                                 juce::Slider& s1, juce::Label& l1,
                                 juce::Slider& s2, juce::Label& l2,
                                 juce::Slider& s3, juce::Label& l3)
        {
            int itemW = r.getWidth() / 3;
            // Top Row (Labels potentially? No using helper)
            
            auto col1 = r.removeFromLeft(itemW).reduced(5);
            l1.setBounds(col1.removeFromTop(20)); s1.setBounds(col1);
            
            auto col2 = r.removeFromLeft(itemW).reduced(5);
            l2.setBounds(col2.removeFromTop(20)); s2.setBounds(col2);
            
            auto col3 = r.reduced(5);
            l3.setBounds(col3.removeFromTop(20)); s3.setBounds(col3);
        };

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
        layoutSection(area, 
                     sldReverbMix, lblReverbMix, 
                     sldReverbSize, lblReverbSize, 
                     sldReverbDamp, lblReverbDamp);
    }

private:
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
};
