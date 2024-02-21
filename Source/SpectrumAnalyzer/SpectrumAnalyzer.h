

#pragma once
#include <JuceHeader.h>
#include "../Constants.h"
#include "../PluginProcessor.h"

//==============================================================================
// Enumeration FFTOrder
// Represents different orders for Fast Fourier Transform (FFT)
enum FFTOrder
{
    order2048 = 11,
    order4096 = 12,
    order8192 = 13
};

//==============================================================================
// Class definition for LogarithmicScale
class LogarithmicScale : public juce::Component
{
public:
    // Constructor
    LogarithmicScale();
    // Destructor
    ~LogarithmicScale() override;

    // Overrides the paint function to draw the logarithmic scale
    void paint(juce::Graphics&) override;
    // Overrides the resized function to handle component resizing
    void resized() override;

    // Function to set the grid color
    void setGridColour(juce::Colour);
    // Function to set the text color
    void setTextColour(juce::Colour);

private:
    // Function to calculate the base ten logarithm for frequency
    void calculateBaseTenLogarithm();
    // Function to calculate the frequency grid
    void calculateFrequencyGrid();
    // Function to add labels to the scale
    void addLabels();

    // Function to calculate the offset in hertz
    int getOffsetInHertz(const int);
    // Function to get the current frequency in hertz
    int getCurrentFrequencyInHertz(const int, const int);

    // Color for the grid
    juce::Colour gridColor { 0xff464646 };
    // Color for the text
    juce::Colour textColor { 0xff848484 };

    // Coefficient for the logarithmic scale
    int coefficient{ 10 };
    // Maximum frequency in hertz
    int maxFreqHz{ 20000 };
    // Minimum frequency in hertz
    int minFreqHz{ 20 };

    // Map to store base ten logarithms
    std::map<int, float> baseTenLog;
    // Map to store frequency grid points
    std::map<int, float> freqGridPoints;
    // Map to store labels
    std::map<int, std::unique_ptr<juce::Label>> labels;

    // Macro to declare the class as non-copyable with leak detector
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LogarithmicScale)
};

//==============================================================================
// Class definition for SpectrumGrid
class SpectrumGrid :
    public juce::Component
{
public:
    // Constructor
    SpectrumGrid(juce::AudioProcessorValueTreeState&);
    // Destructor
    ~SpectrumGrid() override;

    // Overrides the paint function to draw the grid
    void paint(juce::Graphics&) override;
    // Overrides the resized function to handle component resizing
    void resized() override;

    // Function to set the grid color
    void setGridColour(juce::Colour);
    // Function to set the text color
    void setTextColour(juce::Colour);

private:
    // Function to set the volume range in decibels
    void setVolumeRangeInDecibels(const int, int);
    // Function to calculate the amplitude grid
    void calculateAmplitudeGrid();
    // Function to add labels to the grid
    void addLabels();

    // Reference to the audio processor's value tree state
    juce::AudioProcessorValueTreeState& mr_audioProcessorValueTreeState;
    // Logarithmic scale object
    LogarithmicScale m_logarithmicScale;
    // Color for the grid
    juce::Colour gridColor { 0xff464646 };
    // Color for the text
    juce::Colour textColor { 0xff848484 };
    // Atomic boolean to indicate if the grid style is logarithmic
    std::atomic<bool> m_gridStyleIsLogarithmic { true };
    // Atomic integers for maximum and minimum decibels, first offset, and offset decibel
    std::atomic<int> maxDecibel { 12 };
    std::atomic<int> minDecibel { -120 };
    std::atomic<int> firstOffset;
    std::atomic<int> offsetDecibel;
    // Vector to store grid points
    std::vector<float> gridPoints;
    // Map to store labels
    std::map<int, std::unique_ptr<juce::Label>> labels;

    // Macro to declare the class as non-copyable with leak detector
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumGrid)
};

//==============================================================================
// Struct definition for FFTDataGenerator
template<typename BlockType>
struct FFTDataGenerator
{
    // Function to produce FFT data suitable for rendering
    void produceFFTDataForRendering(const juce::AudioBuffer<float>& audioData, const float negativeInfinity)
    {
        // Get the FFT size
        const auto fftSize = getFFTSize();

        // Reset the FFT data and copy audio data into it
        fftData.assign(fftData.size(), 0);
        auto* readIndex = audioData.getReadPointer(0);
        std::copy(readIndex, readIndex + fftSize, fftData.begin());

        // Apply windowing to the FFT data
        window->multiplyWithWindowingTable(fftData.data(), fftSize);

        // Perform forward FFT
        forwardFFT->performFrequencyOnlyForwardTransform(fftData.data());

        // Normalize FFT data and convert to decibels
        int numBins = (int)fftSize / 2;
        for (int i = 0; i < numBins; ++i)
        {
            auto v = fftData[i];
            if (!std::isinf(v) && !std::isnan(v))
            {
                v /= float(numBins);
            }
            else
            {
                v = 0.f;
            }
            fftData[i] = juce::Decibels::gainToDecibels(v, negativeInfinity);
        }

        // Push the processed FFT data into the FIFO
        fftDataFifo.push(fftData);
    }

    // Function to change the FFT order
    void changeOrder(FFTOrder newOrder)
    {
        // Update the FFT order
        order = newOrder;
        auto fftSize = getFFTSize();

        // Recreate forward FFT and windowing objects
        forwardFFT = std::make_unique<juce::dsp::FFT>(order);
        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);

        // Clear and resize the FFT data buffer
        fftData.clear();
        fftData.resize(fftSize * 2, 0);

        // Prepare the FIFO with the new size
        fftDataFifo.prepare(fftData.size());
    }

    // Function to get the FFT size
    int getFFTSize() const
    {
        return 1 << order;
    }

    // Function to get the number of available FFT data blocks in the FIFO
    int getNumAvailableFFTDataBlocks() const
    {
        return fftDataFifo.getNumAvailableForReading();
    }

    // Function to retrieve FFT data from the FIFO
    bool getFFTData(BlockType& fftData)
    {
        return fftDataFifo.pull(fftData);
    }

private:
    FFTOrder order; // Order of the FFT
    BlockType fftData; // Buffer for FFT data
    std::unique_ptr<juce::dsp::FFT> forwardFFT; // Forward FFT object
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window; // Windowing function object
    FifoSpectrumAnalyzer<BlockType> fftDataFifo; // FIFO for storing FFT data
};

//==============================================================================
// Struct definition for AnalyzerPathGenerator
template<typename PathType>
struct AnalyzerPathGenerator
{
    // Function to generate a path based on render data, FFT bounds, etc.
    void generatePath(const std::vector<float>& renderData,
        juce::Rectangle<float> fftBounds,
        int fftSize,
        float binWidth,
        float negativeInfinity)
    {
        // Extract FFT bounds properties
        auto top = fftBounds.getY();
        auto bottom = fftBounds.getHeight();
        auto width = fftBounds.getWidth();

        // Calculate the number of FFT bins
        int numBins = (int)fftSize / 2;

        // Create a new path
        PathType p;
        // Preallocate space for the path
        p.preallocateSpace(3 * (int)fftBounds.getWidth());

        // Lambda function to map render data to y-coordinates
        auto map = [bottom, top, negativeInfinity](float v)
        {
            return juce::jmap(v, negativeInfinity, 0.f, float(bottom + 1), top);
        };

        // Map the first render data point to a y-coordinate
        auto y = map(renderData[0]);
        // Check for NaN or infinity
        jassert(!std::isnan(y) && !std::isinf(y));
        // Start a new subpath at (0, y)
        p.startNewSubPath(0, y);

        // Define the resolution for the path
        const int pathResolution = 1;

        // Iterate over the bins and create path segments
        for (int binNum = 1; binNum < numBins; binNum += pathResolution)
        {
            // Map the render data to a y-coordinate
            y = map(renderData[binNum]);

            // If y-coordinate is not NaN or infinity, create a path segment
            if (!std::isnan(y) && !std::isinf(y))
            {
                // Calculate the frequency of the bin
                auto binFreq = binNum * binWidth;
                // Normalize the bin's x-coordinate
                auto normalizedBinX = juce::mapFromLog10(binFreq, 20.f, 20000.f);
                // Calculate the actual x-coordinate in the FFT bounds
                int binX = std::floor(normalizedBinX * width);
                // Add a line segment to the path
                p.lineTo(binX, y);
            }
        }

        // Push the generated path to the path FIFO
        pathFifo.push(p);
    }

    // Function to get the number of paths available in the FIFO
    int getNumPathsAvailable() const
    {
        return pathFifo.getNumAvailableForReading();
    }

    // Function to retrieve a path from the FIFO
    bool getPath(PathType& path)
    {
        return pathFifo.pull(path);
    }

private:
    // FIFO for storing generated paths
    FifoSpectrumAnalyzer<PathType> pathFifo;
};

//==============================================================================
// Struct definition for PathProducer
struct PathProducer
{
    // Constructor for PathProducer
    PathProducer(SingleChannelSampleFifo<MultiMeterAudioProcessor::BlockType>& scsf) :
        leftChannelFifo(&scsf)
    {
        // Initialize the FFT data generator and set the FFT order to 2048
        leftChannelFFTDataGenerator.changeOrder(FFTOrder::order2048);
        // Set the size of the mono buffer to match the FFT size
        monoBuffer.setSize(1, leftChannelFFTDataGenerator.getFFTSize());
    }

    // Function to process FFT data
    void process(juce::Rectangle<float> fftBounds, double sampleRate);

    // Function to get the path
    juce::Path getPath() { return leftChannelFFTPath; }

private:
    // Pointer to the single channel sample FIFO
    SingleChannelSampleFifo<MultiMeterAudioProcessor::BlockType>* leftChannelFifo;
    // Buffer for mono audio data
    juce::AudioBuffer<float> monoBuffer;
    // FFT data generator for the left channel
    FFTDataGenerator<std::vector<float>> leftChannelFFTDataGenerator;
    // Path generator for analyzer
    AnalyzerPathGenerator<juce::Path> pathProducer;
    // Path for the FFT of the left channel
    juce::Path leftChannelFFTPath;
};

//==============================================================================
// Class definition for ResponseCurveComponent
struct ResponseCurveComponent : juce::Component,
    juce::Timer
{
    // Constructor
    ResponseCurveComponent(MultiMeterAudioProcessor&);

    // Overrides the paint function to draw the component
    void paint(juce::Graphics&) override;

    // Overrides the paintOverChildren function to draw on top of the children
    void paintOverChildren(Graphics& g) override;

    // Overrides the timerCallback function to handle timer events
    void timerCallback() override;

    // Overrides the resized function to handle resizing of the component
    void resized() override;

private:
    // Reference to the audio processor
    MultiMeterAudioProcessor& audioProcessor;

    // Colors for left and right channels
    juce::Colour leftChannelColour { 0xff48bde8 };
    juce::Colour rightChannelColour { 0xffa0a0a0 };

    // Grid for spectrum analysis
    SpectrumGrid logGrid;

    // Chain for mono processing
    MonoChain monoChain;

    // Function to get the area to render
    juce::Rectangle<int> getRenderArea();

    // Function to get the area for analysis
    juce::Rectangle<int> getAnalysisArea();

    // Path producers for left and right channels
    PathProducer leftPathProducer, rightPathProducer;
};
