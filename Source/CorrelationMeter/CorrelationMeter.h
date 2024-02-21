
#pragma once
#include "../GonioMeter/Goniometer.h"
using namespace juce;

//==============================================================================
struct CorrelationMeter : juce::Component
{
    // Constructor to initialize CorrelationMeter with an audio buffer and sample rate
    CorrelationMeter(juce::AudioBuffer<float>& buf, double sampleRate);

    // Override of the paint function to handle the drawing of the correlation meter
    void paint(juce::Graphics& g) override;

    // Function to update the correlation meter based on the average time
    void update(juce::int64 average_time);

private:
    // Reference to the audio buffer
    juce::AudioBuffer<float>& buffer;

    // Type alias for the filter used in correlation meter
    using FilterType = juce::dsp::IIR::Filter<float>;

    // Array to hold the filters used in the correlation meter
    std::array<FilterType, 3> filters;

    // Averager objects for slow and peak averaging
    Averager<float> slowAverager{1024 * 3, 0}, peakAverager{512, 0};

    // Function to draw the average on the correlation meter
    void drawAverage(juce::Graphics& g,
        juce::Rectangle<int> bounds,
        float avg,
        bool drawBorder);
};

