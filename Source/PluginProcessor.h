/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.
    This project is created using JUCE version 6.1.2.

  ==============================================================================
*/

#pragma once

#define USE_OSC false

#include <JuceHeader.h>
#include <array>

//==============================================================================
//
using Filter = juce::dsp::IIR::Filter<float>;
using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, Filter, Filter, CutFilter>;

//==============================================================================
//
template<typename T, size_t Size>
struct Fifo
{
    size_t getSize() const noexcept {return Size;}
    
    void prepare(int numSamples, int numChannels)
    {
        for (auto& buffer : buffers)
        {
            buffer.setSize(numChannels,
                           numSamples,
                           false,
                           true,
                           true);
            buffer.clear();
        }
    }
    
    bool push(const T& t)
    {
        auto write = fifo.read(1);
        if (write.blockSize1 > 0)
        {
            buffers[write.startIndex1] = t;
            return true;
        }
        return false;
    }

    bool pull(T& t)
    {
        auto read = fifo.write(1);
        if (read.blockSize1 > 0)
        {
            t = buffers[read.startIndex1];
            return true;
        }
        return false;
    }
    
    int getNumAvailableForReading() const
    {
        //if (JUCE_WINDOWS)
        //    return fifo.getNumReady();
        //else
            fifo.getNumReady();
    }
    
    int getAvailableSpace() const
    {
        //if (JUCE_WINDOWS)
        //    return fifo.getFreeSpace();
        //else
            fifo.getFreeSpace();
    }
    
private:
    juce::AbstractFifo fifo {Size};
    std::array<T, Size> buffers;
};

//==============================================================================
//
template<typename T>
struct FifoSpectrumAnalyzer
{
    void prepare(int numChannels, int numSamples)
    {
        for(auto& buffer : buffers)
        {
            buffer.setSize(numChannels, numSamples, false, true, true);
            buffer.clear();
        }
    }
    
    void prepare(size_t numElements)
    {
        for(auto& buffer : buffers)
        {
            buffer.clear();
            buffer.resize(numElements, 0);
        }
    }
    
    bool push(const T& t)
    {
        auto write = fifo.write(1);
        if(write.blockSize1 > 0)
        {
            buffers[write.startIndex1] = t;
            return true;
        }
        
        return false;
    }
    
    bool pull(T& t)
    {
        auto read = fifo.read(1);
        if(read.blockSize1 > 0)
        {
            t = buffers[read.startIndex1];
            return true;
        }
        
        return false;
    }
    
    int getNumAvailableForReading() const
    {
        return fifo.getNumReady();
    }

private:
    static constexpr int Capacity = 30;
    std::array<T, Capacity> buffers;
    juce::AbstractFifo fifo {Capacity};
};

//==============================================================================
//
enum Channel
{
    Left,
    Right,
};

//==============================================================================
//
template<typename BlockType>
struct SingleChannelSampleFifo
{
    SingleChannelSampleFifo(Channel ch) : channelToUse(ch)
    {
        prepared.set(false);
    }
    
    void update(const BlockType& buffer)
    {
        jassert(prepared.get());
        if (buffer.getNumChannels() > channelToUse)
        {
            auto* channelPtr = buffer.getReadPointer(channelToUse);

            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                pushNextSampleIntoFifo(channelPtr[i]);
            }
        }
    }

    void prepare(int bufferSize)
    {
        prepared.set(false);
        size.set(bufferSize);
        
        bufferToFill.setSize(1, bufferSize, false, true, true);
        audioBufferFifo.prepare(1, bufferSize);
        fifoIndex = 0;
        prepared.set(true);
    }
    
    int getNumCompleteBuffersAvailable() const
    {
        return audioBufferFifo.getNumAvailableForReading();
    }
    
    bool isPrepared() const
    {
        return prepared.get();
    }
    
    int getSize() const
    {
        return size.get();
    }
    
    bool getAudioBuffer(BlockType& buf)
    {
        return audioBufferFifo.pull(buf);
    }
 
private:
    Channel channelToUse;
    int fifoIndex = 0;
    FifoSpectrumAnalyzer<BlockType> audioBufferFifo;
    BlockType bufferToFill;
    juce::Atomic<bool> prepared = false;
    juce::Atomic<int> size = 0;
    
    void pushNextSampleIntoFifo(float sample)
    {
        if (fifoIndex == bufferToFill.getNumSamples())
        {
            auto ok = audioBufferFifo.push(bufferToFill);

            juce::ignoreUnused(ok);
            
            fifoIndex = 0;
        }
        
        bufferToFill.setSample(0, fifoIndex, sample);
        ++fifoIndex;
    }
};

//==============================================================================
//
enum decayRate
{
    decayRate_3,
    decayRate_6,
    decayRate_12,
    decayRate_24,
    decayRate_36,
};

//==============================================================================
//
enum averageDuration
{
    averageDuration_100,
    averageDuration_250,
    averageDuration_500,
    averageDuration_1000,
    averageDuration_2000,
};

//==============================================================================
//
enum meterView
{
    meterView_both,
    meterView_peak,
    meterView_average,
};

//==============================================================================
//
enum enableHold
{
    enableHold_show,
    enableHold_hide,
};

//==============================================================================
//
enum holdDuration
{
    holdDuration_00,
    holdDuration_05,
    holdDuration_2,
    holdDuration_4,
    holdDuration_6,
    holdDuration_inf,
};

//==============================================================================
//
enum histogramView
{
    histogramView_stacked,
    histogramView_sideBySide,
};

//==============================================================================
//
class MultiMeterAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    MultiMeterAudioProcessor();
    ~MultiMeterAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts{*this, nullptr, "Parameters", createParameterLayout()};
    
    void update();

    using BlockType = juce::AudioBuffer<float>;
    SingleChannelSampleFifo<BlockType> leftChannelFifo {Channel::Left};
    SingleChannelSampleFifo<BlockType> rightChannelFifo {Channel::Right};
    
    Fifo<juce::AudioBuffer<float>, 30> fifo;

    juce::ValueTree pluginSettings;
    
    #if USE_OSC
    juce::dsp::Oscillator<float> osc {[](float x){return std::sin(x);}};
    juce::dsp::Gain<float> gain;
    #endif
    
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultiMeterAudioProcessor)
};
