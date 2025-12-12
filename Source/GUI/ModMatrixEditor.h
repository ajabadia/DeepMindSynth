#pragma once
#include <JuceHeader.h>

class ModMatrixEditor : public juce::Component
{
public:
    ModMatrixEditor(juce::AudioProcessorValueTreeState& apvts)
    {
        for (int i = 0; i < 8; ++i)
        {
            auto row = std::make_unique<ModSlotRow>(apvts, i + 1);
            addAndMakeVisible(row.get());
            rows.add(std::move(row));
        }
        setSize(600, 450);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xFF2D2D2D));
        g.setColour(juce::Colours::cyan);
        g.drawRect(getLocalBounds(), 2);
        g.setFont(20.0f);
        g.drawText("MODULATION MATRIX", getLocalBounds().removeFromTop(40), juce::Justification::centred);
        
        // Headers
        g.setFont(14.0f);
        g.setColour(juce::Colours::white);
        int y = 45;
        g.drawText("Slot", 10, y, 40, 20, juce::Justification::centred);
        g.drawText("Source", 60, y, 150, 20, juce::Justification::centred);
        g.drawText("Destination", 220, y, 150, 20, juce::Justification::centred);
        g.drawText("Amount", 380, y, 200, 20, juce::Justification::centred);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(10);
        area.removeFromTop(60); // Title + Headers

        for (auto* row : rows)
        {
            row->setBounds(area.removeFromTop(40));
            area.removeFromTop(5); // Gap
        }
    }

private:
    struct ModSlotRow : public juce::Component
    {
        ModSlotRow(juce::AudioProcessorValueTreeState& apvts, int index)
        {
            juce::String prefix = "mod_slot_" + juce::String(index);
            
            // Label
            lblIndex.setText(juce::String(index), juce::dontSendNotification);
            addAndMakeVisible(lblIndex);
            
            // Source
            addAndMakeVisible(cmbSrc);
            // Populate similar to PluginProcessor (Manual copy for now or shared list)
            juce::StringArray modSources = { "None", "LFO1", "LFO2", "EnvMod", "Velocity", "ModWheel", "KeyTrack" };
            for(int i=0; i<modSources.size(); ++i) cmbSrc.addItem(modSources[i], i+1);
            attSrc = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, prefix + "_src", cmbSrc);

            // Dest
            addAndMakeVisible(cmbDst);
            juce::StringArray modDests = { "None", "Osc1 Pitch", "Osc1 PWM", "Osc2 Pitch", "VCF Freq", "VCF Res" };
            for(int i=0; i<modDests.size(); ++i) cmbDst.addItem(modDests[i], i+1);
            attDst = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, prefix + "_dst", cmbDst);
            
            // Amount
            addAndMakeVisible(sldAmt);
            sldAmt.setSliderStyle(juce::Slider::LinearHorizontal);
            sldAmt.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
            attAmt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, prefix + "_amt", sldAmt);
        }
        
        void resized() override
        {
            auto r = getLocalBounds();
            lblIndex.setBounds(r.removeFromLeft(40));
            r.removeFromLeft(10);
            cmbSrc.setBounds(r.removeFromLeft(150));
            r.removeFromLeft(10);
            cmbDst.setBounds(r.removeFromLeft(150));
            r.removeFromLeft(10);
            sldAmt.setBounds(r);
        }

        juce::Label lblIndex;
        juce::ComboBox cmbSrc, cmbDst;
        juce::Slider sldAmt;
        
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attSrc, attDst;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attAmt;
    };

    juce::OwnedArray<ModSlotRow> rows;
};
