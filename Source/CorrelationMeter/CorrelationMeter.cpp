
#include "CorrelationMeter.h"

//==============================================================================
// Implementation for the CorrelationMeter class
CorrelationMeter::CorrelationMeter(juce::AudioBuffer<float>& buf, double sampleRate) : buffer(buf)
{
    // Initialize the filters with low-pass coefficients
    for (auto& filter : filters)
    {
        auto coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 20000.f);
        filter.coefficients = coefficients;
    }
}

void CorrelationMeter::paint(juce::Graphics& g)
{
    // Fill the background with the base color
    g.setColour(BASE_COLOR);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 3);

    // Divide the area into two parts: slowBounds and fastBounds
    auto slowBounds = getLocalBounds().removeFromTop(getLocalBounds().getHeight() / 3);

    // Draw the average for peakAverager in slowBounds with a border
    drawAverage(g, slowBounds, peakAverager.getAvg(), true);

    // Draw the average for slowAverager in fastBounds with a border
    drawAverage(g, getLocalBounds(), slowAverager.getAvg(), true);

    // Draw the border around the component
    Path border;
    border.setUsingNonZeroWinding(false);
    border.addRectangle(getLocalBounds());
    auto bounds = getLocalBounds().toFloat().reduced(1);
    border.addRoundedRectangle(bounds, 3);
    g.setColour(BACKGROUND_COLOR);
    g.fillPath(border);
}

void CorrelationMeter::update(juce::int64 average_time)
{
    auto numSamples = buffer.getNumSamples();

    // Set the duration for averaging for both slow and peak averagers
    peakAverager.setAveragerDuration(average_time);
    slowAverager.setAveragerDuration(average_time);

    for (int i = 0; i < numSamples; i++)
    {
        auto left = buffer.getSample(0, i);
        auto right = buffer.getSample(1, i);

        // Calculate correlation using the filters
        auto numerator = filters[0].processSample(left * right);
        auto denominator = sqrt(filters[1].processSample(left * left) * filters[2].processSample(right * right));

        if (std::isnan(numerator) || std::isinf(numerator) ||
            std::isnan(denominator) || std::isinf(denominator) || denominator == 0.0f)
        {
            // Handle special cases where correlation calculation fails
            peakAverager.add(0.f);
            slowAverager.add(0.f);
        }
        else
        {
            // Calculate correlation and add it to both averagers
            auto correlation = numerator / denominator;
            peakAverager.add(correlation);
            slowAverager.add(correlation);
        }
    }

    // Repaint the component after updating the correlation
    repaint();
}

void CorrelationMeter::drawAverage(juce::Graphics& g, juce::Rectangle<int> bounds, float avg, bool drawBorder)
{
    // Map the average value to the width of the bounds
    int width = juce::jmap(avg, -1.0f, 1.0f, 0.f, (float)bounds.getWidth());

    // Create a rectangle representing the average value
    juce::Rectangle<int> rect;
    if (avg >= 0)
    {
        rect.setBounds(bounds.getWidth() / 2, 0, width - bounds.getWidth() / 2, bounds.getHeight());
    }
    else
    {
        rect.setBounds(width, 0, bounds.getWidth() / 2 - width, bounds.getHeight());
    }

    // Fill the rectangle with the highlight color
    g.setColour(HIGHLIGHT_COLOR);
    g.fillRect(rect);

    // Draw a rounded rectangle border around the bounds if required
    if (drawBorder)
    {
        g.setColour(BACKGROUND_COLOR);
        g.drawRoundedRectangle(bounds.toFloat(), 3, 1);
    }
}
