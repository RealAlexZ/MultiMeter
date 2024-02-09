/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.
    This project is created using JUCE version 6.1.2.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CustomComponents.h"


// Two macros to set bounds for Dbs.
#define NEGATIVE_INFINITY -120.f
#define MAX_DECIBELS 12.f

#define BACKGROUND_COLOR juce::Colour{ 0xffd2d2d2 }
#define HIGHLIGHT_COLOR  juce::Colour{ 0xff48bde8 }
#define BASE_COLOR  juce::Colour{ 0xff323232  }

//==============================================================================
struct Tick
{
    float db { 0.f };
    int y { 0 };
};

// ****************************************************************************
// LOGARITHMIC SCALE CLASS
// ****************************************************************************
class LogarithmicScale : public juce::Component
{
public:
    LogarithmicScale();
    ~LogarithmicScale() override;


    // ========================================================================
    void paint(juce::Graphics&) override;
    void resized() override;


    // ========================================================================
    void setGridColour(juce::Colour);
    void setTextColour(juce::Colour);


private:
    // ========================================================================
    void calculateBaseTenLogarithm();
    void calculateFrequencyGrid();
    void addLabels();


    // ========================================================================
    int getOffsetInHertz(const int);
    int getCurrentFrequencyInHertz(const int, const int);


    // ========================================================================
    juce::Colour m_gridColour { 0xff464646 };
    juce::Colour m_textColour { 0xff848484 };

    int m_coefficient{ 10 };
    int m_maximumFrequencyInHertz{ 20000 };
    int m_minimumFrequencyInHertz{ 20 };

    std::map<int, float> m_baseTenLogarithm;
    std::map<int, float> m_frequencyGridPoints;
    std::map<int, std::unique_ptr<juce::Label>> m_labels;


    // ========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LogarithmicScale)
};


// ****************************************************************************
// GRID CLASS
// ****************************************************************************
class xGrid :
    public juce::Component,
    public juce::AudioProcessorValueTreeState::Listener
{
public:

    // =======================================================================
    xGrid(juce::AudioProcessorValueTreeState&);
    ~xGrid() override;


    // ========================================================================
    void paint(juce::Graphics&) override;
    void resized() override;


    // ========================================================================
    void setGridColour(juce::Colour);
    void setTextColour(juce::Colour);

private:
    // ========================================================================
    void parameterChanged(const juce::String&, float) override;


    // ========================================================================
    void setVolumeRangeInDecibels(const int, int);
    void calculateAmplitudeGrid();
    void addLabels();


    // ========================================================================
    juce::AudioProcessorValueTreeState& mr_audioProcessorValueTreeState;

    LogarithmicScale m_logarithmicScale;

    juce::Colour m_gridColour { 0xff464646 };
    juce::Colour m_textColour { 0xff848484 };

    std::atomic<bool> m_gridStyleIsLogarithmic { true };

    std::atomic<int> m_maximumVolumeInDecibels { 12 };
    std::atomic<int> m_minimumVolumeInDecibels { -120 };
    std::atomic<int> m_firstOffsetInDecibels;
    std::atomic<int> m_offsetInDecibels;

    std::vector<float> m_volumeGridPoints;
    std::map<int, std::unique_ptr<juce::Label>> m_labels;


    // ========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(xGrid)
};

//==============================================================================
struct DbScale : juce::Component
{
    ~DbScale() override = default;
    
    void paint(juce::Graphics& g) override;
    
    void buildBackgroundImage(int dbDivision, juce::Rectangle<int> meterBounds, int minDb, int maxDb);
    
    static std::vector<Tick> getTicks(int dbDivision, juce::Rectangle<int> meterBounds, int minDb, int maxDb);
    
private:
    juce::Image bkgd;
    bool show_tick = true;
};

//==============================================================================
struct ValueHolder : juce::Timer
{
    ValueHolder();
    ~ValueHolder();
    
    void timerCallback() override;
    
    void setThreshold(float th);
    
    void updateHeldValue(float v);
    
    void setHoldTime(int ms);
    
    float getCurrentValue() const;
    
    float getHeldValue() const;
    
    bool getIsOverThreshold() const;
    
private:
    float threshold = 0;
    float currentValue = NEGATIVE_INFINITY;
    float heldValue = NEGATIVE_INFINITY;
    juce::int64 timeOfPeak;
    int durationToHoldForMs {500};
    bool isOverThreshold { false };
};

//==============================================================================
struct TextMeter : juce::Component
{
    TextMeter();
    
    void paint(juce::Graphics& g) override;
    
    void update(float valueDb);
    
private:
    float cachedValueDb;
    ValueHolder valueHolder;
    juce::Colour meterBgColour { 0xff323232 };
    juce::Colour meterLevelColour { 0xff48bde8 };
};

//==============================================================================
struct DecayingValueHolder : juce::Timer
{
    DecayingValueHolder();
    
    void updateHeldValue(float input);
    
    float getCurrentValue() const { return currentValue; };
    
    bool isOverThreshold() const { return currentValue > threshold; };
    
    void setHoldTime(int ms) { holdTime = ms; };
    
    void setLevelMeterDecay(float dbPerSec) { decayRatePerFrame = dbPerSec / 60.f; };

    void setCurrentValue(float val);
    
    void timerCallback() override;
    
    juce::int64 getHoldTime();
    
private:
    static juce::int64 getNow() { return juce::Time::currentTimeMillis(); };
    
    void resetLevelMeterDecayMultiplier() { decayRateMultiplier = 1; };
    
    float currentValue { NEGATIVE_INFINITY };
    juce::int64 peakTime = getNow();
    float threshold = 0.f;
    juce::int64 holdTime = 2000; // 2 seconds.
    float decayRatePerFrame { 0 };
    float decayRateMultiplier { 1 };
};

//==============================================================================
struct Meter : juce::Component
{
    void paint(juce::Graphics&) override;
    
    void update(float dbLevel, float decay_rate, float hold_time_, bool reset_hold, bool show_tick_);
    
private:
    float peakDb { NEGATIVE_INFINITY };
    bool show_tick = false;
    DecayingValueHolder decayingValueHolder;

    juce::Colour meterLevelColour { 0xff48bde8 };
    juce::Colour meterBgColour { 0xff323232 };
};

//==============================================================================
template<typename T>
struct Averager
{
    Averager(size_t numElements, T initialValue)
    {
        resize(numElements, initialValue);
        init_size = numElements;
    }
    
    void resize(size_t numElements, T initialValue)
    {
        elements.resize(numElements);
        clear(initialValue);
    }
    
    void clear(T initialValue)
    {
        elements.assign(getSize(), initialValue);
        writeIndex.store(0);
        sum.store(initialValue * getSize());
        avg.store(initialValue);
    }
    
    size_t getSize() const
    {
        return elements.size();
    }
    
    void add(T t)
    {
        auto writeIndexCopy = writeIndex.load();
        auto sumCopy = sum.load();
        
        sumCopy -= elements[writeIndex];
        sumCopy += t;
        elements[writeIndex] = t;
        ++writeIndexCopy;
        if ( writeIndexCopy == getSize())
        {
            writeIndexCopy = 0;
        }
        
        writeIndex.store(writeIndexCopy);
        sum.store(sumCopy);
        avg = static_cast<float>(sumCopy) / static_cast<float>(getSize());
    }
    
    float getAvg() const
    {
        return avg.load();
    }

    void setAveragerDuration(juce::int64 duration)
    {
        size_t new_size = static_cast<size_t>(init_size* duration / DEFAULT_SAMPLE_INTERVAL_MS);
        resize(new_size, static_cast<T>(getAvg()));
    }

private:
    std::vector<T> elements;
    std::atomic<float> avg { static_cast<float>(T()) };
    std::atomic<size_t> writeIndex = { 0 }; // Added {} wrapping around 0.
    std::atomic<T> sum { 0 };
    size_t init_size = 0;
    static constexpr int DEFAULT_SAMPLE_INTERVAL_MS = 100;
};

//==============================================================================
class MacroMeter : public juce::Component
{
public:
    MacroMeter();
    
    void resized() override;
    
    void update(float level, float decay_rate, bool show_peak, bool shwo_avg, float hold_time_, bool reset_hold, bool show_tick_);
    
private:
    TextMeter textMeter;
    Meter instantMeter, averageMeter;
    Averager<float> averager;
    bool show_peak_ = true; bool show_avg_ = true;
};

//==============================================================================
class StereoMeter : public juce::Component
{
public:
    StereoMeter(juce::String nameInput);
    
    void paint(juce::Graphics& g) override;
    
    void resized() override;
    
    void update(float leftChanDb, float rightChanDb, float decay_rate, int meterViewID, bool show_tick, float hold_time_, bool reset_hold);
    
    void setText(juce::String labelName);
    
private:
    juce::Rectangle<int> labelTextArea;
    juce::String labelText;
    MacroMeter leftMeter, rightMeter;
    DbScale dbScale;
    juce::Label label;
};

//==============================================================================
template<typename T>
struct ReadAllAfterWriteCircularBuffer
{
    using DataType = std::vector<T>;
    
    ReadAllAfterWriteCircularBuffer(T fillValue)
    {
        resize(1, fillValue);
    }

    void resize(std::size_t s, T fillValue)
    {
        data.resize(s);
        clear(fillValue);
    }
    
    void clear(T fillValue)
    {
        data.assign(getSize(), fillValue);
        resetWriteIndex();
    }
    
    void write(T t)
    {
        auto indexCopy = writeIndex.load();
        data[indexCopy] = t;
        ++indexCopy;
        if (indexCopy == getSize())
        {
            indexCopy = 0;
        }
        writeIndex.store(indexCopy);
    }

    DataType& getData()
    {
        return data;
    }
    
    size_t getReadIndex() const
    {
        /*
        size_t nextIndex = writeIndex.load() + 1;
        if(nextIndex >= getSize())
        {
            nextIndex = 0;
        }
        return nextIndex;
         */
        return writeIndex.load();
    }
    
    size_t getSize() const
    {
        return data.size();
    }

private:
    void resetWriteIndex()
    {
        writeIndex.store(0);
    }
    
    std::atomic<std::size_t> writeIndex {0};
    DataType data;
};


//==============================================================================
struct Histogram : juce::Component
{
    Histogram(const juce::String& titleInput);
    
    void paint(juce::Graphics& g) override;
    
    void resized() override;
    
    void mouseDown(const juce::MouseEvent& e) override;
    
    void update(float value);
    
private:
    void displayPath(juce::Graphics& g, juce::Rectangle<float> bounds);
    
    static juce::Path buildPath(juce::Path& p,
                                ReadAllAfterWriteCircularBuffer<float>& buffer,
                                juce::Rectangle<float> bounds)
    {
        p.clear();
        auto bufferSizeCopy = buffer.getSize();
        auto bottomOfBoundsCopy = bounds.getBottom();
        auto& bufferDataCopy = buffer.getData();
        int readIndexCopy = static_cast<int>(buffer.getReadIndex());
        
        auto map = [bottomOfBoundsCopy](float db) { return juce::jmap(db, NEGATIVE_INFINITY, MAX_DECIBELS, bottomOfBoundsCopy, 0.f); };
        auto increment = [bufferSizeCopy](int& index) mutable { index = (index + 1) % bufferSizeCopy; };
        
        p.startNewSubPath(0, map(bufferDataCopy[readIndexCopy]));
        increment(readIndexCopy);
        
        for (int i = 1; i < bounds.getWidth(); ++i)
        {
            p.lineTo(i, map(bufferDataCopy[readIndexCopy]));
            increment(readIndexCopy);
        }
        
        juce::Path fillPath = p;
        if (p.getBounds().getHeight() > 0)
        {
            fillPath.lineTo(bounds.getRight(), bottomOfBoundsCopy);
            fillPath.lineTo(bounds.getX(), bottomOfBoundsCopy);
            fillPath.closeSubPath();
            return fillPath;
        }
        return {};
    }
    
    ReadAllAfterWriteCircularBuffer<float> buffer {float(NEGATIVE_INFINITY)};
    juce::Path path;
    const juce::String title;
};

//==============================================================================
struct Goniometer : juce::Component
{
    Goniometer(juce::AudioBuffer<float>& bufferInput);
    
    void paint(juce::Graphics& g) override;
    
    void resized() override;
    
    void update(juce::AudioBuffer<float>& buffer);

    void updateCoeff(float new_db);
    
private:
    void drawBackground(juce::Graphics& g);
    
    juce::AudioBuffer<float>& buffer;
    juce::AudioBuffer<float> internalBuffer;
    juce::Path p;
    int w, h;
    juce::Point<int> center;
    std::vector<juce::String> chars { "+S", "-S", "L", "M", "R" };
    float scale;

    juce::Colour m_backgroundColour { 0xff323232 };
    juce::Colour edgeColour { 0xffd2d2d2 };
    juce::Colour pathColourInside { 0xffd2d2d2 };
    juce::Colour pathColourOutside { 0xff48bde8 };
};

//==============================================================================
//
struct CorrelationMeter : juce::Component
{
    CorrelationMeter(juce::AudioBuffer<float>& buf, double sampleRate);
    void paint(juce::Graphics& g) override;
    void update(juce::int64 average_time);
private:
    juce::AudioBuffer<float>& buffer;
    using FilterType = juce::dsp::IIR::Filter<float>;
    std::array<FilterType, 3> filters;

    juce::Colour m_backgroundColour { 0xff323232 };
    juce::Colour graphColour { 0xff48bde8 };

    Averager<float> slowAverager{1024*3, 0}, peakAverager{512, 0};
    
    void drawAverage(juce::Graphics& g,
                     juce::Rectangle<int> bounds,
                     float avg,
                     bool drawBorder);
};

//==============================================================================
//
enum FFTOrder
{
    order2048 = 11,
    order4096 = 12,
    order8192 = 13
};

//==============================================================================
//
template<typename BlockType>
struct FFTDataGenerator
{
    void produceFFTDataForRendering(const juce::AudioBuffer<float>& audioData, const float negativeInfinity)
    {
        const auto fftSize = getFFTSize();
        
        fftData.assign(fftData.size(), 0);
        auto* readIndex = audioData.getReadPointer(0);
        std::copy(readIndex, readIndex + fftSize, fftData.begin());
        
        window->multiplyWithWindowingTable (fftData.data(), fftSize);
        
        forwardFFT->performFrequencyOnlyForwardTransform (fftData.data());
        
        int numBins = (int)fftSize / 2;
        
        for( int i = 0; i < numBins; ++i )
        {
            auto v = fftData[i];
            if( !std::isinf(v) && !std::isnan(v) )
            {
                v /= float(numBins);
            }
            else
            {
                v = 0.f;
            }
            fftData[i] = v;
        }
        
        for( int i = 0; i < numBins; ++i )
        {
            fftData[i] = juce::Decibels::gainToDecibels(fftData[i], negativeInfinity);
        }
        
        fftDataFifo.push(fftData);
    }
    
    void changeOrder(FFTOrder newOrder)
    {
        order = newOrder;
        auto fftSize = getFFTSize();
        
        forwardFFT = std::make_unique<juce::dsp::FFT>(order);
        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);
        
        fftData.clear();
        fftData.resize(fftSize * 2, 0);

        fftDataFifo.prepare(fftData.size());
    }

    int getFFTSize() const
    {
        return 1 << order;
    }
    
    int getNumAvailableFFTDataBlocks() const
    {
        return fftDataFifo.getNumAvailableForReading();
    }
    
    bool getFFTData(BlockType& fftData)
    {
        return fftDataFifo.pull(fftData);
    }
    
private:
    FFTOrder order;
    BlockType fftData;
    std::unique_ptr<juce::dsp::FFT> forwardFFT;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;
    FifoSpectrumAnalyzer<BlockType> fftDataFifo;
};

//==============================================================================
//
template<typename PathType>
struct AnalyzerPathGenerator
{
    void generatePath(const std::vector<float>& renderData,
                      juce::Rectangle<float> fftBounds,
                      int fftSize,
                      float binWidth,
                      float negativeInfinity)
    {
        auto top = fftBounds.getY();
        auto bottom = fftBounds.getHeight();
        auto width = fftBounds.getWidth();

        int numBins = (int)fftSize / 2;

        PathType p;
        p.preallocateSpace(3 * (int)fftBounds.getWidth());

        auto map = [bottom, top, negativeInfinity](float v)
        {
            return juce::jmap(v,
                              negativeInfinity, 0.f,
                              float(bottom+1),   top);
        };

        auto y = map(renderData[0]);

        jassert( !std::isnan(y) && !std::isinf(y) );

        p.startNewSubPath(0, y);

        const int pathResolution = 1;

        for( int binNum = 1; binNum < numBins; binNum += pathResolution )
        {
            y = map(renderData[binNum]);

            if( !std::isnan(y) && !std::isinf(y) )
            {
                auto binFreq = binNum * binWidth;
                auto normalizedBinX = juce::mapFromLog10(binFreq, 20.f, 20000.f);
                int binX = std::floor(normalizedBinX * width);
                p.lineTo(binX, y);
            }
        }

        pathFifo.push(p);
    }

    int getNumPathsAvailable() const
    {
        return pathFifo.getNumAvailableForReading();
    }

    bool getPath(PathType& path)
    {
        return pathFifo.pull(path);
    }
private:
    FifoSpectrumAnalyzer<PathType> pathFifo;
};

//==============================================================================
//
struct PathProducer
{
    PathProducer(SingleChannelSampleFifo<MultiMeterAudioProcessor::BlockType>& scsf) :
    leftChannelFifo(&scsf)
    {
        leftChannelFFTDataGenerator.changeOrder(FFTOrder::order2048);
        monoBuffer.setSize(1, leftChannelFFTDataGenerator.getFFTSize());
    }

    void process(juce::Rectangle<float> fftBounds, double sampleRate);

    juce::Path getPath() { return leftChannelFFTPath; }

private:
    SingleChannelSampleFifo<MultiMeterAudioProcessor::BlockType>* leftChannelFifo;
    juce::AudioBuffer<float> monoBuffer;
    FFTDataGenerator<std::vector<float>> leftChannelFFTDataGenerator;
    AnalyzerPathGenerator<juce::Path> pathProducer;
    juce::Path leftChannelFFTPath;
};

//==============================================================================
//
struct ResponseCurveComponent : juce::Component,
juce::Timer
{
    ResponseCurveComponent(MultiMeterAudioProcessor&);

    void paint (juce::Graphics&) override;
    void paintOverChildren(Graphics& g) override;

    void timerCallback() override;

    void resized() override;
    
private:
    MultiMeterAudioProcessor& audioProcessor;

    juce::Colour m_backgroundColour { 0xff323232 };

    juce::Colour m_GraphColourLeft { 0xff48bde8 };
    juce::Colour m_GraphColourRight { 0xffa0a0a0 };

    xGrid m_grid;
    
    MonoChain monoChain;
    
    void drawBackgroundGrid(juce::Graphics& g);
    void drawTextLabels(juce::Graphics& g);
    
    std::vector<float> getFrequencies();
    std::vector<float> getGains();
    std::vector<float> getXs(const std::vector<float>& freqs, float left, float width);

    juce::Rectangle<int> getRenderArea();
    
    juce::Rectangle<int> getAnalysisArea();

    PathProducer leftPathProducer, rightPathProducer;
};




///*
//==============================================================================
//
struct myLookAndFeel : juce::LookAndFeel_V4
{
    juce::Colour m_backgroundColour { 0xff323232 };

    void drawRotarySlider(juce::Graphics & g,
                         int     x,
                         int     y,
                         int     width,
                         int     height,
                         float     sliderPosProportional,
                         float     rotaryStartAngle,
                         float     rotaryEndAngle,
                         juce::Slider & slider
                         ) override;
};

//==============================================================================
//
struct RotarySliderWithLabels : juce::Slider
{
    RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix) :
    juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
                 juce::Slider::TextEntryBoxPosition::NoTextBox),
                 param(&rap),
                 suffix(unitSuffix)
    {
        setLookAndFeel(&lnf);
    }
    
    ~RotarySliderWithLabels()
    {
        setLookAndFeel(nullptr);
    }

    struct LabelPos
    {
        float pos;
        juce::String label;
    };

    juce::Array<LabelPos> labels;
    
    void paint(juce::Graphics& g) override;
    
    juce::Rectangle<int> getSliderBounds() const;
    
    int getTextHeight() const
    {
        return 14;
    }
    
    juce::String getDisplayString() const;
    
private:
    myLookAndFeel lnf;
    juce::RangedAudioParameter* param;
    juce::String suffix;
};
//*/
 
//==============================================================================
class MultiMeterAudioProcessorEditor  : public juce::AudioProcessorEditor, juce::Timer, juce::ComboBox::Listener, juce::ToggleButton::Listener, juce::Slider::Listener
{
public:
    MultiMeterAudioProcessorEditor (MultiMeterAudioProcessor&);
    ~MultiMeterAudioProcessorEditor() override;
    
    void paint(juce::Graphics&) override;

    void resized() override;

    void timerCallback() override;

    void comboBoxChanged(juce::ComboBox* comboBox) override;
    void buttonClicked(juce::Button* button) override;
    void sliderValueChanged(juce::Slider* slider) override;
    
    juce::AudioBuffer<float> buffer{2, 256};
    StereoMeter peakMeter{"PEAK"}, RMSMeter{"RMS"};
    Histogram peakHistogram{"PEAK"}, RMSHistogram{"RMS"};
    
private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    MultiMeterAudioProcessor& audioProcessor;
    Goniometer goniometer;
    CorrelationMeter correlationMeter;
    ResponseCurveComponent spectrumAnalyzer;

    CustomLook2 lookandfeel;

    SwitchButton visualSwitch;

    int globalWidth{ 800 }, gloablHeight{400};

    juce::Rectangle<int> visualsRoom, meterRoom, controlRoom, correlationRoom,testSpace;

    juce::Colour backgroundColour { 0xffd2d2d2 };

    static constexpr double m_marginInPixels{ 10 };

    // all combobox controls are defined here
    juce::ComboBox levelMeterDecaySelector, averagerDurationSelector, holdTimeSelector;
    Switch tickDisplay{ "Hide Tick","Show Tick" }, resetHold{"Reset Hold","Reset Hold"};

    ToggleChain histogramViewButton, meterViewButton;

    juce::Label levelMeterDecayLabel, averagerDurationLabel, meterViewLabel, holdTimeLabel, histogramViewLabel, tickDisplayLabel, correlationLabel0, correlationLabel1, correlationLabel2, scaleKnobLabel;

    float current_decay_rate = 3.f; //define a variable to hold the current decay rate being used and update the variable from combo box changed callback function
    float hold_time = 2.f; // use this variable to store hold time
    juce::int64 averager_duration = 100; //use the default average time 100 for averager.

    // use this rectangle to define bounds for side by side and stacked histogram positions
    juce::Rectangle<int> peakSBS, rmsSBS, peakStacked, rmsStacked;

    RotarySliderWithLabels levelMeterDecaySlider,
                           averagerDurationSlider,
                           meterViewSlider,
                           scaleKnobSlider,
                           enableHoldSlier,
                           holdDurationSlider,
                           histogramViewSlider;
    
    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    
    Attachment levelMeterDecaySliderAttachment,
               averagerDurationSliderAttachment,
               meterViewSliderAttachment,
               scaleKnobSliderAttachment,
               enableHoldSlierAttachment,
               holdDurationSliderAttachment,
               histogramViewSliderAttachment;
    
    std::vector<juce::Component*> getComps();
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultiMeterAudioProcessorEditor)
};

