#include "DeepMindLookAndFeel.h"

using namespace gui;

DeepMindLookAndFeel::DeepMindLookAndFeel()
{
}

void DeepMindLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float minSliderPos, float maxSliderPos,
                                         const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    // DeepMind Style: Dark track, Orange/Amber Cap, LED strip feel
    
    // 1. Draw Track
    auto trackWidth = 4.0f;
    juce::Rectangle<float> track (x + width * 0.5f - trackWidth * 0.5f, y + 10, trackWidth, height - 20);
    
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (track, 2.0f);
    
    g.setColour (juce::Colours::darkgrey); // Border
    g.drawRoundedRectangle (track, 2.0f, 1.0f);

    // 2. Draw Cap (Thumb)
    auto thumbWidth = 24.0f;
    auto thumbHeight = 12.0f;
    
    juce::Rectangle<float> thumb;
    thumb.setBounds (x + width * 0.5f - thumbWidth * 0.5f,
                     sliderPos - thumbHeight * 0.5f,
                     thumbWidth, thumbHeight);
                     
    // Amber/Orange Gradient for the cap
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xFFFF9900), thumb.getX(), thumb.getY(),
                                             juce::Colour (0xFFCC6600), thumb.getX(), thumb.getBottom(), false));
    g.fillRoundedRectangle (thumb, 2.0f);
    
    // Cap Detail
    g.setColour (juce::Colours::white.withAlpha(0.6f));
    g.drawLine (thumb.getX() + 2, thumb.getCentreY(), thumb.getRight() - 2, thumb.getCentreY(), 1.0f);
}
