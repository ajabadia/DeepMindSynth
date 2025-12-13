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
        ownedContent = std::move(newContent);
        activeContent = ownedContent.get();
        addAndMakeVisible(activeContent);
        resized();
        setVisible(true);
    }
    
    // Show existing component (Non-owning)
    void showComponent(juce::Component* componentToShow)
    {
        activeContent = componentToShow;
        addAndMakeVisible(activeContent);
        resized();
        setVisible(true);
    }
    
    void resized() override
    {
        if (activeContent)
        {
            // Center the content, with max size
            int w = juce::jmin(800, getWidth() - 40);
            int h = juce::jmin(600, getHeight() - 40);
            activeContent->setBounds(getLocalBounds().getCentre().getX() - w/2, getLocalBounds().getCentre().getY() - h/2, w, h);
        }
    }
    
    void mouseDown(const juce::MouseEvent& e) override
    {
        // Close on click (simple dismissal)
        setVisible(false);
    }
    
    void clearContent()
    {
        ownedContent = nullptr;
        activeContent = nullptr;
    }

private:
    std::unique_ptr<juce::Component> ownedContent;
    juce::Component* activeContent = nullptr;
};
