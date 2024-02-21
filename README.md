# MultiMeter

## Overview

MultiMeter, a cutting-edge AU/VST audio analyzer, caters to audio engineers, producers, and musicians who seek precision and versatility. Leveraging the JUCE framework, MultiMeter delivers a robust array of features for pristine real-time audio analysis, enhancing mixing, mastering, and sound design processes.


## User Interface

![multimeter-demo](https://github.com/RealAlexZ/MultiMeter/assets/97690118/ce64ecb6-801e-4e9d-8815-1f97655c272d)

Note: If you are using MacOS and the meters are not responding to the audio signals, try removing the return keyword in Fifo::getNumAvailableForReading() and Fifo::getAvailableSpace(). 

## Features

### Metering
- Provides instantaneous visual feedback of audio signal levels with support for both peak and RMS readings.
- Displays numeric values of signal levels in decibels, providing precise and quick reference.
- Implements a high-performance FIFO (First In, First Out) buffer to efficiently handle audio data between DSP and GUI threads.
- Allows users to adjust the decay rate of meter ticks with multiple responsiveness options.
- Includes the ability to hold peak values for a specified duration to enhance the analysis of transient audio material.
- Features a series of comboboxes and sliders to personalize metering behavior, including averager durations and various meter displays.

### Histogram
- Visualizes the distribution of signal level dynamics over time.

### Goniometer
- Converts L/R audio signals into Mid/Side representations that provide valuable insights into stereo field distribution and phase relationships.

### Correlation Meter
- Provides instantaneous and average correlation readings between left and right channels to help identify phase issues and ensure mono compatibility.

### Spectrogram Analyzer
- Boasts a high-resolution spectrogram that displays the frequency content over time across both left and right channels, enabling in-depth spectral balance analysis and identification of potential mix flaws.

## Dependencies
- **JUCE:** 6.1.2
