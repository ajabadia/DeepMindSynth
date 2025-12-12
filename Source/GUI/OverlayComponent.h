#pragma once
#include <JuceHeader.h>

// A generic overlay component that covers the parent and shows a content component
class OverlayComponent : public juce::Component
{
public:
    OverlayComponent()
    {
        // Semi-transparent background
    }
    
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black.withAlpha(0.75f));
    }
    
    void setContent(std::unique_ptr<juce::Component> newContent)
    {
        content = std::move(newContent);
        addAndMakeVisible(content.get());
        resized();
    }
    
    void resized() override
    {
        if (content)
        {
            // Center the content, with max size
            int w = juce::jmin(800, getWidth() - 40);
            int h = juce::jmin(600, getHeight() - 40);
            content->setBounds(getLocalBounds().getCentre().getX() - w/2, getLocalBounds().getCentre().getY() - h/2, w, h);
        }
    }
    
    void mouseDown(const juce::MouseEvent& e) override
    {
        // Close on click (simple dismissal for Phase 4)
        setVisible(false);
    }
    
    void clearContent()
    {
        content = nullptr;
    }

private:
    std::unique_ptr<juce::Component> content;
};
