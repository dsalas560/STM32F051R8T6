## Project Overview

This project implements a simple digital sound generation system using the STM32F0DISCOVERY microcontroller. A custom digital-to-analog converter (DAC) is constructed using a binary-weighted resistor network to generate analog waveforms from digital output values.

Low-level device drivers are developed in C to control the DAC, read piano key inputs, and generate sound output. Timer-driven interrupts are used to periodically update the DAC, allowing stored waveform data to be converted into audible tones. Three piano keys are supported, each producing a distinct musical note while pressed.

This project demonstrates embedded DAC design, device driver abstraction, timer-based waveform generation, and real-time audio signal synthesis in an embedded system.

