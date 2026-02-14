# Digital Piano Using DAC

A 3-note digital piano built with the STM32F051R8Tx microcontroller. Press buttons to play musical notes generated through a custom 4-bit digital-to-analog converter (DAC).

## Overview

This project demonstrates embedded audio synthesis using a binary-weighted resistor DAC. Timer-driven interrupts output a 32-sample sine wave lookup table to generate three different musical notes. The system reads button inputs and dynamically adjusts the timer frequency to produce the selected note in real-time.

### Features

- **Custom 4-bit DAC**: Binary-weighted resistor network converts digital outputs to analog audio
- **Three musical notes**: C4 (~262 Hz), E4 (~329 Hz), G4 (~391 Hz)
- **Real-time audio synthesis**: Timer interrupts generate waveforms at note frequencies
- **Simple interface**: Press a button, hear a note
- **Low-level drivers**: Direct register manipulation for DAC control and GPIO reading

## How to Use

1. **Press a button**: Each of the 3 buttons plays a different note
2. **Hold for continuous tone**: Notes play as long as the button is held
3. **Release to stop**: Audio stops when all buttons are released

**Controls**:
- **Button 1 (PB0)**: Play low note (C4)
- **Button 2 (PB1)**: Play middle note (E4)
- **Button 3 (PB2)**: Play high note (G4)

## Hardware Requirements

- STM32F051R8Tx microcontroller (STM32F0DISCOVERY board)
- 4 resistors: 500Ω, 1kΩ, 2kΩ, 4kΩ
- 3 push buttons (tactile switches)
- 3.5mm audio jack or headphones
- Breadboard and jumper wires

## Pin Configuration

### DAC Output (Port C)
- **PC0**: DAC bit 0 (LSB) → 500Ω resistor
- **PC1**: DAC bit 1 → 1kΩ resistor
- **PC2**: DAC bit 2 → 2kΩ resistor
- **PC3**: DAC bit 3 (MSB) → 4kΩ resistor

All resistors connect to a common summing node, which feeds the audio jack.

### Button Inputs (Port B)
- **PB0**: Button 1 (active-low, internal pull-up)
- **PB1**: Button 2 (active-low, internal pull-up)
- **PB2**: Button 3 (active-low, internal pull-up)

## Building from Source

### Prerequisites

1. [Visual Studio Code](https://code.visualstudio.com/)
2. [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html)
3. ARM GCC toolchain
4. ST-LINK programmer (included on STM32F0DISCOVERY board)

## Project Structure
```
Digital_Piano_Using_DAC/
├── Core/
│   ├── Src/
│   │   ├── DAC.c              # 4-bit DAC driver
│   │   ├── Piano.c            # Button input reading
│   │   ├── Sound.c            # Waveform generation
│   │   └── main.c             # Main loop and initialization
│   └── Inc/
│       ├── DAC.h
│       ├── Piano.h
│       └── Sound.h
└── README.md
```

## How It Works

### DAC Design
The 4-bit DAC uses a binary-weighted resistor network with a 1:2:4:8 ratio. Each GPIO pin drives a resistor, and the currents sum at the audio output node to create 16 discrete voltage levels (0-15).

### Sound Generation
- **Waveform**: 32-sample sine wave stored in a lookup table
- **Timer**: TIM3 fires interrupts at (note frequency × 32) Hz
- **ISR**: Each interrupt outputs the next sample from the lookup table to the DAC
- **Notes**: Auto-reload register (ARR) dynamically changed per note

### Note Frequencies
- **NOTE_LOW**: ARR = 118 → ~262 Hz (C4)
- **NOTE_MED**: ARR = 94 → ~329 Hz (E4)
- **NOTE_HIGH**: ARR = 79 → ~391 Hz (G4)

## Technologies Used

- **C**: Low-level embedded programming
- **STM32 HAL**: Hardware abstraction layer for peripheral control
- **STM32CubeMX**: Peripheral initialization code generation
- **ARM GCC**: Compiler toolchain

## Author

[dsalas560](https://github.com/dsalas560)

## Acknowledgments

- Built for embedded systems coursework
- Demonstrates DAC design, timer interrupts, and real-time audio synthesis
