
#include "LevelMeter.h"


//==============================================================================
// Implementation for the Meter class
void Meter::paint(juce::Graphics& g)
{

    // Fill this rectangle with a different color than the color filling the component
    g.setColour(BASE_COLOR);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4);
    g.fillRect(getLocalBounds().removeFromTop(5));
    g.setColour(HIGHLIGHT_COLOR);

    // Maps peakDb to be between NEGATIVE_INFINITY and MAX_DECIBELS
    // NEGATIVE_INFINITY corresponds to the BOTTOM of the component
    // MAX_DECIBELS corresponds to the TOP of the component
    // y = 0.f is at the TOP of the component, and vice versa
    float peakDbMapping = juce::jmap(peakDb, // sourceValue
        NEGATIVE_INFINITY, // sourceRangeMin
        MAX_DECIBELS, // sourceRangeMax
        static_cast<float>(getHeight()), // targetRangeMin
        0.f); // targetRangeMax

    // Draws the rectangle
    g.fillRoundedRectangle(0.f, // x
        peakDbMapping, // y
        static_cast<float>(getWidth()), // width
        static_cast<float>(getHeight()) - peakDbMapping, 2); // height

    float decayValueMapping = juce::jmap(decayingValueHolder.getCurrentValue(), // sourceValue
        NEGATIVE_INFINITY, // sourceRangeMin
        MAX_DECIBELS, // sourceRangeMax
        static_cast<float>(getHeight()), // targetRangeMin
        0.f); // targetRangeMax

    juce::Colour color = decayingValueHolder.isOverThreshold() ?
        juce::Colours::red : juce::Colours::grey;
    g.setColour(color);

    auto r = getLocalBounds().toFloat();
    r.setHeight(5.f);

    if (decayingValueHolder.getHoldTime() != 0)
    {
        r.setY(decayValueMapping);
    }
    else
    {
        r.setY(peakDbMapping);
    }

    if (show_tick)
        g.fillRect(r);

}

void Meter::update(float dbLevel, float decay_rate, float hold_time_, bool reset_hold, bool show_tick_)
{
    // Pass in a decibel value and store it in peakDb
    peakDb = dbLevel;
    show_tick = show_tick_;

    // Here the setLevelMeterDecay and setHoldTime will actually set the decay_rate and holdTime variable to the meter
    decayingValueHolder.updateHeldValue(dbLevel);
    // Because the decay rate could change anytime so we will pass the decay rate as argument from meter update function
    decayingValueHolder.setLevelMeterDecay(decay_rate);
    decayingValueHolder.setHoldTime(hold_time_);

    // Call repaint()
    if (reset_hold)
        decayingValueHolder.setCurrentValue(NEGATIVE_INFINITY);
    repaint();
}

//==============================================================================
// Implementation for the DbScale class
void DbScale::paint(juce::Graphics& g)
{
    g.drawImage(bkgd, getLocalBounds().toFloat());
}

void DbScale::buildBackgroundImage(int dbDivision, juce::Rectangle<int> meterBounds, int minDb, int maxDb)
{
    if (minDb < maxDb)
    {
        std::swap(minDb, maxDb);
    }

    juce::Rectangle<int> bounds = getLocalBounds();

    if (bounds.isEmpty())
    {
        return;
    }

    float scaleFactor = juce::Desktop::getInstance().getGlobalScaleFactor();
    bkgd = juce::Image(juce::Image::ARGB, // format
        static_cast<int>(bounds.getWidth()), // imageWidth
        static_cast<int>(bounds.getHeight()), // imageHeight
        true); // clearImage

    // Create a Graphics context for bkgd
    juce::Graphics context(bkgd);

    context.addTransform(juce::AffineTransform().scaled(scaleFactor));
    // Add a transform to it that accounts for the global scale factor

    context.setColour(juce::Colours::darkgrey);
    auto ticks = getTicks(dbDivision, meterBounds, minDb, maxDb);

    for (auto tick : ticks)
    {
        context.setFont(13);
        context.drawFittedText(tick.db >= 0 ?
            ("+" + std::to_string(static_cast<int>(tick.db))) :
            ("" + std::to_string(static_cast<int>(tick.db))), // text
            0, // x
            tick.y - 0.5, // y
            40, // width
            1, // height
            juce::Justification::centred, // justification
            1); // maximum number of lines
    }
}

std::vector<Tick> DbScale::getTicks(int dbDivision, juce::Rectangle<int> meterBounds, int minDb, int maxDb)
{
    // Make sure minDb is less than maxDb. std::swap() them if they are not
    if (minDb > maxDb)
    {
        std::swap(minDb, maxDb);
    }

    // Create the vector
    // Reserve the correct number of elements needed
    std::vector<Tick> ticks;
    ticks.reserve((maxDb - minDb) / dbDivision);

    // Now loop, starting at db = minDb; db <= maxDb; db += dbDivision
    for (int db = minDb; db <= maxDb; db += dbDivision)
    {
        // Declare a Tick, set the db, and use the same jmap trick from the Meter to map the tick.db to
        // some y value within the meterBounds

        // Add the tick to the vector
        // ticks.push_back(tick);
        ticks.push_back({ float(db), juce::jmap(db, // sourceValue
                                               minDb, // sourceRangeMin
                                               maxDb, // sourceRangeMax
                                               meterBounds.getY() + meterBounds.getHeight(), // targetRangeMin
                                               meterBounds.getY()) }); // targetRangeMax
    }

    // When done making Tick's, return the vector
    return ticks;
}



//==============================================================================
// Implementation for the ValueHolder class
ValueHolder::ValueHolder() : timeOfPeak(juce::Time::currentTimeMillis())
{
    startTimerHz(60); // Start the timer with a frequency of 60 Hz
}

ValueHolder::~ValueHolder()
{
    stopTimer(); // Stop the timer
}

void ValueHolder::timerCallback()
{
    // Calculate the elapsed time since the last peak
    juce::int64 now = juce::Time::currentTimeMillis();
    juce::int64 elapsed = now - timeOfPeak;

    // If the elapsed time exceeds the hold duration, update the held value
    if (elapsed > durationToHoldForMs)
    {
        // Check if the current value is over the threshold
        isOverThreshold = (currentValue > threshold);

        // Reset the held value if the current value is not over the threshold
        if (!isOverThreshold)
        {
            heldValue = NEGATIVE_INFINITY;
        }
    }
}

void ValueHolder::setThreshold(float th)
{
    threshold = th; // Set the threshold value
    isOverThreshold = (currentValue > threshold); // Update the over threshold flag
}

void ValueHolder::updateHeldValue(float v)
{
    // Update the held value if the new value is over the threshold
    if (v > threshold)
    {
        isOverThreshold = true; // Set the over threshold flag
        timeOfPeak = juce::Time::currentTimeMillis(); // Update the time of peak
        if (v > heldValue)
        {
            heldValue = v; // Update the held value
        }
    }
    currentValue = v; // Update the current value
}

void ValueHolder::setHoldTime(int ms)
{
    durationToHoldForMs = ms; // Set the duration to hold for
}

float ValueHolder::getCurrentValue() const
{
    return currentValue; // Return the current value
}

float ValueHolder::getHeldValue() const
{
    return heldValue; // Return the held value
}

bool ValueHolder::getIsOverThreshold() const
{
    return isOverThreshold; // Return whether the current value is over the threshold
}

//==============================================================================
// Implementations for the MacroMeter class
MacroMeter::MacroMeter() : averager(60, NEGATIVE_INFINITY)
{
    // Add and make visible the child components: textMeter, instantMeter, and averageMeter
    addAndMakeVisible(textMeter);
    addAndMakeVisible(instantMeter);
    addAndMakeVisible(averageMeter);
}

void MacroMeter::resized()
{
    auto r = getLocalBounds();

    // Set the bounds for the textMeter component at the top
    textMeter.setBounds(r.removeFromTop(14));

    // Set the visibility and bounds for the instantMeter and averageMeter components based on show_peak_ and show_avg_ flags
    instantMeter.setVisible(show_peak_);
    averageMeter.setVisible(show_avg_);

    if (show_peak_ && show_avg_)
    {
        // If both show_peak_ and show_avg_ are true, divide the available width into 3/4 for instantMeter and 1/4 for averageMeter
        instantMeter.setBounds(r.removeFromLeft(r.getWidth() * 3 / 4));
        averageMeter.setBounds(r.withTrimmedLeft(r.getWidth() / 4));
    }
    else
    {
        // If only one of show_peak_ or show_avg_ is true, set the bounds accordingly
        if (show_peak_)
            instantMeter.setBounds(r);
        else
            averageMeter.setBounds(r);
    }
}

void MacroMeter::update(float level, float decay_rate, bool show_peak, bool show_avg, float hold_time_, bool reset_hold, bool show_tick_)
{
    // Update the child components with the provided parameters
    textMeter.update(level);
    instantMeter.update(level, decay_rate, hold_time_, reset_hold, show_tick_);
    averageMeter.update(averager.getAvg(), decay_rate, hold_time_, reset_hold, show_tick_);

    // Add the current level to the averager
    averager.add(level);

    // Update the show_peak_ and show_avg_ flags
    show_peak_ = show_peak;
    show_avg_ = show_avg;

    // Resize the components
    resized();
}

//==============================================================================
// Implementation for the StereoMeter class
StereoMeter::StereoMeter(juce::String nameInput) : labelText(nameInput)
{
    // Add and make visible the child components: leftMeter, rightMeter, dbScale, and label
    addAndMakeVisible(leftMeter);
    addAndMakeVisible(rightMeter);
    addAndMakeVisible(dbScale);
    addAndMakeVisible(label);
}

void StereoMeter::paint(juce::Graphics& g)
{
    // Draw the labels for left and right channels
    g.setColour(juce::Colours::darkgrey);
    g.drawText("    L", labelTextArea, juce::Justification::centredLeft);
    g.drawText(labelText, labelTextArea, juce::Justification::centred);
    g.drawText("R    ", labelTextArea, juce::Justification::centredRight);
}

void StereoMeter::resized()
{
    auto bounds = getLocalBounds();
    auto meterWidth = bounds.getWidth() / 3;

    // Set the bounds for the label
    labelTextArea = bounds.removeFromBottom(30);
    label.setBounds(labelTextArea);

    // Set the bounds for the leftMeter component
    auto leftMeterArea = bounds.removeFromLeft(meterWidth);
    leftMeter.setBounds(leftMeterArea);

    // Set the bounds for the rightMeter component
    auto rightMeterArea = bounds.removeFromRight(meterWidth);
    rightMeter.setBounds(rightMeterArea);

    // Set the bounds for the dbScale component
    dbScale.setBounds(bounds);
    bounds = bounds.withTrimmedBottom(5);
    dbScale.buildBackgroundImage(10, bounds.withTrimmedTop(13), NEGATIVE_INFINITY, MAX_DECIBELS);
}

void StereoMeter::update(float leftChanDb, float rightChanDb, float decay_rate, int meterViewID, bool show_tick, float hold_time_, bool reset_hold)
{
    // Determine whether to show peak and average based on meterViewID
    bool show_peak = !meterViewID || meterViewID == 1;
    bool show_avg = !meterViewID || meterViewID == 2;

    // Update the leftMeter and rightMeter components with the provided parameters
    leftMeter.update(leftChanDb, decay_rate, show_peak, show_avg, hold_time_, reset_hold, show_tick);
    rightMeter.update(rightChanDb, decay_rate, show_peak, show_avg, hold_time_, reset_hold, show_tick);

    // Resize the components and repaint
    resized();
    repaint();
}

void StereoMeter::setText(juce::String labelName)
{
    // Set the text for the label
    label.setText(labelName, juce::dontSendNotification);
}

//==============================================================================
// Implementation for the TextMeter class
TextMeter::TextMeter()
{
    // Set the threshold for the value holder to 0 and initialize the held value to negative infinity
    valueHolder.setThreshold(0.f);
    valueHolder.updateHeldValue(NEGATIVE_INFINITY);
}

void TextMeter::paint(juce::Graphics& g)
{
    juce::Colour textColor;
    float valueToDisplay;

    // Determine the color and value to display based on whether the current value is over the threshold
    if (valueHolder.getIsOverThreshold())
    {
        // If over threshold, set color to red and display the held value
        g.setColour(juce::Colours::red);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4);
        g.fillRect(getLocalBounds().removeFromBottom(5));
        textColor = juce::Colours::black;
        valueToDisplay = valueHolder.getHeldValue();
    }
    else
    {
        // If not over threshold, set color to base color and display the current value
        g.setColour(BASE_COLOR);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4);
        g.fillRect(getLocalBounds().removeFromBottom(5));
        textColor = HIGHLIGHT_COLOR;
        valueToDisplay = valueHolder.getCurrentValue();
    }
    g.setColour(textColor);
    g.setFont(12.f);

    // Draw the text with appropriate formatting
    g.drawFittedText(valueToDisplay > NEGATIVE_INFINITY ?
        juce::String(valueToDisplay, 1).trimEnd() : "-inf", // text
        getLocalBounds(), // bounds
        juce::Justification::centredBottom, // justification
        1); // number of lines
}

void TextMeter::update(float valueDb)
{
    // Update the cached value, held value, and repaint
    cachedValueDb = valueDb;
    valueHolder.updateHeldValue(valueDb);
    repaint();
}

//==============================================================================
// Implementation for the DecayingValueHolder class
DecayingValueHolder::DecayingValueHolder()
{
    // Set the default decay rate to 3 dB per second and start the timer
    setLevelMeterDecay(3.f);
    startTimerHz(60);
}

void DecayingValueHolder::updateHeldValue(float input)
{
    // Update the held value and peak time if the input value is greater than the current value
    if (input > currentValue)
    {
        peakTime = getNow();
        currentValue = input;
        resetLevelMeterDecayMultiplier();
    }
}

void DecayingValueHolder::setCurrentValue(float val)
{
    // Set the current value to the specified value
    currentValue = val;
}

void DecayingValueHolder::timerCallback()
{
    juce::int64 now = getNow();

    // If the elapsed time exceeds the hold time, decay the current value
    if (now - peakTime > holdTime)
    {
        currentValue -= decayRatePerFrame * decayRateMultiplier;

        // Ensure the current value stays within the specified range
        currentValue = juce::jlimit(NEGATIVE_INFINITY, MAX_DECIBELS, currentValue);

        // Increase the decay rate multiplier
        decayRateMultiplier *= 1.05;

        // Reset the decay rate multiplier if the current value becomes negative infinity
        if (currentValue <= NEGATIVE_INFINITY)
        {
            resetLevelMeterDecayMultiplier();
        }
    }
}

juce::int64 DecayingValueHolder::getHoldTime()
{
    // Return the hold time
    return holdTime;
}
