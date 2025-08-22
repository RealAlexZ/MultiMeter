# MultiMeter

ChucKsplainer is a Web-based chatbot that teaches ChucK—a programming language for music creation—by helping students explore, understand, and complete ChucK code. This platform democratizes educational resources for both computer science and music education.

![multimeter-demo](https://github.com/RealAlexZ/MultiMeter/assets/97690118/ce64ecb6-801e-4e9d-8815-1f97655c272d)

Note: If you are using MacOS and the meters are not responding to the audio signals, try removing the return keyword in Fifo::getNumAvailableForReading() and Fifo::getAvailableSpace(). 


### General Metering
- Employs a high-performance FIFO (First In, First Out) buffer to handle audio data between DSP and GUI threads.
- Features comboboxes and sliders to personalize metering behavior.

### Level Meter
- Provides instantaneous visual feedback of audio signal levels with numeric value displays in decibels.
- Supports both Root Mean Squared (RMS) and peak readings.
- Enables user adjustment of the decay rate of meter ticks with multiple responsiveness options.
- Allows holding peak tick values for a specified duration to enhance the analysis of transient audio materials.

### FFT Spectrogram Analyzer
- Presents a high-resolution Fast Fourier Transform (FFT) spectrum with logarithmically scaled frequency bins, displaying the frequency content over time with a curve of all frequency components in the incoming signal and enabling in-depth spectral balance analysis.

### Histogram
- Visualizes the distribution of signal level dynamics over time.

### Correlation Meter
- Provides instantaneous and average correlation readings between left and right channels to help identify phase issues and ensure mono compatibility.

### Goniometer
- Converts L/R audio signals into Mid/Side representations that provide insights into the coherence of the stereo field distribution and phase differences between the left and right channels.

### Correlation Meter
- Provides real-time readings of the phase correlation between left and right audio channels, ranging from +1 (fully in-phase) to 0 (wide stereo) to -1 (out-of-phase), for identifying phase issues and ensuring mono compatibility.

### Dependencies
- **JUCE:** 6.1.2
