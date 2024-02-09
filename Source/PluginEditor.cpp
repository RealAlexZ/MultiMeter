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

levelMeterDecaySlider(*audioProcessor.apvts.getParameter("Level Meter Decay"), "dB/s"),
averagerDurationSlider(*audioProcessor.apvts.getParameter("Averager Duration"), "ms"),
meterViewSlider(*audioProcessor.apvts.getParameter("Meter View"), ""),
scaleKnobSlider(*audioProcessor.apvts.getParameter("Scale Knob"), "%"),
enableHoldSlier(*audioProcessor.apvts.getParameter("Enable Hold"), ""),
holdDurationSlider(*audioProcessor.apvts.getParameter("Hold Duration"), "s"),
histogramViewSlider(*audioProcessor.apvts.getParameter("Histogram View"), ""),

levelMeterDecaySliderAttachment(audioProcessor.apvts, "Level Meter Decay", levelMeterDecaySlider),
averagerDurationSliderAttachment(audioProcessor.apvts, "Averager Duration", averagerDurationSlider),
meterViewSliderAttachment(audioProcessor.apvts, "Meter View", meterViewSlider),
scaleKnobSliderAttachment(audioProcessor.apvts, "Scale Knob", scaleKnobSlider),
enableHoldSlierAttachment(audioProcessor.apvts, "Enable Hold", enableHoldSlier),
holdDurationSliderAttachment(audioProcessor.apvts, "Hold Duration", holdDurationSlider),
histogramViewSliderAttachment(audioProcessor.apvts, "Histogram View", histogramViewSlider)

{
    startTimerHz(60);
    buffer.clear();

    addAndMakeVisible(histogramViewButton);
    histogramViewButton.addOption("Parallel", *this);
    histogramViewButton.addOption("Stacked", *this);

    int histoID = (audioProcessor.histogramDisplayID > 1 || audioProcessor.histogramDisplayID < 0) ? 0 : audioProcessor.histogramDisplayID;
    histogramViewButton.setSelection(histoID);

    addAndMakeVisible(peakMeter);
    addAndMakeVisible(RMSMeter);
    
    addChildComponent(peakHistogram);
    addChildComponent(RMSHistogram);
    
    addChildComponent(goniometer);
    
    addAndMakeVisible(correlationMeter);
    
    addAndMakeVisible(spectrumAnalyzer);

    for( auto* comp : getComps() )
    {
        addAndMakeVisible(comp);
    }

    addAndMakeVisible(visualSwitch);
    visualSwitch.addListener(*this);
    
/************************* SCALE KNOB ************************/

    // add listener to this slider so that whenever user drags sliderValueChanged will be called
    scaleKnobSlider.addListener(this);

    float validValue = (audioProcessor.sliderValue > 200 || audioProcessor.sliderValue < 50) ? 100 : audioProcessor.sliderValue;
    //use this slider to adjust the goniometer signal strength
    //we're in constructor of editor and we need to call the last saved value of scaleknob that was stored in pluginSettings valueTree
    scaleKnobSlider.setValue(validValue, juce::dontSendNotification);

    //attaching a label to the slider
    addAndMakeVisible(scaleKnobLabel);
    scaleKnobLabel.setText("Goniometer Scale", juce::NotificationType::dontSendNotification);
    scaleKnobLabel.setColour(Label::ColourIds::textColourId, Colours::black);


/*************************Level Meter Decay********************************/
        // Just defining the behaviour and contents of combobox in the constructor is editor class
    addAndMakeVisible(levelMeterDecaySelector);
    levelMeterDecaySelector.addItemList(juce::StringArray("-3dB/s", "-6dB/s", "-12dB/s", "-24dB/s", "-36dB/s"), 1);
    levelMeterDecaySelector.addListener(this);
    levelMeterDecaySelector.setColour(Label::ColourIds::textColourId, Colours::black);

    // Here we're getting the last selected ID from pluginSetting valueTree 
    // we'll set the last selected decay rate id in constructor,
    int validID = (audioProcessor.levelMeterDecayId > 5 || audioProcessor.levelMeterDecayId < 1) ? 1 : audioProcessor.levelMeterDecayId;
    levelMeterDecaySelector.setSelectedId(validID, juce::dontSendNotification);

    addAndMakeVisible(levelMeterDecayLabel);
    levelMeterDecayLabel.setText("Level Meter Decay", juce::NotificationType::dontSendNotification);
    levelMeterDecayLabel.setColour(Label::ColourIds::textColourId, Colours::black);


    /*************************************************/


    /********************** AVERAGER DURATION ************************/

    // for all controls the pattern is same ->makeitvisible->addItems->addListener->getLastStoredID->setSelectedID
    addAndMakeVisible(averagerDurationSelector);
    averagerDurationSelector.addItemList(juce::StringArray("100ms", "250ms", "500ms", "1000ms", "2000ms"), 1);
    averagerDurationSelector.addListener(this);

    validID = (audioProcessor.averagerDurationId > 5 || audioProcessor.averagerDurationId < 1) ? 1 : audioProcessor.averagerDurationId;
    averagerDurationSelector.setSelectedId(validID, juce::dontSendNotification);

    addAndMakeVisible(averagerDurationLabel);
    averagerDurationLabel.setText("Averager Duration", juce::NotificationType::dontSendNotification);
    averagerDurationLabel.setColour(Label::ColourIds::textColourId, Colours::black);

    /*************** METER VIEW **********************************/

    //do same with the meterveiw combobox
    addAndMakeVisible(meterViewButton);
    meterViewButton.addOption("Both", *this);
    meterViewButton.addOption("Peak", *this);
    meterViewButton.addOption("Avg", *this);

    int meterID = (audioProcessor.levelMeterDisplayID > 2 || audioProcessor.levelMeterDisplayID < 0) ? 0 : audioProcessor.levelMeterDisplayID;
    meterViewButton.setSelection(meterID);

    addAndMakeVisible(meterViewLabel);
    meterViewLabel.setText("Level Meter Display", juce::NotificationType::dontSendNotification);
    meterViewLabel.setColour(Label::ColourIds::textColourId, Colours::black);


    /****************TICK DISPLAY ******************/

    addAndMakeVisible(tickDisplay);
    tickDisplay.addListener(this);

    tickDisplay.setToggleState(true, juce::dontSendNotification);
    tickDisplay.clicked();

    addAndMakeVisible(tickDisplayLabel);
    tickDisplayLabel.setText("Tick Display", juce::NotificationType::dontSendNotification);
    tickDisplayLabel.setColour(Label::ColourIds::textColourId, Colours::black);

    /******************** HOLD TIME *********************/
    // hold time also follows same pattern for constructor
    addAndMakeVisible(holdTimeSelector);
    holdTimeSelector.addItemList(juce::StringArray("0s", "0.5s", "2s", "4s", "6s", "inf"), 1);
    holdTimeSelector.addListener(this);

    validID = (audioProcessor.holdTimeId > 6 || audioProcessor.holdTimeId < 1) ? 3 : audioProcessor.holdTimeId;
    holdTimeSelector.setSelectedId(validID, juce::dontSendNotification);

    addAndMakeVisible(holdTimeLabel);
    holdTimeLabel.setText("Tick Hold Duration", juce::NotificationType::dontSendNotification);
    holdTimeLabel.setColour(Label::ColourIds::textColourId, Colours::black);

    /*************** RESET HOLD ****************/
    addAndMakeVisible(resetHold);
    resetHold.clicked();

    // for reset hold we're not using valuetree because it has only on state (responds to user click and then go back to false state)
    resetHold.setVisible(holdTimeSelector.getSelectedId() == 6);

    /************************ HISTOGRAM VIEW ********************/

    addAndMakeVisible(histogramViewLabel);
    histogramViewLabel.setText("Histogram Display", juce::NotificationType::dontSendNotification);
    histogramViewLabel.setColour(Label::ColourIds::textColourId, Colours::black);

    addAndMakeVisible(correlationLabel0);
    correlationLabel0.setText("-1", juce::NotificationType::dontSendNotification);
    correlationLabel0.setColour(Label::ColourIds::textColourId, Colours::black);

    addAndMakeVisible(correlationLabel1);
    correlationLabel1.setText("0", juce::NotificationType::dontSendNotification);
    correlationLabel1.setColour(Label::ColourIds::textColourId, Colours::black);

    addAndMakeVisible(correlationLabel2);
    correlationLabel2.setText("+1", juce::NotificationType::dontSendNotification);
    correlationLabel2.setColour(Label::ColourIds::textColourId, Colours::black);

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (globalWidth, gloablHeight);

    setLookAndFeel(&lookandfeel);
}

MultiMeterAudioProcessorEditor::~MultiMeterAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void MultiMeterAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Fill the background with a solid color
    g.fillAll(BACKGROUND_COLOR);

}

void MultiMeterAudioProcessorEditor::resized()
{
    auto height = getHeight();
    auto width = getWidth();
    const int gonioMeterWidth = 285;

    visualsRoom = getBounds();
    visualsRoom.removeFromTop(20);
    meterRoom  = visualsRoom.removeFromRight(getWidth() / 3);
    controlRoom = visualsRoom.removeFromBottom(visualsRoom.getHeight() / 4);
    correlationRoom = meterRoom.removeFromBottom(meterRoom.getHeight() / 5);

    auto stackedSpace = visualsRoom.reduced(26,20);
    peakStacked = stackedSpace.removeFromTop(stackedSpace.getHeight() / 2).withTrimmedBottom(5);
    rmsStacked = stackedSpace.withTrimmedTop(5);

    auto sbsSpace = visualsRoom.reduced(26, 20);
    peakSBS = sbsSpace.removeFromLeft(sbsSpace.getWidth() / 2).withTrimmedRight(5);
    rmsSBS = sbsSpace.withTrimmedLeft(5);

    int SBSWidth = 230;
    int SBSHeight = 180;

    auto combobox_height = 25;
    auto combobox_width = 90;
    auto label_height = 25;
    auto label_width = 100;


    visualSwitch.setBounds(0, 0, getWidth()+5, 20);

    //// **********Visualizers
    spectrumAnalyzer.setBounds(visualsRoom.reduced(20));
    goniometer.setBounds(visualsRoom.getCentreX() - gonioMeterWidth / 2, visualsRoom.getCentreY() - gonioMeterWidth / 2, gonioMeterWidth, gonioMeterWidth);

    peakHistogram.setBounds(audioProcessor.histogramDisplayID ? peakStacked : peakSBS);
    RMSHistogram.setBounds(audioProcessor.histogramDisplayID ? rmsStacked : rmsSBS);

    auto peakSection = meterRoom.removeFromLeft(meterRoom.getWidth() / 2).reduced(10,0);
    peakMeter.setBounds(peakSection.expanded(0, 5).translated(0,25));
    RMSMeter.setBounds(meterRoom.reduced(10,0).expanded(0, 5).translated(0,25));

    correlationMeter.setBounds(correlationRoom.reduced(13,30).translated(0,2));

    int y = correlationMeter.getBottom();
    int wd = 24;
    int ht = 24;
    correlationLabel0.setBounds(correlationMeter.getX(),y, wd, ht);
    correlationLabel2.setBounds(correlationMeter.getRight()-wd, y, wd, ht);
    correlationLabel1.setBounds(correlationMeter.getX() + correlationMeter.getWidth()/2-12, y, wd, ht);

    /////********* Menu Controls

    auto delY = 22;

    auto tempspace = controlRoom.getWidth() / 5;
    auto knobSpace = controlRoom.removeFromLeft(tempspace);
    auto knobLabel = knobSpace.removeFromTop(delY);

    scaleKnobLabel.setBounds(knobLabel);
    scaleKnobSlider.setBounds(knobSpace.expanded(5).translated(0,5));

    auto levelLabel = controlRoom.removeFromLeft(120);
    auto levelSpace = controlRoom.removeFromLeft(159);

    meterViewLabel.setBounds(levelLabel.removeFromTop(delY));
    meterViewButton.setBounds(levelSpace.removeFromTop(delY));

    levelMeterDecayLabel.setBounds(levelLabel.removeFromTop(delY));
    levelMeterDecaySelector.setBounds(levelSpace.removeFromTop(delY).reduced(0,1));

    tickDisplayLabel.setBounds(levelLabel.removeFromTop(delY));
    tickDisplay.setBounds(levelSpace.removeFromTop(delY).reduced(0,2));

    holdTimeLabel.setBounds(levelLabel.removeFromTop(delY));
    auto tickSpace1 = levelSpace.removeFromTop(delY);
    auto tickSpace = tickSpace1.removeFromRight(80);
    holdTimeSelector.setBounds(tickSpace1.reduced(0, 1).withTrimmedRight(2));
    resetHold.setBounds(tickSpace.reduced(0,1).withTrimmedLeft(2));

    controlRoom.removeFromLeft(10);
    auto space1 = controlRoom.removeFromRight(130);
    histogramViewLabel.setBounds(space1.removeFromTop(delY).withTrimmedRight(10));
    histogramViewButton.setBounds(space1.removeFromTop(delY).reduced(0, 1).translated(5,0));

    averagerDurationLabel.setBounds(space1.removeFromTop(delY).withTrimmedRight(10));
    averagerDurationSelector.setBounds(space1.removeFromTop(delY).removeFromLeft(104).reduced(0, 1).translated(6,0));
}



// STEP 1.3.4:
// In timerCallback(), if the audioProcessor.fifo has items available for reading,
// use a while( fifo.pull(buffer) ) loop to pull every element available out of the fifo.
// Once finishing pulling every element out of the fifo, process the buffer that was pulled
// as the project requires. This part is fully implemented after the meter class is implemented
// and an instance is created.
void MultiMeterAudioProcessorEditor::timerCallback()
{

    auto& audioProcessorFifo = audioProcessor.fifo;

    // If the audioProcessor.fifo has items available for reading.
    if (/*JUCE_WINDOWS ||*/ audioProcessorFifo.getNumAvailableForReading() > 0)
    {
        // Use a while( fifo.pull(buffer) ) loop to pull every element available out of the fifo.
        while (audioProcessorFifo.pull(buffer))
        {
        }
    }
    
    // After finishing pulling all buffers out of the fifo in timerCallback,
    // use the buffer’s member function that returns the magnitude for a channel to get
    // the Left channel’s magnitude.
    // This function returns a "gain" value.f
    float leftChannelMagnitudeRaw = buffer.getMagnitude(0, 0, buffer.getNumSamples());
    float rightChannelMagnitudeRaw = buffer.getMagnitude(1, 0, buffer.getNumSamples());
    
    float leftChannelRMSRaw = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
    float rightChannelRMSRaw = buffer.getRMSLevel(1, 0, buffer.getNumSamples());

    // Convert this value to decibels.
    // The juce::Decibels::gainToDecibels() function takes a 2nd parameter.
    // This 2nd parameter lets you define what "negative infinity" is, which is NEGATIVE_INFINITY.
    float leftChannelMagnitudeDecibels = juce::Decibels::gainToDecibels(leftChannelMagnitudeRaw,
                                                                        NEGATIVE_INFINITY);
    float rightChannelMagnitudeDecibels = juce::Decibels::gainToDecibels(rightChannelMagnitudeRaw,
                                                                         NEGATIVE_INFINITY);
    
    float leftChannelRMSDecibels = juce::Decibels::gainToDecibels(leftChannelRMSRaw,
                                                                  NEGATIVE_INFINITY);
    float rightChannelRMSDecibels = juce::Decibels::gainToDecibels(rightChannelRMSRaw,
                                                                  NEGATIVE_INFINITY);
    

    // Almost all control values are passed with in this timercallback

    //all these variables are passed in update function so that when timecallback is routinely called, these updated variables will be passed to update function
    // these variables like levelMeterDecay, meterview, tickDisplay will be updated via corresponding litener functions like comboboxchanged, slidervaluechanged...
    peakMeter.update(leftChannelMagnitudeDecibels, rightChannelMagnitudeDecibels,current_decay_rate, audioProcessor.levelMeterDisplayID,tickDisplay.getToggleState(),hold_time,resetHold.getToggleState()); 
    RMSMeter.update(leftChannelRMSDecibels, rightChannelRMSDecibels, current_decay_rate, audioProcessor.levelMeterDisplayID,tickDisplay.getToggleState(),hold_time, resetHold.getToggleState());
    
    // we'll reset this button to false state so that it'll not go into toggle state.
    if(resetHold.getToggleState())
        resetHold.setToggleState(false, juce::dontSendNotification);

    peakHistogram.update((leftChannelMagnitudeDecibels + rightChannelMagnitudeDecibels) / 2);
    RMSHistogram.update((leftChannelRMSDecibels + rightChannelRMSDecibels) / 2);

    // correlation averager will update with the new averager duration everytime the timercallback is called.
    correlationMeter.update(averager_duration);

    // scale knob values are already mapped to 50 - 200 so we'll use this value as a gain 0.5 - 2.0 passed to the goniometer update function
    float gain = scaleKnobSlider.getValue() / 100;

    goniometer.updateCoeff(gain); //to scale the goniometer plot

    goniometer.repaint();
}

void MultiMeterAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBox)
{
    // THis is where all the combobox control parameter value will be changed. 
    // based on which combo box has changed we can modify the variables that'll be updated in timercallback function
    if (comboBox == &levelMeterDecaySelector)
    {
        // we're parsing the actual float value for the decay rate
        // use of the - to assure the negative decay value will be passed as positive floating point value 
        current_decay_rate = - comboBox->getItemText(comboBox->getSelectedId() - 1).removeCharacters("dB/s").getFloatValue(); 

        //This is used to store the selected ID to the value tree so that the next plugin will parse the value with getProperty in editor constructor
        audioProcessor.levelMeterDecayId = comboBox->getSelectedId();
    }

    // Every combobox calls to update the variable and then store the selected id in value tree
    else if (comboBox == &averagerDurationSelector)
    {
        averager_duration = comboBox->getItemText(comboBox->getSelectedId() - 1).removeCharacters("ms").getFloatValue(); //ms
        
        audioProcessor.averagerDurationId = comboBox->getSelectedId();
    }
    else if (comboBox == &holdTimeSelector)
    {

        if (comboBox->getNumItems() == comboBox->getSelectedId())
        {
            hold_time = 60; //let's assume 60 sec for inf hold time
            resetHold.setVisible(true);
        }

        else
        {
            hold_time = comboBox->getItemText(comboBox->getSelectedId() - 1).removeCharacters("s").getFloatValue();
            resetHold.setVisible(false);
        }

        hold_time *= 1000;

        audioProcessor.holdTimeId = comboBox->getSelectedId();
    }    

}

void MultiMeterAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &tickDisplay)
    {
        audioProcessor.tickDisplayState = button->getToggleState();
    }
    else
    {
        visualSwitch.buttonClicked(button);
        histogramViewButton.buttonClicked(button);
        meterViewButton.buttonClicked(button);

        int id = visualSwitch.getSwitchID();

        spectrumAnalyzer.setVisible(id == 1);
        peakHistogram.setVisible(id == 2);
        RMSHistogram.setVisible(id == 2);
        goniometer.setVisible(id == 0);

        audioProcessor.levelMeterDisplayID = meterViewButton.getSelectedId();
        audioProcessor.histogramDisplayID = histogramViewButton.getSelectedId();
    }

    resized();
    repaint();
}

void MultiMeterAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    // This callback is dedicated to the scaleKnobSlider to store the value of slider in valueTree
    if (slider == &scaleKnobSlider)
    {
        audioProcessor.sliderValue = slider->getValue();
    }
}


//==============================================================================
// Implementation for the Meter class.
// STEP 1.4.2:
// The job of paint() is to draw a rectangle that has the following properties:
// The bottom is always equal to the bottom of the component;
// The width is always as wide as the component;
// The x value is 0;
// The y value is peakDb mapped to a position within the component.
void Meter::paint(juce::Graphics& g)
{

    // Fill this rectangle with a different color than the color filling the component.
    
    g.setColour(meterBgColour);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4);
    g.fillRect(getLocalBounds().removeFromTop(5));
    g.setColour(meterLevelColour);
        
    // Meter::paint(juce::Graphics& g) maps peakDb to be between NEGATIVE_INFINITY and MAX_DECIBELS.
    // NEGATIVE_INFINITY corresponds to the BOTTOM of the component.
    // MAX_DECIBELS corresponds to the TOP of the component.
    // y = 0.f is at the TOP of the component, and vice versa.
    float peakDbMapping = juce::jmap(peakDb, // sourceValue
                                    NEGATIVE_INFINITY, // sourceRangeMin
                                    MAX_DECIBELS, // sourceRangeMax
                                     static_cast<float>(getHeight()), // targetRangeMin
                                    0.f); // targetRangeMax
    
    // Draws the rectangle.
    g.fillRoundedRectangle(0.f, // x
               peakDbMapping, // y
               static_cast<float>(getWidth()), // width
               static_cast<float>(getHeight()) - peakDbMapping,2); // height
    
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
    
    if(decayingValueHolder.getHoldTime() != 0)
    {
        r.setY(decayValueMapping);
    }
    else
    {
        r.setY(peakDbMapping);
    }

    if(show_tick)
        g.fillRect(r);
    
}

void Meter::update(float dbLevel, float decay_rate, float hold_time_, bool reset_hold, bool show_tick_)
{
    // Pass in a decibel value and store it in peakDb.
    peakDb = dbLevel;
    show_tick = show_tick_;

    // here the setLevelMeterDecay and setHoldTime will actually set the decay_rate and hold_time variable to the meter.
    decayingValueHolder.updateHeldValue(dbLevel);
    decayingValueHolder.setLevelMeterDecay(decay_rate); // because the decay rate could change anytime so we'll pass the decay rate as argument from meter update function
    decayingValueHolder.setHoldTime(hold_time_);

    // Call repaint().
    if(reset_hold)
        decayingValueHolder.setCurrentValue(NEGATIVE_INFINITY);
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
    bkgd = juce::Image(juce::Image::ARGB, // format
                       static_cast<int>(bounds.getWidth()), // imageWidth
                       static_cast<int>(bounds.getHeight()), // imageHeight
                       true); // clearImage
    
    // Create a Graphics context for bkgd.
    juce::Graphics context(bkgd);
    
    context.addTransform(juce::AffineTransform().scaled(scaleFactor));
    // Add a transform to it that accounts for the global scale factor.
    
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
    // Make sure minDb is less than maxDb. std::swap() them if they aren’t.
    if (minDb > maxDb)
    {
        std::swap(minDb, maxDb);
    }
    
    // Create the vector.
    // Reserve the correct number of elements needed.
    std::vector<Tick> ticks;
    ticks.reserve((maxDb - minDb) / dbDivision);
    
    // Now loop, starting at db = minDb; db <= maxDb; db += dbDivision.
    for (int db = minDb; db <= maxDb; db += dbDivision)
    {
        // Declare a Tick, set the db, and use the same jmap trick from the Meter to map the tick.db to
        // some y value within the meterBounds.
        
        // Add the tick to the vector.
        // ticks.push_back(tick);
        ticks.push_back({float(db), juce::jmap(db, // sourceValue
                                               minDb, // sourceRangeMin
                                               maxDb, // sourceRangeMax
                                               meterBounds.getY() + meterBounds.getHeight(), // targetRangeMin
                                               meterBounds.getY())}); // targetRangeMax
    }
    
    // When done making Tick's, return the vector.
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
        g.setColour(juce::Colours::red);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4);
        g.fillRect(getLocalBounds().removeFromBottom(5));
        textColor = juce::Colours::black;
        valueToDisplay = valueHolder.getHeldValue();
    }
    else
    {
        g.setColour(meterBgColour);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4);
        g.fillRect(getLocalBounds().removeFromBottom(5));
        textColor = meterLevelColour;
        valueToDisplay = valueHolder.getCurrentValue();
    }
    g.setColour(textColor);
    g.setFont(12.f);
    
    g.drawFittedText(valueToDisplay > NEGATIVE_INFINITY ?
                     juce::String(valueToDisplay, 1).trimEnd() : "-inf", // text
                     getLocalBounds(), // bounds
                     juce::Justification::centredBottom, // justification
                     1); // number of lines
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
    setLevelMeterDecay(3.f);
    startTimerHz(60);
}

void DecayingValueHolder::updateHeldValue(float input)
{
    if (input > currentValue)
    {
        peakTime = getNow();
        currentValue = input;
        resetLevelMeterDecayMultiplier();
    }
}

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
        
        currentValue = juce::jlimit(NEGATIVE_INFINITY, // lowerLimit
                     MAX_DECIBELS, // upperLimit
                     currentValue); // valueToConstrain
        
        decayRateMultiplier = decayRateMultiplier * 1.05;
        
        if (currentValue <= NEGATIVE_INFINITY)
        {
            resetLevelMeterDecayMultiplier();
        }
    }
}

juce::int64 DecayingValueHolder::getHoldTime()
{
    return holdTime;
}

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
    instantMeter.setVisible(show_peak_);
    averageMeter.setVisible(show_avg_);
    if (show_peak_ && show_avg_)
    {
        instantMeter.setBounds(r.removeFromLeft(r.getWidth() * 3 / 4));
        averageMeter.setBounds(r.withTrimmedLeft(r.getWidth() / 4));
    }
    else
    {
        if(show_peak_)
            instantMeter.setBounds(r);
        else
            averageMeter.setBounds(r);
    }

}

void MacroMeter::update(float level, float decay_rate, bool show_peak, bool show_avg, float hold_time_, bool reset_hold, bool show_tick_)
{
    // pass the control parameter values to the update function untill we reach to the actual implementation of the variable
    textMeter.update(level);
    instantMeter.update(level, decay_rate, hold_time_, reset_hold, show_tick_);
    averageMeter.update(averager.getAvg(), decay_rate, hold_time_, reset_hold, show_tick_);
    averager.add(level);
    show_peak_ = show_peak;
    show_avg_ = show_avg;
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
    g.setColour(juce::Colours::darkgrey);
    g.drawText("    L", labelTextArea, juce::Justification::centredLeft);
    g.drawText(labelText, labelTextArea, juce::Justification::centred);
    g.drawText("R    ", labelTextArea, juce::Justification::centredRight);
}

void StereoMeter::resized()
{
    auto bounds = getLocalBounds();
    auto meterWidth = bounds.getWidth() / 3;
    labelTextArea = bounds.removeFromBottom(30);
    label.setBounds(labelTextArea);

    auto leftMeterArea = bounds.removeFromLeft(meterWidth);
    leftMeter.setBounds(leftMeterArea);
    
    auto rightMeterArea = bounds.removeFromRight(meterWidth);
    rightMeter.setBounds(rightMeterArea);
    
    dbScale.setBounds(bounds);
    bounds = bounds.withTrimmedBottom(5);
    dbScale.buildBackgroundImage(10, bounds.withTrimmedTop(13), NEGATIVE_INFINITY, MAX_DECIBELS);
}

void StereoMeter::update(float leftChanDb, float rightChanDb, float decay_rate, int meterViewID, bool show_tick, float hold_time_, bool reset_hold)
{
    bool show_peak = !meterViewID || meterViewID == 1; //if meterViewID is zero both peak and both will set to true
    bool show_avg = !meterViewID || meterViewID == 2;  // otherwise OR operator let's one of the meter to set true

    // pass the control variable  has been updated from callback functions to the actual update function of the meter 
    leftMeter.update(leftChanDb, decay_rate, show_peak, show_avg,hold_time_, reset_hold, show_tick);
    rightMeter.update(rightChanDb, decay_rate, show_peak, show_avg,hold_time_, reset_hold,show_tick);
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
    g.setColour(BASE_COLOR);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4);
    
    g.setColour(juce::Colours::white);
    g.setFont(16.f);
    g.drawText(title, getLocalBounds().removeFromBottom(20), juce::Justification::centred);
    
    displayPath(g, getLocalBounds().toFloat());

    Path border;
    border.setUsingNonZeroWinding(false);
    border.addRectangle(getLocalBounds());
    auto bounds = getLocalBounds().toFloat().reduced(1);
    border.addRoundedRectangle(bounds, 4);
    g.setColour(BACKGROUND_COLOR);
    g.fillPath(border);
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
        ColourGradient gradient(HIGHLIGHT_COLOR.withAlpha(0.8f),
            bounds.getX(), bounds.getY(),
            BASE_COLOR.withAlpha(0.3f),
            bounds.getX(), bounds.getBottom(),
            false);

        juce::FillType fillType(gradient);
        g.setFillType(fillType);

        g.fillPath(fill);
        
        //g.setColour(HIGHLIGHT_COLOR);
        //g.strokePath(fill, juce::PathStrokeType(1));
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
    
    for (int channel = 0; channel < 2; ++channel) // Assuming stereo data (2 channels)
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
            
            //juce::Point<float> point{125 + xCoordinate, yCoordinate - 150};
            // TODO: Without adjusting the line above, like below, the path looks completely out of bounds.
            // TODO: Fix this.
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
        juce::ColourGradient gradientColor(pathColourInside, center.x, center.y,
                                              pathColourOutside, w / 2, h / 2, true);
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
    g.setColour(edgeColour);
    g.drawEllipse(center.getX() - w / 2, center.getY() - h / 2, w, h, 1);
    g.setColour(m_backgroundColour);
    g.fillEllipse(center.getX() - w / 2, center.getY() - h / 2, w, h);

    for (int i = 0; i < 8; ++i)
    {
        juce::Point<float> endPoint = center.getPointOnCircumference(122, i * juce::MathConstants<double>::pi / 4 + juce::MathConstants<double>::pi / 2);
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
            g.setColour(m_backgroundColour);
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

void Goniometer::updateCoeff(float new_db)
{
    scale = new_db;
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
    g.setColour(m_backgroundColour);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 3);
    auto slowBounds = getLocalBounds().removeFromTop(getLocalBounds().getHeight() / 3);
    
    drawAverage(g, slowBounds, peakAverager.getAvg(), false);
    drawAverage(g, getLocalBounds(), slowAverager.getAvg(), true);

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

    peakAverager.setAveragerDuration(average_time);
    slowAverager.setAveragerDuration(average_time);

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
    // avg = JUCE_LIVE_CONSTANT(0.0f);
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

    g.setColour(graphColour);
    g.fillRect(rect);
    g.setColour(BACKGROUND_COLOR);
    g.drawRoundedRectangle(bounds.toFloat(), 3,1);
}

//==============================================================================
// Implementation for the ResponseCurveComponent class.
ResponseCurveComponent::ResponseCurveComponent(MultiMeterAudioProcessor& p) : audioProcessor(p),
leftPathProducer(audioProcessor.leftChannelFifo),
rightPathProducer(audioProcessor.rightChannelFifo),
m_grid(p.apvts)
{
    startTimerHz(60);

    addAndMakeVisible(m_grid);
    m_grid.setGridColour(juce::Colour(0xff464646));
    m_grid.setTextColour(juce::Colour(0xff848484));
}

void ResponseCurveComponent::paint (juce::Graphics& g)
{
    
    using namespace juce;

     ////Fill a rounded rectangle with the background color
    g.setColour(m_backgroundColour);

    g.fillRect(getAnalysisArea());
  
}

void ResponseCurveComponent::paintOverChildren(Graphics& g)
{
    auto responseArea = getAnalysisArea();

    auto rightChannelFFTPath = rightPathProducer.getPath();
    rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));

    g.setColour(m_GraphColourRight);
    g.strokePath(rightChannelFFTPath, PathStrokeType(1.f));
    
    auto leftChannelFFTPath = leftPathProducer.getPath();
    leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));

    g.setColour(m_GraphColourLeft);
    g.strokePath(leftChannelFFTPath, PathStrokeType(1.f));

    Path border;

    border.setUsingNonZeroWinding(false);
    border.addRectangle(getAnalysisArea());
    auto bounds = getLocalBounds().toFloat();
    bounds.removeFromLeft(6);
    bounds.removeFromRight(6);
    
    border.addRoundedRectangle(bounds, 9);
    g.setColour(m_backgroundColour);
    g.fillPath(border);
}


std::vector<float> ResponseCurveComponent::getFrequencies()
{
    return std::vector<float>{
        20, 50, 100,
        200, 500, 1000,
        2000, 5000, 10000,
        20000
    };
}

std::vector<float> ResponseCurveComponent::getGains()
{
    return std::vector<float>{
        -24, -12, 0, 12, 24
    };
}

std::vector<float> ResponseCurveComponent::getXs(const std::vector<float> &freqs, float left, float width)
{
    std::vector<float> xs;
    for( auto f : freqs )
    {
        auto normX = juce::mapFromLog10(f, 20.f, 20000.f);
        xs.push_back( left + width * normX );
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
    
    g.setColour(juce::Colour(0xff464646));
    for( auto x : xs )
    {

        g.drawVerticalLine(x, top, bottom);
    }

    auto gain = getGains();
    
    
    for( auto gDb : gain )
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));

        g.setColour(juce::Colour(0xff464646));
        
        g.drawHorizontalLine(y, left, right);
    }
}

void ResponseCurveComponent::drawTextLabels(juce::Graphics &g)
{
    using namespace juce;
    g.setColour(Colours::black);
    const int fontHeight = 10;
    g.setFont(fontHeight);
    
    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();
    
    auto freqs = getFrequencies();
    auto xs = getXs(freqs, left, width);

    for( int i = 0; i < freqs.size(); ++i )
    {
        auto f = freqs[i];
        auto x = xs[i];

        bool addK = false;
        String str;
        if( f > 999.f )
        {
            addK = true;
            f /= 1000.f;
        }

        str << f;
        if( addK )
            str << "k";
        str << "Hz";

        auto textWidth = g.getCurrentFont().getStringWidth(str);

        Rectangle<int> r;

        r.setSize(textWidth, fontHeight);
        r.setCentre(x, 0);
        r.setY(1);

        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }
    
    auto gain = getGains();

    for( auto gDb : gain )
    {

        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));

        String str;
        if( gDb > 0 )
            str << "+";
        str << gDb;

        auto textWidth = g.getCurrentFont().getStringWidth(str);

        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(getWidth() - textWidth);
        r.setCentre(r.getCentreX(), y);

        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::lightgrey );

        g.drawFittedText(str, r, juce::Justification::centredLeft, 1);
        
        str.clear();
        str << (gDb - 24.f);

        r.setX(1);
        textWidth = g.getCurrentFont().getStringWidth(str);
        r.setSize(textWidth, fontHeight);
        g.setColour(Colours::lightgrey);
        g.drawFittedText(str, r, juce::Justification::centredLeft, 1);
    }
}

void ResponseCurveComponent::timerCallback()
{
    auto fftBounds = getAnalysisArea().toFloat();
    auto sampleRate = audioProcessor.getSampleRate();
        
    leftPathProducer.process(fftBounds, sampleRate);
    rightPathProducer.process(fftBounds, sampleRate);
    // Paint the generated path
    repaint();
}

void ResponseCurveComponent::resized()
{

    m_grid.setBounds(getAnalysisArea());
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(7);
    bounds.removeFromBottom(7);
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
    while( leftChannelFifo->getNumCompleteBuffersAvailable() > 0 )
    {
        if( leftChannelFifo->getAudioBuffer(tempIncomingBuffer) )
        {
            auto size = tempIncomingBuffer.getNumSamples();

            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0),
                                              monoBuffer.getReadPointer(0, size),
                                              monoBuffer.getNumSamples() - size);

            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
                                              tempIncomingBuffer.getReadPointer(0, 0),
                                              size);
            
            leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -120.f);
        }
    }
    
    const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();
    const auto binWidth = sampleRate / double(fftSize);

    while( leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0 )
    {
        std::vector<float> fftData;
        if( leftChannelFFTDataGenerator.getFFTData( fftData) )
        {
            pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -120.f);
        }
    }
    
    while( pathProducer.getNumPathsAvailable() > 0 )
    {
        pathProducer.getPath( leftChannelFFTPath );
    }
}

//==============================================================================
//
std::vector<juce::Component*> MultiMeterAudioProcessorEditor::getComps()
{
    return
    {
        &levelMeterDecaySlider,
        &averagerDurationSlider,
        &meterViewSlider,
        &scaleKnobSlider,
        &enableHoldSlier,
        &holdDurationSlider,
        &histogramViewSlider,
    };
}

//==============================================================================
//
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

    auto bounds = Rectangle<float>(x,y,width,height);

    auto enabled = slider.isEnabled();
    
    g.setColour(m_backgroundColour);
    g.fillEllipse(bounds);

    g.setColour(enabled ? Colours::white : Colours::grey);
    g.drawEllipse(bounds, 2.f);

    if( auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider) )
    {
        auto center = bounds.getCentre();

        Path p;
        Rectangle<float> r;
        r.setLeft( center.getX() - 2 );
        r.setRight( center.getX() + 2 );
        r.setTop( bounds.getY() + 2 );

        r.setBottom(center.getY() - rswl->getTextHeight()*1.5);

        p.addRoundedRectangle(r, 2.f);

        auto sliderAngRad = jmap(sliderPosProportional,
                                 0.f, 1.f,
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

        g.setColour(enabled ? m_backgroundColour : Colours::darkgrey);
        g.fillRect(r);

        g.setColour(enabled ? Colours::white : Colours::lightgrey);

        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

//==============================================================================
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
    for( int i = 0; i < numChoices; ++i )
    {
        auto pos = labels[i].pos;
        jassert( 0.f <= pos );
        jassert( pos <= 1.f );
        auto ang = jmap(labels[i].pos, 0.f, 1.f, startAng, endAng);

        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, ang);

        Rectangle<float> r;
        auto str = labels[i].label;

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
    if( auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param) )
        return choiceParam->getCurrentChoiceName();

    juce::String str;
    
    bool addK = false;

    if( auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param) )
    {
        float val = getValue();

        if( val > 999.f)
        {
            val /= 1000.f;
            addK = true;
        }

        str = juce::String(val,
                           (addK ? 2 : 0));
    }
    else
    {
        jassertfalse;
    }

    if( suffix.isNotEmpty() )
    {
        str << " ";
        if( addK )
            str << "k";
        str << suffix;
    }
    return str;
}
//==============================================================================


LogarithmicScale::LogarithmicScale()
{
    calculateBaseTenLogarithm();
    addLabels();
}



LogarithmicScale::~LogarithmicScale() {}


// ============================================================================
void LogarithmicScale::paint(juce::Graphics& g)
{
    g.setColour(m_gridColour);

    for (const auto& [frequency, x] : m_frequencyGridPoints)
    {
        g.drawLine(
            x,
            0,
            x,
            getHeight()
        );
    }

    for (const auto& [frequency, label] : m_labels)
    {
        label->setBounds(
            m_frequencyGridPoints[frequency] - 14,
            1,
            28,
            20
        );
    }
}



void LogarithmicScale::resized()
{
    calculateFrequencyGrid();
}


// ============================================================================
void LogarithmicScale::setGridColour(juce::Colour colour)
{
    m_gridColour = colour;
}



void LogarithmicScale::setTextColour(juce::Colour colour)
{
    m_textColour = colour;
}


// ===========================================================================
void LogarithmicScale::calculateBaseTenLogarithm()
{
    auto offsetInHertz
    {
        getOffsetInHertz(m_minimumFrequencyInHertz)
    };

    auto currentFrequencyInHertz
    {
        getCurrentFrequencyInHertz(m_minimumFrequencyInHertz, offsetInHertz)
    };

    if (currentFrequencyInHertz != m_minimumFrequencyInHertz)
    {
        m_baseTenLogarithm[m_minimumFrequencyInHertz] =
            std::log10f(
                static_cast<float>(m_minimumFrequencyInHertz)
            );

    }

    while (currentFrequencyInHertz < m_maximumFrequencyInHertz)
    {
        m_baseTenLogarithm[currentFrequencyInHertz] =
            std::log10f(
                static_cast<float>(currentFrequencyInHertz)
            );

        if (offsetInHertz * m_coefficient == currentFrequencyInHertz)
        {
            offsetInHertz *= m_coefficient;
        }
        currentFrequencyInHertz += offsetInHertz;
    }

    if (m_maximumFrequencyInHertz <= currentFrequencyInHertz)
    {
        m_baseTenLogarithm[m_maximumFrequencyInHertz] =
            std::log10f(
                static_cast<float>(m_maximumFrequencyInHertz)
            );
    }
}



void LogarithmicScale::calculateFrequencyGrid()
{
    auto sourceRangeMinimum = (m_baseTenLogarithm.begin())->second;
    auto sourceRangeMaximum = (--m_baseTenLogarithm.end())->second;

    auto targetRangeMinimum = 0.0f;
    auto targetRangeMaximem = static_cast<float>(getWidth());

    m_frequencyGridPoints.clear();

    for (const auto& [frequency, value] : m_baseTenLogarithm)
    {
        m_frequencyGridPoints[frequency] =
            juce::jmap(
                value,
                sourceRangeMinimum,
                sourceRangeMaximum,
                targetRangeMinimum,
                targetRangeMaximem
            );
    }
}



void LogarithmicScale::addLabels()
{
    for (auto frequency = 100; frequency <= 10000; frequency *= 10)
    {
        m_labels.insert(
            std::pair<int, std::unique_ptr<juce::Label>>(
                frequency,
                new juce::Label()
            )
        );
    }

    for (const auto& [frequency, label] : m_labels) {
        addAndMakeVisible(*label);
        label->setText(
            frequency == 100 ?
            juce::String(frequency) :
            juce::String(frequency / 1000) + "k",
            juce::NotificationType::dontSendNotification
        );
        label->setFont(12);
        label->setColour(juce::Label::textColourId, m_textColour);
        label->setJustificationType(juce::Justification::centredTop);
    }
}


// ===========================================================================
int LogarithmicScale::getOffsetInHertz(const int frequency)
{
    auto minimumForDivisions{ frequency };
    auto divisionCounter{ 1 };

    while (m_coefficient < (minimumForDivisions / m_coefficient))
    {
        minimumForDivisions /= m_coefficient;
        ++divisionCounter;
    }

    return
        static_cast<int>(
            pow(static_cast<float>(m_coefficient), divisionCounter)
            );
}



int LogarithmicScale::getCurrentFrequencyInHertz(
    const int currentFrequencyInHertz,
    const int offsetInHertz
) {
    if (currentFrequencyInHertz % offsetInHertz == 0)
    {
        return currentFrequencyInHertz;
    }
    else
    {
        auto newFrequency{ currentFrequencyInHertz };
        newFrequency -= currentFrequencyInHertz % offsetInHertz;
        return newFrequency + offsetInHertz;
    }
}



xGrid::xGrid(
    juce::AudioProcessorValueTreeState& audioProcessorValueTreeState
) :
    mr_audioProcessorValueTreeState(audioProcessorValueTreeState)
{
    addChildComponent(m_logarithmicScale);

}



xGrid::~xGrid() {}


// ===========================================================================
void xGrid::paint(juce::Graphics& g)
{
    g.setColour(m_gridColour);
    g.drawRect(getLocalBounds());

    calculateAmplitudeGrid();
    addLabels();

    for (const auto y : m_volumeGridPoints)
    {
        g.drawLine(
            0.0f,
            y,
            static_cast<float>(getWidth()),
            y
        );
    }

    for (const auto& [volume, label] : m_labels)
    {
        label->setBounds(
            0.0f,
            juce::jmap(
                static_cast<float>(volume),
                static_cast<float>(m_maximumVolumeInDecibels.load()),
                static_cast<float>(m_minimumVolumeInDecibels.load()),
                0.0f,
                static_cast<float>(getHeight())
            ) - 7.0f,
            28.0f,
            20.0f
        );
    }

    m_logarithmicScale.setVisible(m_gridStyleIsLogarithmic.load());

}



void xGrid::resized()
{
    m_logarithmicScale.setBounds(getLocalBounds());
    repaint();
}


// ============================================================================
void xGrid::setGridColour(juce::Colour colour)
{
    m_gridColour = colour;
}



void xGrid::setTextColour(juce::Colour colour)
{
    m_textColour = colour;
}


// ============================================================================
void xGrid::parameterChanged(
    const juce::String& parameterID,
    float newValue
) {

}

void xGrid::setVolumeRangeInDecibels(const int maximum, int minimum)
{
    if (maximum - 10 < minimum) { minimum = maximum - 10; }

    m_maximumVolumeInDecibels.store(maximum);
    m_minimumVolumeInDecibels.store(minimum);
}

// ============================================================================
void xGrid::calculateAmplitudeGrid()
{
    const auto maximum{ m_maximumVolumeInDecibels.load() };
    const auto minimum{ m_minimumVolumeInDecibels.load() };

    int rangeInDecibels;

    if (maximum < 0)
    {
        rangeInDecibels = (minimum - maximum) * -1;
    }
    else if (0 <= minimum)
    {
        rangeInDecibels = maximum - minimum;
    }
    else
    {
        rangeInDecibels = maximum + minimum * -1;
    }

    m_offsetInDecibels.store(0);
    auto offset{ 0.0f };

    while (offset < 16.0f)
    {
        m_offsetInDecibels.store(m_offsetInDecibels.load() + 6);

        offset =
            juce::jmap(
                static_cast<float>(m_offsetInDecibels.load()),
                0.0f,
                static_cast<float>(rangeInDecibels),
                0.0f,
                static_cast<float>(getHeight())
            );
    }

    m_firstOffsetInDecibels.store(maximum);

    while (
        m_firstOffsetInDecibels.load() %
        m_offsetInDecibels.load() !=
        0
        ) {
        m_firstOffsetInDecibels.store(m_firstOffsetInDecibels.load() - 1);
    }

    const auto first
    {
        juce::jmap(
            static_cast<float>(m_firstOffsetInDecibels.load()),
            static_cast<float>(maximum),
            static_cast<float>(minimum),
            0.0f,
            static_cast<float>(getHeight())
        )
    };

    const auto height{ static_cast<float>(getHeight()) };
    m_volumeGridPoints.clear();

    for (auto position{ first }; position < height; position += offset)
    {
        m_volumeGridPoints.push_back(position);
    }
}



void xGrid::addLabels()
{
    auto volume{ m_firstOffsetInDecibels.load() };
    const auto offset{ m_offsetInDecibels.load() };
    const auto minimum{ m_minimumVolumeInDecibels.load() };

    m_labels.clear();

    while (minimum < volume - offset)
    {
        volume -= offset;
        m_labels.insert(
            std::pair<int, std::unique_ptr<juce::Label>>(
                volume,
                new juce::Label()
            )
        );
    }

    for (const auto& [volume, label] : m_labels) {
        addAndMakeVisible(*label);
        label->setText(
            juce::String(volume),
            juce::NotificationType::dontSendNotification
        );
        label->setFont(12);
        label->setColour(juce::Label::textColourId, m_textColour);
        label->setJustificationType(juce::Justification::centredTop);
    }
}

