/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.
    This project is created using JUCE version 6.1.2.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================

//==============================================================================
MultiMeterAudioProcessor::MultiMeterAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
    pluginSettings(juce::ValueTree("multimeterSetting"))
{
}

MultiMeterAudioProcessor::~MultiMeterAudioProcessor()
{
}

//==============================================================================
const juce::String MultiMeterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MultiMeterAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MultiMeterAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MultiMeterAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MultiMeterAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MultiMeterAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int MultiMeterAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MultiMeterAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String MultiMeterAudioProcessor::getProgramName (int index)
{
    return {};
}

void MultiMeterAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void MultiMeterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    fifo.prepare(sampleRate, getTotalNumOutputChannels());
    
    leftChannelFifo.prepare(samplesPerBlock);
    rightChannelFifo.prepare(samplesPerBlock);
    
    #if USE_OSC
        juce::dsp::ProcessSpec spec;
        spec.maximumBlockSize = sampleRate;
        spec.sampleRate = samplesPerBlock;
        spec.numChannels = getTotalNumOutputChannels();
        
        osc.prepare(spec);
        gain.prepare(spec);
    #endif
}

void MultiMeterAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MultiMeterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void MultiMeterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    {
        buffer.clear(i, 0, buffer.getNumSamples());
    }
    
    #if USE_OSC
    buffer.clear();
    juce::dsp::AudioBlock<float> audioBlock {buffer};
    osc.setFrequency(440.0f);
    gain.setGainDecibels(JUCE_LIVE_CONSTANT(6.f));
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float nextOscillatorSample = osc.processSample(0.f);
        audioBlock.setSample(0, sample, nextOscillatorSample);
        audioBlock.setSample(1, sample, nextOscillatorSample);
    }
    gain.process(juce::dsp::ProcessContextReplacing<float>(audioBlock));
    #endif
    
    fifo.push(buffer);
    
    leftChannelFifo.update(buffer);
    rightChannelFifo.update(buffer);
    
    #if USE_OSC
    buffer.clear();
    #endif
}

//==============================================================================
bool MultiMeterAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* MultiMeterAudioProcessor::createEditor()
{
    return new MultiMeterAudioProcessorEditor (*this);
}

//==============================================================================
void MultiMeterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Serialize your ValueTree into destData
    juce::MemoryOutputStream stream(destData, false);
    pluginSettings.writeToStream(stream);
}

void MultiMeterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Deserialize your ValueTree from data
    juce::MemoryInputStream stream(data, sizeInBytes, false);
    pluginSettings = juce::ValueTree::readFromStream(stream);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout MultiMeterAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    juce::StringArray stringArrayDecayRate {"-3", "-6", "-12", "-24", "-36"};
    juce::StringArray stringArrayAverageDuration {"100", "250", "500", "1000", "2000"};
    juce::StringArray stringArrayMeterView {"Both", "Peak", "Avg"};
    juce::StringArray stringArrayEnableHold {"Show", "Hide"};
    juce::StringArray stringArrayHoldDuration {"0", "0.5", "2", "4", "6", "inf"};
    juce::StringArray stringArrayHistogramView {"Stacked", "Side-by-Side"};

    layout.add(std::make_unique<juce::AudioParameterChoice>("Decay Rate", "Decay Rate", stringArrayDecayRate, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("Average Duration", "Average Duration", stringArrayAverageDuration, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("Meter View", "Meter View", stringArrayMeterView, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("Enable Hold", "Enable Hold", stringArrayEnableHold, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("Hold Duration", "Hold Duration", stringArrayHoldDuration, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("Histogram View", "Histogram View", stringArrayHistogramView, 0));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Scale Knob",
                                                           "Scale Knob",
                                                           juce::NormalisableRange<float>(50.f, 200.f, 1.f, 0.1),
                                                           100.f));
    
    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MultiMeterAudioProcessor();
}
