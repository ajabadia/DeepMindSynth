#pragma once
#include <JuceHeader.h>

namespace gui
{
    class DeepMindLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        DeepMindLookAndFeel();
        
        void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                              float sliderPos, float minSliderPos, float maxSliderPos,
                              const juce::Slider::SliderStyle, juce::Slider& slider) override;
    };
}
