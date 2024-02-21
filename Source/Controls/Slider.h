#pragma once

#include <JuceHeader.h>
#include "../Constants.h"

using namespace juce;

//==============================================================================
// Define a custom LookAndFeel to draw the rotary slider
struct myLookAndFeel : juce::LookAndFeel_V4
{
    void drawRotarySlider(juce::Graphics& g,
        int x,
        int y,
        int width,
        int height,
        float sliderPosProportional,
        float rotaryStartAngle,
        float rotaryEndAngle,
        juce::Slider& slider) override;
};

//==============================================================================
// Define a custom RotarySliderWithLabels class inheriting from Slider
struct RotarySliderWithLabels : Slider
{
    // Constructor taking a RangedAudioParameter and a unit suffix
    RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix) :
        juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
            juce::Slider::TextEntryBoxPosition::NoTextBox),
        param(&rap),
        suffix(unitSuffix)
    {
        // Set the custom LookAndFeel
        setLookAndFeel(&lnf);
    }

    // Destructor
    ~RotarySliderWithLabels()
    {
        // Reset the LookAndFeel
        setLookAndFeel(nullptr);
    }

    // Structure to store label positions
    struct LabelPos
    {
        float pos;
        juce::String label;
    };

    juce::Array<LabelPos> labels; // Array to hold label positions

    // Override the paint method to customize the appearance
    void paint(juce::Graphics& g) override;

    // Function to get the bounds of the slider
    juce::Rectangle<int> getSliderBounds() const;

    // Function to get the text height
    int getTextHeight() const
    {
        return 14;
    }

    // Function to get the display string
    String getDisplayString() const;

private:
    myLookAndFeel lnf; // Custom LookAndFeel object
    RangedAudioParameter* param; // Pointer to the associated audio parameter
    String suffix; // Unit suffix
};
