
#include "Histogram.h"

//==============================================================================
// Implementation for the Histogram class
Histogram::Histogram(const juce::String& titleInput) : title(titleInput) {}

void Histogram::paint(juce::Graphics& g)
{
    // Fill the background with a rounded rectangle using the base color
    g.setColour(BASE_COLOR);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4);

    // Set the color and font for drawing the title text
    g.setColour(juce::Colours::white);
    g.setFont(16.f);
    // Draw the title text at the bottom of the component, centered horizontally
    g.drawText(title, getLocalBounds().removeFromBottom(20), juce::Justification::centred);

    // Display the path representing the histogram waveform
    displayPath(g, getLocalBounds().toFloat());

    // Create a border path with rounded corners to outline the component
    Path border;
    border.setUsingNonZeroWinding(false);
    border.addRectangle(getLocalBounds());
    auto bounds = getLocalBounds().toFloat().reduced(1);
    border.addRoundedRectangle(bounds, 4);
    // Fill the border path with the background color
    g.setColour(BACKGROUND_COLOR);
    g.fillPath(border);
}

void Histogram::resized()
{
    // Resize the circular buffer to match the width of the component
    buffer.resize(getWidth(), NEGATIVE_INFINITY);
}

void Histogram::mouseDown(const juce::MouseEvent& e)
{
    // Clear the circular buffer when the mouse is clicked
    buffer.clear(NEGATIVE_INFINITY);
}

void Histogram::update(float value)
{
    // Write the new value to the circular buffer
    buffer.write(value);
    // Trigger a repaint to update the display
    repaint();
}

void Histogram::displayPath(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Build the path to be displayed using the circular buffer data
    juce::Path fill = buildPath(path, buffer, bounds);

    // Check if the path is not empty before proceeding to fill it
    if (!fill.isEmpty())
    {
        // Create a gradient for filling the path, from HIGHLIGHT_COLOR to BASE_COLOR
        ColourGradient gradient(HIGHLIGHT_COLOR.withAlpha(0.8f),
            bounds.getX(), bounds.getY(),
            BASE_COLOR.withAlpha(0.3f),
            bounds.getX(), bounds.getBottom(),
            false);

        // Set the fill type using the gradient
        juce::FillType fillType(gradient);
        g.setFillType(fillType);

        // Fill the path with the gradient
        g.fillPath(fill);
    }
}
