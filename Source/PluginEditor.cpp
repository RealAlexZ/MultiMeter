/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.
 This project is created using JUCE version 6.1.2.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Implementation for the GonioGlanceAudioProcessorEditor class.
MultiMeterAudioProcessorEditor::MultiMeterAudioProcessorEditor (MultiMeterAudioProcessor& p) :
AudioProcessorEditor (&p),
audioProcessor (p),
goniometer(buffer),
correlationMeter(buffer, audioProcessor.getSampleRate()),
spectrumAnalyzer(audioProcessor),

decayRateSlider(*audioProcessor.apvts.getParameter("Decay Rate"), "dB/s"),
averageDurationSlider(*audioProcessor.apvts.getParameter("Average Duration"), "ms"),
meterViewSlider(*audioProcessor.apvts.getParameter("Meter View"), ""),
scaleKnobSlider(*audioProcessor.apvts.getParameter("Scale Knob"), "%"),
enableHoldSlier(*audioProcessor.apvts.getParameter("Enable Hold"), ""),
holdDurationSlider(*audioProcessor.apvts.getParameter("Hold Duration"), "s"),
histogramViewSlider(*audioProcessor.apvts.getParameter("Histogram View"), ""),

decayRateSliderAttachment(audioProcessor.apvts, "Decay Rate", decayRateSlider),
averageDurationSliderAttachment(audioProcessor.apvts, "Average Duration", averageDurationSlider),
meterViewSliderAttachment(audioProcessor.apvts, "Meter View", meterViewSlider),
scaleKnobSliderAttachment(audioProcessor.apvts, "Scale Knob", scaleKnobSlider),
enableHoldSlierAttachment(audioProcessor.apvts, "Enable Hold", enableHoldSlier),
holdDurationSliderAttachment(audioProcessor.apvts, "Hold Duration", holdDurationSlider),
histogramViewSliderAttachment(audioProcessor.apvts, "Histogram View", histogramViewSlider)

{
    startTimerHz(60);
    buffer.clear();

    addAndMakeVisible(peakMeter);
    addAndMakeVisible(RMSMeter);
    
    addAndMakeVisible(peakHistogram);
    addAndMakeVisible(RMSHistogram);
    
    addAndMakeVisible(goniometer);
    
    addAndMakeVisible(correlationMeter);
    
    addAndMakeVisible(correlationMeterLeft);
    addAndMakeVisible(correlationMeterRight);
    correlationMeterLeft.setText("-1", juce::NotificationType::dontSendNotification);
    correlationMeterRight.setText("+1", juce::NotificationType::dontSendNotification);
    correlationMeterLeft.attachToComponent(&correlationMeter, true);
    correlationMeterRight.attachToComponent(&correlationMeter, true);
    
    addAndMakeVisible(spectrumAnalyzer);
    
    for(auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }
    
    scaleKnobSlider.addListener(this);
    float scaleKnobValue = audioProcessor.pluginSettings.getProperty("ScaleKnob", 100);
    scaleKnobSlider.setValue(scaleKnobValue, juce::sendNotification);

    addAndMakeVisible(scaleKnobLabel);
    scaleKnobLabel.setText("Goniometer Scale", juce::NotificationType::dontSendNotification);
    scaleKnobLabel.attachToComponent(&scaleKnobSlider, true);


    
    addAndMakeVisible(decayRateSelector);
    decayRateSelector.addItemList(juce::StringArray("-3dB/s", "-6dB/s", "-12dB/s", "-24dB/s", "-36dB/s"), 1);
    decayRateSelector.addListener(this);
    int decayTimeID = audioProcessor.pluginSettings.getProperty("DecayTime", 1);
    decayRateSelector.setSelectedId(decayTimeID, juce::sendNotification);

    addAndMakeVisible(decayRateLabel);
    decayRateLabel.setText("Decay Rate", juce::NotificationType::dontSendNotification);
    decayRateLabel.attachToComponent(&decayRateSelector,true);


    
    addAndMakeVisible(averageDurationSelector);
    averageDurationSelector.addItemList(juce::StringArray("100ms", "250ms", "500ms", "1000ms", "2000ms"), 1);
    averageDurationSelector.addListener(this);
    int averageDurationID = audioProcessor.pluginSettings.getProperty("AveragerDuration", 1);
    averageDurationSelector.setSelectedId(averageDurationID, juce::sendNotification);

    addAndMakeVisible(averageDurationLabel);
    averageDurationLabel.setText("Averager Duration", juce::NotificationType::dontSendNotification);
    averageDurationLabel.attachToComponent(&averageDurationSelector, true);

    
    
    addAndMakeVisible(meterView);
    meterView.addItemList(juce::StringArray("Both", "Peak", "Avg"), 1);
    meterView.addListener(this);
    int meterViewID = audioProcessor.pluginSettings.getProperty("MeterView", 1);
    meterView.setSelectedId(meterViewID,juce::sendNotification);

    addAndMakeVisible(meterViewLabel);
    meterViewLabel.setText("Meter View", juce::NotificationType::dontSendNotification);
    meterViewLabel.attachToComponent(&meterView, true);



    addAndMakeVisible(showTick);
    showTick.addListener(this);
    int showTickID = audioProcessor.pluginSettings.getProperty("ShowTick", 0);
    showTick.setToggleState(showTickID, juce::sendNotification);

    addAndMakeVisible(showTickLabel);
    showTickLabel.setText("Show Tick", juce::NotificationType::dontSendNotification);
    showTickLabel.attachToComponent(&showTick, true);


    
    addAndMakeVisible(resetHold);
    resetHold.setVisible(false);
    addAndMakeVisible(resetHoldLabel);
    resetHoldLabel.setText("Reset Hold", juce::NotificationType::sendNotification);
    resetHoldLabel.attachToComponent(&resetHold, true);


    
    addAndMakeVisible(holdTimeSelector);
    holdTimeSelector.addItemList(juce::StringArray("0s","0.5s","2s","4s","6s","inf"), 1);
    holdTimeSelector.addListener(this);
    int holdTimeID = audioProcessor.pluginSettings.getProperty("HoldTime", 3);
    holdTimeSelector.setSelectedId(holdTimeID, juce::sendNotification);

    addAndMakeVisible(holdTimeLabel);
    holdTimeLabel.setText("Tick Hold Time", juce::NotificationType::dontSendNotification);
    holdTimeLabel.attachToComponent(&holdTimeSelector, true);

    

    addAndMakeVisible(histogramViewSelector);
    histogramViewSelector.addItemList(juce::StringArray("Stacked", "Side-by-Side"), 1);
    histogramViewSelector.addListener(this);
    int histogramViewID = audioProcessor.pluginSettings.getProperty("HistogramView", 1);
    histogramViewSelector.setSelectedId(histogramViewID,juce::sendNotification);

    addAndMakeVisible(histogramViewLabel);
    histogramViewLabel.setText("Histogram View", juce::NotificationType::dontSendNotification);
    histogramViewLabel.attachToComponent(&histogramViewSelector, true);

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (700, 700);
}

MultiMeterAudioProcessorEditor::~MultiMeterAudioProcessorEditor()
{
}

void MultiMeterAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (BACKGROUND_COLOR);
}

void MultiMeterAudioProcessorEditor::resized()
{
    auto height = getHeight();
    auto width = getWidth();
    const int gonioMeterWidth = 300;

    peakSBS = juce::Rectangle<int>{5, getWidth() * 2 / 3 + 5, (getWidth() - 10) / 2, getHeight() / 3 - 10};
    rmsSBS = juce::Rectangle<int>{getWidth() / 2 + 15, getWidth() * 2 / 3 + 5, (getWidth() - 10) / 2 - 15, getHeight() / 3 - 10};

    peakStacked = juce::Rectangle<int>{5, getWidth() * 2 / 3 + 5, getWidth() - 10, getHeight() / 6 - 10};
    rmsStacked = juce::Rectangle<int>{5, getWidth() * 5 / 6 + 5,getWidth() - 10, getHeight() / 6 - 10};

    auto comboboxHeight = 25;
    auto comboboxWidth = 90;
    auto labelHeight = 25;
    auto labelWidth = 100;

    decayRateSelector.setBounds(130, 15, comboboxWidth, comboboxHeight);
    decayRateLabel.setBounds(130, 15 + comboboxHeight, labelWidth, labelHeight);

    averageDurationSelector.setBounds(475, 230, comboboxWidth, comboboxHeight);
    averageDurationLabel.setBounds(475, 230 + comboboxHeight, labelWidth, labelHeight);

    meterView.setBounds(475, 15, comboboxWidth, comboboxHeight);
    meterViewLabel.setBounds(475, 15 + comboboxHeight, labelWidth, labelHeight);

    holdTimeSelector.setBounds(475, 70, comboboxWidth, comboboxHeight);
    holdTimeLabel.setBounds(475, 70 + comboboxHeight, labelWidth, labelHeight);

    scaleKnobSlider.setBounds(125, 190, 90, 90);
    scaleKnobLabel.setBounds(125, 230 + comboboxHeight, labelWidth, labelHeight);
    
    showTick.setBounds(130, 125, 25, 25);
    showTickLabel.setBounds(130, 120 + comboboxHeight, labelWidth, labelHeight);
    
    resetHold.setBounds(550, 125, 25, 25);
    resetHoldLabel.setBounds(500, 120 + comboboxHeight, labelWidth, labelHeight);

    histogramViewSelector.setBounds(130, 70, comboboxWidth, comboboxHeight);
    histogramViewLabel.setBounds(130, 70 + comboboxHeight, labelWidth, labelHeight);

    peakMeter.setSize(120, height * 2 / 3);
    peakMeter.setTopLeftPosition(5, 10);
    RMSMeter.setSize(120, height * 2 / 3);
    RMSMeter.setTopRightPosition(getWidth() - 5, 10);
    
    peakHistogram.setBounds(isHistogramStacked ? peakStacked : peakSBS);
    RMSHistogram.setBounds(isHistogramStacked? rmsStacked : rmsSBS);
    goniometer.setBounds(width / 2 - gonioMeterWidth / 2, 0, gonioMeterWidth, gonioMeterWidth);
    
    correlationMeter.setBounds(goniometer.getX(), goniometer.getBottom() - 15, gonioMeterWidth, 15);
    
    
    correlationMeterLeft.setBounds(correlationMeter.getX() - 24, correlationMeter.getY(), 24, correlationMeter.getHeight());
    correlationMeterRight.setBounds(correlationMeter.getRight(), correlationMeter.getY(), 24, correlationMeter.getHeight());
    
    spectrumAnalyzer.setBounds(peakMeter.getRight() + 10,
                               goniometer.getBottom() + 10,
                               RMSMeter.getX() - peakMeter.getRight() - 20,
                               peakHistogram.getY() - goniometer.getBottom() - 15);
}


void MultiMeterAudioProcessorEditor::timerCallback()
{

    auto& audioProcessorFifo = audioProcessor.fifo;

    if (/*JUCE_WINDOWS ||*/ audioProcessorFifo.getNumAvailableForReading() > 0)
    {
        while (audioProcessorFifo.pull(buffer))
        {
        }
    }

    float leftChannelMagnitudeRaw = buffer.getMagnitude(0, 0, buffer.getNumSamples());
    float rightChannelMagnitudeRaw = buffer.getMagnitude(1, 0, buffer.getNumSamples());
    
    float leftChannelRMSRaw = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
    float rightChannelRMSRaw = buffer.getRMSLevel(1, 0, buffer.getNumSamples());

    float leftChannelMagnitudeDecibels = juce::Decibels::gainToDecibels(leftChannelMagnitudeRaw,
                                                                        NEGATIVE_INFINITY);
    float rightChannelMagnitudeDecibels = juce::Decibels::gainToDecibels(rightChannelMagnitudeRaw,
                                                                         NEGATIVE_INFINITY);
    
    float leftChannelRMSDecibels = juce::Decibels::gainToDecibels(leftChannelRMSRaw,
                                                                  NEGATIVE_INFINITY);
    float rightChannelRMSDecibels = juce::Decibels::gainToDecibels(rightChannelRMSRaw,
                                                                  NEGATIVE_INFINITY);
    
    peakMeter.update(leftChannelMagnitudeDecibels,
                     rightChannelMagnitudeDecibels,
                     currentDecayRate,
                     meterViewID,
                     showTick.getToggleState(),
                     holdTime,
                     resetHold.getToggleState());
    RMSMeter.update(leftChannelRMSDecibels,
                    rightChannelRMSDecibels,
                    currentDecayRate,
                    meterViewID,
                    showTick.getToggleState(),
                    holdTime,
                    resetHold.getToggleState());
    
    if(resetHold.getToggleState())
    {
        resetHold.setToggleState(false, juce::dontSendNotification);
    }

    peakHistogram.update((leftChannelMagnitudeDecibels + rightChannelMagnitudeDecibels) / 2);
    RMSHistogram.update((leftChannelRMSDecibels + rightChannelRMSDecibels) / 2);

    correlationMeter.update(averageDuration);

    float gain = scaleKnobSlider.getValue() / 100;

    goniometer.updateCoeff(gain);

    goniometer.repaint();
}

void MultiMeterAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox == &decayRateSelector)
    {
        currentDecayRate = - comboBox->getItemText(comboBox->getSelectedId() - 1).removeCharacters("dB/s").getFloatValue();
        int selectedId = comboBox->getSelectedId();
        audioProcessor.pluginSettings.setProperty("DecayTime", selectedId, nullptr);
    }
    else if (comboBox == &averageDurationSelector)
    {
        averageDuration = comboBox->getItemText(comboBox->getSelectedId() - 1).removeCharacters("ms").getFloatValue();
        int selectedId = comboBox->getSelectedId();
        audioProcessor.pluginSettings.setProperty("AveragerDuration", selectedId, nullptr);
    }
    else if (comboBox == &meterView)
    {
        meterViewID = comboBox->getSelectedId() - 1;
        int selectedId = comboBox->getSelectedId();
        audioProcessor.pluginSettings.setProperty("MeterView", selectedId, nullptr);
    }
    else if (comboBox == &holdTimeSelector)
    {
        if (comboBox->getNumItems() == comboBox->getSelectedId())
        {
            holdTime = 60;
            resetHold.setVisible(true);
        }
        else
        {
            holdTime = comboBox->getItemText(comboBox->getSelectedId() - 1).removeCharacters("s").getFloatValue();
            resetHold.setVisible(false);
        }
        holdTime *= 1000;
        int selectedId = comboBox->getSelectedId();
        audioProcessor.pluginSettings.setProperty("HoldTime", selectedId, nullptr);
    }
    else if (comboBox == &histogramViewSelector)
    {
        if (comboBox->getSelectedId() == 1)
        {
            isHistogramStacked = true;
        }
        else
        {
            isHistogramStacked = false;
        }
        resized();
        repaint();
        int selectedId = comboBox->getSelectedId();
        audioProcessor.pluginSettings.setProperty("HistogramView", selectedId, nullptr);
    }
}

void MultiMeterAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    // We'll use this callback function to store the state of showTick button
    if (button == &showTick)
    {
        int showTickId = button->getToggleState();
        audioProcessor.pluginSettings.setProperty("ShowTick", showTickId, nullptr);
    }

}

void MultiMeterAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    // This callback is dedicated to the scaleKnobSlider to store the value of slider in valueTree
    if (slider == &scaleKnobSlider)
    {
        float sliderValue = scaleKnobSlider.getValue();
        audioProcessor.pluginSettings.setProperty("ScaleKnob", sliderValue, nullptr);
    }

}


//==============================================================================
// Implementation for the Meter class.
void Meter::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::white);

    float peakDbMapping = juce::jmap(peakDb,
                                     NEGATIVE_INFINITY,
                                     MAX_DECIBELS,
                                     static_cast<float>(getHeight()),
                                     0.f);
    
    g.fillRect(0.f,
               peakDbMapping,
               static_cast<float>(getWidth()),
               static_cast<float>(getHeight()) - peakDbMapping);
    
    float decayValueMapping = juce::jmap(decayingValueHolder.getCurrentValue(),
                                         NEGATIVE_INFINITY,
                                         MAX_DECIBELS,
                                         static_cast<float>(getHeight()),
                                         0.f);
    
    juce::Colour color = decayingValueHolder.isOverThreshold() ? juce::Colours::red : juce::Colours::grey;
    g.setColour(color);
    
    auto r = getLocalBounds().toFloat();
    r.setHeight(5.f);
    
    if(decayingValueHolder.getHoldTime() != 0)
    {
        r.setY(decayValueMapping);
    }
    else
    {
        r.setY(peakDbMapping);
    }

    if (showTick)
    {
        g.fillRect(r);
    }
}

void Meter::update(float dbLevel, float decayRate, float holdTime, bool resetHold, bool showTick_)
{
    peakDb = dbLevel;
    showTick = showTick_;
    decayingValueHolder.updateHeldValue(dbLevel);
    decayingValueHolder.setDecayRate(decayRate);
    decayingValueHolder.setHoldTime(holdTime);

    if(resetHold)
    {
        decayingValueHolder.setCurrentValue(NEGATIVE_INFINITY);
    }
    
    repaint();
}

//==============================================================================
// Implementation for the DbScale class.
void DbScale::paint(juce::Graphics& g)
{
    g.drawImage(bkgd, getLocalBounds().toFloat());
}

void DbScale::buildBackgroundImage(int dbDivision, juce::Rectangle<int> meterBounds, int minDb, int maxDb)
{
    if(minDb < maxDb)
    {
        std::swap(minDb, maxDb);
    }
    
    juce::Rectangle<int> bounds = getLocalBounds();
    
    if (bounds.isEmpty())
    {
        return;
    }
    
    float scaleFactor = juce::Desktop::getInstance().getGlobalScaleFactor();
    bkgd = juce::Image(juce::Image::ARGB,
                       static_cast<int>(bounds.getWidth()),
                       static_cast<int>(bounds.getHeight()),
                       true);
    
    juce::Graphics context(bkgd);
    
    context.addTransform(juce::AffineTransform().scaled(scaleFactor));

    context.setColour(juce::Colours::white);
    auto ticks = getTicks(dbDivision, meterBounds, minDb, maxDb);
    
    for (auto tick : ticks)
    {
        context.setFont(13);
        context.drawFittedText(tick.db >= 0 ?
                               ("+" + std::to_string(static_cast<int>(tick.db)) + ".00") :
                               ("" + std::to_string(static_cast<int>(tick.db)) + ".00"),
                               0,
                               tick.y - 0.5,
                               40,
                               1,
                               juce::Justification::centred,
                               1);
    }
}

std::vector<Tick> DbScale::getTicks(int dbDivision, juce::Rectangle<int> meterBounds, int minDb, int maxDb)
{
    if (minDb > maxDb)
    {
        std::swap(minDb, maxDb);
    }
    
    std::vector<Tick> ticks;
    ticks.reserve((maxDb - minDb) / dbDivision);
    
    for (int db = minDb; db <= maxDb; db += dbDivision)
    {
        ticks.push_back({float(db), juce::jmap(db,
                                               minDb,
                                               maxDb,
                                               meterBounds.getY() + meterBounds.getHeight(),
                                               meterBounds.getY())});
    }
    return ticks;
}

//==============================================================================
// Implementation for the ValueHolder class.
ValueHolder::ValueHolder() : timeOfPeak(juce::Time::currentTimeMillis())
{
    startTimerHz(60);
}

ValueHolder::~ValueHolder()
{
    stopTimer();
}

void ValueHolder::timerCallback()
{
    juce::int64 now = juce::Time::currentTimeMillis();
    juce::int64 elapsed = now - timeOfPeak;
    if (elapsed > durationToHoldForMs)
    {
        isOverThreshold = (currentValue > threshold);
        if (!isOverThreshold)
        {
            heldValue = NEGATIVE_INFINITY;
        }
    }
}

void ValueHolder::setThreshold(float th)
{
    threshold = th;
    isOverThreshold = (currentValue > threshold);
}

void ValueHolder::updateHeldValue(float v)
{
    if (v > threshold)
    {
        isOverThreshold = true;
        timeOfPeak = juce::Time::currentTimeMillis();
        if (v > heldValue)
        {
            heldValue = v;
        }
    }
    currentValue = v;
}

void ValueHolder::setHoldTime(int ms)
{
    durationToHoldForMs = ms;
}

float ValueHolder::getCurrentValue() const
{
    return currentValue;
}

float ValueHolder::getHeldValue() const
{
    return heldValue;
}

bool ValueHolder::getIsOverThreshold() const
{
    return isOverThreshold;
}

//==============================================================================
// Implementation for the TextMeter class.
TextMeter::TextMeter()
{
    valueHolder.setThreshold(0.f);
    valueHolder.updateHeldValue(NEGATIVE_INFINITY);
}

void TextMeter::paint(juce::Graphics& g)
{
    juce::Colour textColor;
    float valueToDisplay;
    
    if (valueHolder.getIsOverThreshold())
    {
        g.fillAll(juce::Colours::red);
        textColor = juce::Colours::black;
        valueToDisplay = valueHolder.getHeldValue();
    }
    else
    {
        g.fillAll(juce::Colours::black);
        textColor = juce::Colours::white;
        valueToDisplay = valueHolder.getCurrentValue();
    }
    g.setColour(textColor);
    g.setFont(12.f);
    
    g.drawFittedText(valueToDisplay > NEGATIVE_INFINITY ?
                     juce::String(valueToDisplay, 1).trimEnd() : "-inf",
                     getLocalBounds(),
                     juce::Justification::centredBottom,
                     1);
}

void TextMeter::update(float valueDb)
{
    cachedValueDb = valueDb;
    valueHolder.updateHeldValue(valueDb);
    repaint();
}

//==============================================================================
// Implementation for the DecayingValueHolder class.
DecayingValueHolder::DecayingValueHolder()
{
    setDecayRate(3.f);
    startTimerHz(60);
}

void DecayingValueHolder::updateHeldValue(float input)
{
    if (input > currentValue)
    {
        peakTime = getNow();
        currentValue = input;
        resetDecayRateMultiplier();
    }
}

float DecayingValueHolder::getCurrentValue() const
{
    return currentValue;
}

bool DecayingValueHolder::isOverThreshold() const
{
    return currentValue > threshold;
};

void DecayingValueHolder::setHoldTime(int ms)
{
    holdTime = ms;
};

void DecayingValueHolder::setDecayRate(float dbPerSec)
{
    decayRatePerFrame = dbPerSec / 60.f;
};

void DecayingValueHolder::setCurrentValue(float val)
{
    currentValue = val;
}

void DecayingValueHolder::timerCallback()
{
    juce::int64 now = getNow();
    
    if (now - peakTime > holdTime)
    {
        currentValue = currentValue - decayRatePerFrame * decayRateMultiplier;
        
        currentValue = juce::jlimit(NEGATIVE_INFINITY,
                     MAX_DECIBELS,
                     currentValue);
        
        decayRateMultiplier = decayRateMultiplier * 1.05;
        
        if (currentValue <= NEGATIVE_INFINITY)
        {
            resetDecayRateMultiplier();
        }
    }
}

juce::int64 DecayingValueHolder::getHoldTime()
{
    return holdTime;
}

void DecayingValueHolder::resetDecayRateMultiplier()
{
    decayRateMultiplier = 1;
};

//==============================================================================
// Implementations for the MacroMeter class.
MacroMeter::MacroMeter() : averager(60, NEGATIVE_INFINITY)
{
    addAndMakeVisible(textMeter);
    addAndMakeVisible(instantMeter);
    addAndMakeVisible(averageMeter);
}

void MacroMeter::resized()
{
    auto r = getLocalBounds();
    textMeter.setBounds(r.removeFromTop(14));
    instantMeter.setVisible(showPeak);
    averageMeter.setVisible(showAvg);
    if (showPeak && showAvg)
    {
        instantMeter.setBounds(r.removeFromLeft(r.getWidth() * 3 / 4));
        averageMeter.setBounds(r.withTrimmedLeft(r.getWidth() / 4));
    }
    else
    {
        if(showPeak)
        {
            instantMeter.setBounds(r);
        }
        else
        {
            averageMeter.setBounds(r);
        }
    }

}

void MacroMeter::update(float level, float decayRate, bool showPeak_, bool showAvg_, float holdTime, bool resetHold, bool showTick)
{
    textMeter.update(level);
    instantMeter.update(level, decayRate, holdTime, resetHold, showTick);
    averageMeter.update(averager.getAvg(), decayRate, holdTime, resetHold, showTick);
    averager.add(level);
    showPeak = showPeak_;
    showAvg = showAvg_;
    resized();
}

//==============================================================================
// Implementation for the StereoMeter class.
StereoMeter::StereoMeter(juce::String nameInput) : labelText(nameInput)
{
    addAndMakeVisible(leftMeter);
    addAndMakeVisible(rightMeter);
    addAndMakeVisible(dbScale);
    addAndMakeVisible(label);
}

void StereoMeter::paint(juce::Graphics &g)
{
    g.setColour(juce::Colours::white);
    g.drawText("    L", labelTextArea, juce::Justification::centredLeft);
    g.drawText(labelText, labelTextArea, juce::Justification::centred);
    g.drawText("R    ", labelTextArea, juce::Justification::centredRight);
}

void StereoMeter::resized()
{
    auto bounds = getLocalBounds();
    labelTextArea = bounds.removeFromBottom(30);
    label.setBounds(labelTextArea);

    auto leftMeterArea = bounds.removeFromLeft(bounds.getWidth() / 3);
    leftMeter.setBounds(leftMeterArea.withTrimmedBottom(4));
    
    auto rightMeterArea = bounds.removeFromRight(bounds.getWidth() / 2);
    rightMeter.setBounds(rightMeterArea.withTrimmedBottom(4));
    
    dbScale.setBounds(bounds);
    bounds = bounds.withTrimmedBottom(5);
    dbScale.buildBackgroundImage(6, bounds.withTrimmedTop(13), NEGATIVE_INFINITY, MAX_DECIBELS);
}

void StereoMeter::update(float leftChanDb, float rightChanDb, float decayRate, int meterViewID, bool showTick, float holdTime, bool resetHold)
{
    bool showPeak = !meterViewID || meterViewID == 1;
    bool showAvg = !meterViewID || meterViewID == 2;
    leftMeter.update(leftChanDb, decayRate, showPeak, showAvg, holdTime, resetHold, showTick);
    rightMeter.update(rightChanDb, decayRate, showPeak, showAvg, holdTime, resetHold, showTick);
    resized();
    repaint();
}

void StereoMeter::setText(juce::String labelName)
{
    label.setText(labelName, juce::dontSendNotification);
}

//==============================================================================
// Implementation for the Histogram class.
Histogram::Histogram(const juce::String& titleInput) : title(titleInput) {}

void Histogram::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::black);
    g.fillAll();
    
    drawTextLabels(g);
    
    g.setColour(juce::Colours::white);
    g.setFont(16.f);
    g.drawText(title, getLocalBounds().removeFromBottom(20), juce::Justification::centred);
    
    displayPath(g, getLocalBounds().toFloat());
}

void Histogram::resized()
{
    buffer.resize(getWidth(), NEGATIVE_INFINITY);
}

void Histogram::mouseDown(const juce::MouseEvent& e)
{
    buffer.clear(NEGATIVE_INFINITY);
}

void Histogram::update(float value)
{
    buffer.write(value);
    repaint();
}

void Histogram::displayPath(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    juce::Path fill = buildPath(path, buffer, bounds);
    if (!fill.isEmpty())
    {
        g.setColour(juce::Colours::grey.withAlpha(0.4f));
        g.fillPath(fill);
        
        g.setColour(juce::Colours::white);
        g.strokePath(fill, juce::PathStrokeType(1));
    }
}

void Histogram::drawTextLabels(juce::Graphics& g)
{
    using namespace juce;
    g.setColour(Colours::white);
    const int fontHeight = 10;
    g.setFont(fontHeight);
    
    auto bounds = getLocalBounds();
    auto left = bounds.getX();
    auto right = bounds.getRight();
    auto top = bounds.getY();
    auto bottom = bounds.getBottom();

    std::vector<float> ys {12.f, 0.f, -12.f, -24.f, -36.f, -48.f, -60.f};
    std::vector<String> ss {"+12", " +0", "-12", "-24", "-36", "-48", "-60"};
    for(int i = 0; i < 7; ++i)
    {
        auto yValue = jmap(ys[i], -66.f, 12.f, float(bottom), float(top));
        g.setColour(Colours::white);
        g.drawText(ss[i], left, int(yValue), 40, 10, Justification::left);
        g.drawText(ss[i], right - 40, int(yValue), 40, 10, Justification::right);
    }
}

//==============================================================================
// Implementation for the Goniometer class.
Goniometer::Goniometer(juce::AudioBuffer<float>& bufferInput) : buffer(bufferInput)
{
    internalBuffer.setSize(2, bufferInput.getNumSamples(), false, true, true);
    internalBuffer.clear();
    scale = 1;
}

void Goniometer::paint(juce::Graphics& g)
{
    drawBackground(g);
    p.clear();
    
    for (int channel = 0; channel < 2; ++channel)
    {
        internalBuffer.copyFrom(channel, 0, buffer, channel, 0, internalBuffer.getNumSamples());
    }
    
    int internalBufferSize = internalBuffer.getNumSamples();
    
    if (internalBufferSize < 256)
    {
        internalBuffer.applyGain(juce::Decibels::decibelsToGain(-3.f));
    }
    else
    {
        float coefficient = juce::Decibels::decibelsToGain(0.f+juce::Decibels::gainToDecibels(scale));
        float maxGain = juce::Decibels::decibelsToGain(MAX_DECIBELS);
        float minGain = juce::Decibels::decibelsToGain(NEGATIVE_INFINITY);

        for (int i = 0; i < internalBufferSize; ++i)
        {
            float leftRaw = internalBuffer.getSample(0, i);
            float rightRaw = internalBuffer.getSample(1, i);
            float S = (leftRaw - rightRaw) * coefficient;
            float M = (leftRaw + rightRaw) * coefficient;
            
            auto a = (float)getLocalBounds().getX() + getWidth()/2;
            auto b = (float)getLocalBounds().getRight() + getWidth()/2 - 40;
            auto c = (float)getLocalBounds().getBottom()- getHeight()/2 - 40;
            auto d = (float)getLocalBounds().getY() - getHeight()/2;

            float xCoordinate = juce::jmap(S, minGain, maxGain, (float)a, (float)b);
            float yCoordinate = juce::jmap(M, minGain, maxGain, (float)c, (float)d);

            juce::Point<float> point{xCoordinate, yCoordinate};
            
            if (i == 0)
            {
                p.startNewSubPath(point);
            }
            
            if (point.isFinite())
            {
                p.lineTo(point);
            }
        }
    }
    
    if (!p.isEmpty())
    {
        p.applyTransform(juce::AffineTransform::verticalFlip(h));
        juce::ColourGradient gradientColor(juce::Colours::green, center.x, center.y,
                                              juce::Colours::red, w / 2, h / 2, true);
        g.setGradientFill(gradientColor);
        g.strokePath(p, juce::PathStrokeType(1));
    }
}

void Goniometer::resized()
{
    center = juce::Point<int>(getWidth() / 2, getHeight() / 2);
    w = getWidth() - 40;
    h = getHeight() - 40;
}

void Goniometer::drawBackground(juce::Graphics& g)
{
    g.setColour(juce::Colours::white);
    g.drawEllipse(center.getX() - w / 2, center.getY() - h / 2, w, h, 1);
    g.setColour(juce::Colours::black);
    g.fillEllipse(center.getX() - w / 2, center.getY() - h / 2, w, h);

    for (int i = 0; i < 8; ++i)
    {
        juce::Point<float> endPoint = center.getPointOnCircumference(130, i * juce::MathConstants<double>::pi / 4 + juce::MathConstants<double>::pi / 2);
        g.setColour(juce::Colours::grey);
        g.drawLine(juce::Line<float>(center.toFloat(), endPoint.toFloat()), 1);
        if (endPoint.getY() <= center.getY())
        {
            int additionalDistanceY = 0;
            int additionalDistanceX = 0;
            if (i == 6)
            {
                additionalDistanceY = -15;
            }
            else if (i == 5 || i == 7)
            {
                additionalDistanceX = (i == 5 ? -12 : 12);
                additionalDistanceY = -15;
            }
            else if (i == 4 || i == 0)
            {
                additionalDistanceX = (i == 4 ? -10 : 10);
                additionalDistanceY = -7;
            }
            g.setColour(juce::Colours::white);
            g.drawText(chars[i == 0 ? i : i - 3 ],
                       endPoint.getX() - 10 + additionalDistanceX,
                       endPoint.getY() + additionalDistanceY,
                       20,
                       10,
                       juce::Justification::centredTop);
        }
    }
}

void Goniometer::update(juce::AudioBuffer<float> &bufferInput)
{
    bufferInput.clear();
    buffer = bufferInput;
}

void Goniometer::updateCoeff(float newDb)
{
    scale = newDb;
}

//==============================================================================
// Implementation for the CorrelationMeter class.
CorrelationMeter::CorrelationMeter(juce::AudioBuffer<float>& buf, double sampleRate) : buffer(buf)
{
    for(auto& filter : filters)
    {
        auto coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 20000.f);
        filter.coefficients = coefficients;
    }
}

void CorrelationMeter::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colours::black);
    auto slowBounds = getLocalBounds().removeFromTop(getLocalBounds().getHeight() / 3);
    
    drawAverage(g, slowBounds, peakAverager.getAvg(), false);
    drawAverage(g, getLocalBounds(), slowAverager.getAvg(), true);
}

void CorrelationMeter::update(juce::int64 averageTime)
{
    auto numSamples = buffer.getNumSamples();

    peakAverager.setAverageDuration(averageTime);
    slowAverager.setAverageDuration(averageTime);

    for (int i = 0; i < numSamples; i++)
    {
        auto left = buffer.getSample(0, i);
        auto right = buffer.getSample(1, i);
        auto numerator = filters[0].processSample(left * right);
        auto denominator = sqrt(filters[1].processSample(left * left) * filters[2].processSample(right * right));
        
        if (std::isnan(numerator) || std::isinf(numerator) ||
            std::isnan(denominator) || std::isinf(denominator) || denominator == 0.0f)
        {
            peakAverager.add(0.f);
            slowAverager.add(0.f);
        }
        else
        {
            auto correlation = numerator / denominator;
            peakAverager.add(correlation);
            slowAverager.add(correlation);
        }
    }
    repaint();
}

void CorrelationMeter::drawAverage(juce::Graphics &g, juce::Rectangle<int> bounds, float avg, bool drawBorder)
{
    int width = juce::jmap(avg, -1.0f, 1.0f, 0.f, (float)bounds.getWidth());
    
    juce::Rectangle<int> rect;
    if (avg >= 0)
    {
        rect.setBounds(bounds.getWidth() / 2,
                       0,
                       width - bounds.getWidth() / 2,
                       bounds.getHeight());
    } else
    {
        rect.setBounds(width,
                       0,
                       bounds.getWidth() / 2 - width, bounds.getHeight());
    }

    g.setColour(juce::Colours::white);
    g.fillRect(rect);
    g.setColour(juce::Colours::grey);
    g.drawRect(bounds);
}

//==============================================================================
// Implementation for the ResponseCurveComponent class.
ResponseCurveComponent::ResponseCurveComponent(MultiMeterAudioProcessor& p) : audioProcessor (p),
leftPathProducer(audioProcessor.leftChannelFifo),
rightPathProducer(audioProcessor.rightChannelFifo)
{
    startTimerHz(60);
}

void ResponseCurveComponent::paint (juce::Graphics& g)
{
    using namespace juce;
    g.fillAll (juce::Colours::black);
    drawBackgroundGrid(g);

    auto responseArea = getAnalysisArea();

    auto leftChannelFFTPath = leftPathProducer.getPath();
    leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
    g.setColour(juce::Colours::green);
    g.strokePath(leftChannelFFTPath, PathStrokeType(1.f));
    
    auto rightChannelFFTPath = rightPathProducer.getPath();
    rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
    g.setColour(juce::Colours::red);
    g.strokePath(rightChannelFFTPath, PathStrokeType(1.f));
    
    Path border;
    border.setUsingNonZeroWinding(false);
    border.addRoundedRectangle(getRenderArea(), 4);
    border.addRectangle(getLocalBounds());
    
    g.setColour(BACKGROUND_COLOR);
    g.fillPath(border);
    drawTextLabels(g);

    g.setColour(Colours::grey);
    g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);
    
}

std::vector<float> ResponseCurveComponent::getFrequencies()
{
    return std::vector<float>{20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};
}

std::vector<float> ResponseCurveComponent::getGains()
{
    return std::vector<float>{-24, -12, 0, 12, 24};
}

std::vector<float> ResponseCurveComponent::getXs(const std::vector<float> &freqs, float left, float width)
{
    std::vector<float> xs;
    
    for(auto f : freqs)
    {
        auto normX = juce::mapFromLog10(f, 20.f, 20000.f);
        xs.push_back(left + width * normX);
    }
    
    return xs;
}

void ResponseCurveComponent::drawBackgroundGrid(juce::Graphics &g)
{
    using namespace juce;
    auto freqs = getFrequencies();

    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();
    
    auto xs = getXs(freqs, left, width);
    
    g.setColour(Colours::dimgrey);
    
    for(auto x : xs)
    {
        g.drawVerticalLine(x, top, bottom);
    }

    auto gain = getGains();
    
    for(auto gDb : gain)
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        g.setColour(gDb == 0.f ? Colours::lightgrey : Colours::darkgrey);
        g.drawHorizontalLine(y, left, right);
    }
}

void ResponseCurveComponent::drawTextLabels(juce::Graphics &g)
{
    using namespace juce;
    g.setColour(Colours::white);
    const int fontHeight = 10;
    g.setFont(fontHeight);
    
    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();
    
    auto freqs = getFrequencies();
    auto xs = getXs(freqs, left, width);

    for(int i = 0; i < freqs.size(); ++i)
    {
        auto f = freqs[i];
        auto x = xs[i];

        bool addK = false;
        String str;
        if(f > 999.f)
        {
            addK = true;
            f /= 1000.f;
        }

        str << f;
        if(addK)
        {
            str << "k";
        }
        str << "Hz";

        auto textWidth = g.getCurrentFont().getStringWidth(str);

        Rectangle<int> r;

        r.setSize(textWidth, fontHeight);
        r.setCentre(x, 0);
        r.setY(1);

        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }
    
    auto gain = getGains();

    for(auto gDb : gain)
    {

        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));

        String str;
        if(gDb > 0)
        {
            str << "+";
        }
        str << gDb;

        auto textWidth = g.getCurrentFont().getStringWidth(str);

        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(getWidth() - textWidth);
        r.setCentre(r.getCentreX(), y);

        g.setColour(Colours::white);

        g.drawFittedText(str, r, juce::Justification::centredLeft, 1);
        
        str.clear();
        str << (gDb - 24.f);

        r.setX(1);
        textWidth = g.getCurrentFont().getStringWidth(str);
        r.setSize(textWidth, fontHeight);
        g.drawFittedText(str, r, juce::Justification::centredLeft, 1);
    }
}

void ResponseCurveComponent::timerCallback()
{
    auto fftBounds = getAnalysisArea().toFloat();
    auto sampleRate = audioProcessor.getSampleRate();
        
    leftPathProducer.process(fftBounds, sampleRate);
    rightPathProducer.process(fftBounds, sampleRate);
    repaint();
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(12);
    bounds.removeFromBottom(2);
    bounds.removeFromLeft(20);
    bounds.removeFromRight(20);

    return bounds;
}

juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea()
{
    auto bounds = getRenderArea();
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);
    return bounds;
}

//==============================================================================
// Implementation for the PathProducer class.
void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate)
{
    juce::AudioBuffer<float> tempIncomingBuffer;
    while(leftChannelFifo->getNumCompleteBuffersAvailable() > 0)
    {
        if(leftChannelFifo->getAudioBuffer(tempIncomingBuffer))
        {
            auto size = tempIncomingBuffer.getNumSamples();

            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0),
                                              monoBuffer.getReadPointer(0, size),
                                              monoBuffer.getNumSamples() - size);

            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
                                              tempIncomingBuffer.getReadPointer(0, 0),
                                              size);
            
            leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);
        }
    }
    
    const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();
    const auto binWidth = sampleRate / double(fftSize);

    while(leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0)
    {
        std::vector<float> fftData;
        
        if(leftChannelFFTDataGenerator.getFFTData(fftData))
        {
            pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -48.f);
        }
    }
    
    while(pathProducer.getNumPathsAvailable() > 0)
    {
        pathProducer.getPath(leftChannelFFTPath);
    }
}

juce::Path PathProducer::getPath()
{
    return leftChannelFFTPath;
}

//==============================================================================
// Implementation for the getComps() function.
std::vector<juce::Component*> MultiMeterAudioProcessorEditor::getComps()
{
    return
    {
        &decayRateSlider,
        &averageDurationSlider,
        &meterViewSlider,
        &scaleKnobSlider,
        &enableHoldSlier,
        &holdDurationSlider,
        &histogramViewSlider,
    };
}

//==============================================================================
// Implementation for the myLookAndFeel class.
void myLookAndFeel::drawRotarySlider(juce::Graphics &g,
                                     int x,
                                     int y,
                                     int width,
                                     int height,
                                     float sliderPosProportional,
                                     float rotaryStartAngle,
                                     float rotaryEndAngle,
                                     juce::Slider &slider)
{
    using namespace juce;
    auto bounds = Rectangle<float>(x, y, width, height);
    auto enabled = slider.isEnabled();
    
    g.setColour(enabled ? Colours::black : Colours::darkgrey);
    g.fillEllipse(bounds);

    g.setColour(enabled ? Colours::white : Colours::grey);
    g.drawEllipse(bounds, 2.f);

    if(auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        auto center = bounds.getCentre();
        Path p;
        Rectangle<float> r;
        
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY() + 2);
        r.setBottom(center.getY() - rswl->getTextHeight()*1.5);

        p.addRoundedRectangle(r, 2.f);

        auto sliderAngRad = jmap(sliderPosProportional,
                                 0.f,
                                 1.f,
                                 rotaryStartAngle,
                                 rotaryEndAngle);

        p.applyTransform(AffineTransform().rotated(sliderAngRad,
                                                   center.getX(),
                                                   center.getY()));
        g.fillPath(p);
        
        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);
        r.setSize(strWidth, rswl->getTextHeight());
        r.setCentre(bounds.getCentre());

        g.setColour(enabled ? Colours::black : Colours::darkgrey);
        g.fillRect(r);
        g.setColour(enabled ? Colours::white : Colours::lightgrey);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

//==============================================================================
// Implementation for the RotarySliderWithLabels class.
void RotarySliderWithLabels::paint(juce::Graphics &g)
{
    using namespace juce;
    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;
    auto range = getRange();
    auto sliderBounds = getSliderBounds();

    getLookAndFeel().drawRotarySlider(g,
                                      sliderBounds.getX(),
                                      sliderBounds.getY(),
                                      sliderBounds.getWidth(),
                                      sliderBounds.getHeight(),
                                      jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
                                      startAng,
                                      endAng,
                                      *this);
    
    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;

    g.setColour(Colours::white);
    g.setFont(getTextHeight());
    
    auto numChoices = labels.size();
    for(int i = 0; i < numChoices; ++i)
    {
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.f);
        auto ang = jmap(labels[i].pos, 0.f, 1.f, startAng, endAng);

        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, ang);
        auto str = labels[i].label;
        Rectangle<float> r;
        r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight());

        g.drawFittedText(str, r.toNearestInt(), Justification::centred, 1);
    }
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    auto bounds = getLocalBounds();
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    size -= getTextHeight() * 2;

    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2);

    return r;
}

juce::String RotarySliderWithLabels::getDisplayString() const
{
    if(auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
    {
        return choiceParam->getCurrentChoiceName();
    }

    juce::String str;
    bool addK = false;

    if(auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
    {
        float val = getValue();
        if(val > 999.f)
        {
            val /= 1000.f;
            addK = true;
        }
        str = juce::String(val, (addK ? 2 : 0));
    }
    else
    {
        jassertfalse;
    }

    if(suffix.isNotEmpty())
    {
        str << " ";
        if(addK)
            str << "k";
        str << suffix;
    }
    
    return str;
}
