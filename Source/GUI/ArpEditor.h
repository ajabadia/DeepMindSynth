#pragma once
#include <JuceHeader.h>

class ArpEditor : public juce::Component
{
public:
    ArpEditor(juce::AudioProcessorValueTreeState& apvts)
    {
        // ARP ON
        addAndMakeVisible(btnOn);
        btnOn.setButtonText("Arp On");
        attOn = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "arp_on", btnOn);

        // MODE
        addAndMakeVisible(cmbMode);
        cmbMode.addItem("Up", 1);
        cmbMode.addItem("Down", 2);
        cmbMode.addItem("UpDown", 3);
        cmbMode.addItem("Random", 4);
        cmbMode.addItem("Chord", 5);
        cmbMode.addItem("Pattern", 6);
        attMode = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "arp_mode", cmbMode);

        // RATE
        addAndMakeVisible(sldRate);
        sldRate.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        sldRate.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        attRate = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "arp_rate", sldRate);
        
        // GATE
        addAndMakeVisible(sldGate);
        sldGate.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        sldGate.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        attGate = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "arp_gate", sldGate);

        // OCTAVES
        addAndMakeVisible(sldOct);
        sldOct.setSliderStyle(juce::Slider::LinearBar);
        // PATTERN
        addAndMakeVisible(cmbPattern);
        cmbPattern.addItem("Up 4", 1);
        cmbPattern.addItem("UpDown 8", 2);
        cmbPattern.addItem("Oct Jump", 3);
        cmbPattern.addItem("Randomish", 4);
        cmbPattern.addItem("Rhythmic", 5);
        attPattern = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "arp_pattern", cmbPattern);

        setSize(500, 300); // Slightly wider
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xFF2D2D2D));
        g.setColour(juce::Colours::orange);
        g.drawRect(getLocalBounds(), 2);
        g.setFont(20.0f);
        g.drawText("ARPEGGIATOR", getLocalBounds().removeFromTop(40), juce::Justification::centred);
        
        g.setFont(14.0f);
        g.setColour(juce::Colours::white);
        g.drawText("Mode", cmbMode.getX(), cmbMode.getY() - 20, cmbMode.getWidth(), 20, juce::Justification::centred);
        g.drawText("Rate", sldRate.getX(), sldRate.getY() - 20, sldRate.getWidth(), 20, juce::Justification::centred);
        g.drawText("Gate", sldGate.getX(), sldGate.getY() - 20, sldGate.getWidth(), 20, juce::Justification::centred);
        g.drawText("Octaves", sldOct.getX(), sldOct.getY() - 20, sldOct.getWidth(), 20, juce::Justification::centred);
        g.drawText("Pattern", cmbPattern.getX(), cmbPattern.getY() - 20, cmbPattern.getWidth(), 20, juce::Justification::centred);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(20);
        area.removeFromTop(40); // Title

        auto topRow = area.removeFromTop(40);
        btnOn.setBounds(topRow.removeFromLeft(80));
        
        area.removeFromTop(20); // Spacer

        auto controls = area.removeFromTop(100);
        cmbMode.setBounds(controls.removeFromLeft(100).reduced(5));
        sldRate.setBounds(controls.removeFromLeft(80).reduced(5));
        sldGate.setBounds(controls.removeFromLeft(80).reduced(5));
        sldOct.setBounds(controls.removeFromLeft(80).reduced(5, 30));
        cmbPattern.setBounds(controls.removeFromLeft(100).reduced(5, 30)); // Added
    }

private:
    juce::ToggleButton btnOn;
    juce::ComboBox cmbMode;
    juce::Slider sldRate, sldGate, sldOct;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attOn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attMode;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attRate, attGate, attOct;
    
    juce::ComboBox cmbPattern;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attPattern;
};
