# Seven Segment Display Driver

A 4-digit seven-segment display driver using the STM32F051R8Tx microcontroller, 74HC595 shift register, and SPI communication. Displays numbers 0-9999 with increment/decrement button controls.

## Overview

This project demonstrates how to interface a 4-digit seven-segment display with a microcontroller using time-multiplexing and serial-to-parallel conversion. The 74HC595 shift register reduces the number of GPIO pins needed from 12 (8 segments × 4 digits) to just 3 SPI pins plus 4 digit select lines.

### Features

- **4-digit display**: Shows numbers from 0000 to 9999
- **SPI communication**: Efficient serial data transfer to shift register
- **Time-multiplexing**: Refreshes all 4 digits at ~125 Hz (imperceptible flicker)
- **Button controls**: Increment and decrement buttons with debouncing
- **Common-anode display**: Inverted logic (LOW = segment ON)
- **Shift register control**: 74HC595N 8-bit serial-in, parallel-out

## How to Use

1. **Power on**: Display shows 0000
2. **Increment**: Press button on PA1 to count up (wraps at 9999 → 0)
3. **Decrement**: Press button on PA2 to count down (wraps at 0 → 9999)

**Controls**:
- **Button 1 (PA1)**: Increment number
- **Button 2 (PA2)**: Decrement number

## Hardware Requirements

- STM32F051R8Tx microcontroller (STM32F0DISCOVERY board)
- 74HC595N 8-bit shift register (SN74HC595N)
- 4-digit 7-segment common-anode display (LTC-5650G)
- 2 tactile push buttons
- resitors
- Breadboard and jumper wires

## Pin Configuration

### SPI1 (Shift Register Communication)
- **PA5**: SPI1_SCK → 74HC595 SRCLK (pin 11, shift clock)
- **PA7**: SPI1_MOSI → 74HC595 SER (pin 14, serial data input)

### Shift Register Control
- **PA4**: RCLK/Latch (pin 12, storage register clock)

### 74HC595 Other Connections
- **Pin 10 (SRCLR#)**: Connected to VCC (no reset)
- **Pin 13 (OE#)**: Connected to GND (always enabled)
- **VCC**: 3.3V
- **GND**: Ground


### Digit Select Lines (Common Anode Control)
- **PB8**: D1 enable (thousands digit)
- **PB9**: D2 enable (hundreds digit)
- **PB10**: D3 enable (tens digit)
- **PB11**: D4 enable (ones digit)

### Button Inputs
- **PA1**: Increment button (EXTI, falling edge, internal pull-up)
- **PA2**: Decrement button (EXTI, falling edge, internal pull-up)


## Project Structure
```
Seven_Seg_Display_Driver/
├── Core/
│   ├── Inc/
│   │   ├── SSEG.h             # Seven segment display driver header
│   │   └── main.h             # Main program header
│   └── Src/
│       ├── SSEG.c             # Display driver (SPI + multiplexing)
│       ├── main.c             # Main loop and button interrupts
│       └── [HAL files]        # STM32 HAL support files
└── README.md
```



## Author

[dsalas560](https://github.com/dsalas560)

## Acknowledgments

- Lab adapted from Dr. John Faller, California State University, Fullerton
- Demonstrates SPI communication, shift register interfacing, and time-multiplexed displays
- Based on the 74HC595 shift register datasheet from Texas Instruments
