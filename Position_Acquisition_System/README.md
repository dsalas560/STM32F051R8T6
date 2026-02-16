# ADC Position Measurement System

A real-time position measurement system using the STM32F051R8Tx's 12-bit ADC to read a slide potentiometer and display the position on a 16x2 LCD display with 0.001 cm resolution.

## Overview

This project demonstrates analog-to-digital conversion, interrupt-driven sampling, fixed-point arithmetic, and real-time data display. A slide potentiometer position (0-2 cm) is sampled at 10 Hz using a SysTick interrupt, converted to a fixed-point decimal value, and displayed on an LCD with millimeter precision.

### Features

- **12-bit ADC**: High-resolution analog-to-digital conversion (0-4095)
- **10 Hz sampling rate**: Timer interrupt-driven data acquisition
- **Mailbox communication**: Safe data transfer between ISR and main loop
- **Fixed-point display**: Shows position as X.XXX cm (0.001 cm resolution)
- **Real-time LCD output**: Continuous position updates on 16x2 display
- **Heartbeat LED**: Visual indicator of sampling activity (PC8)

## How It Works

1. **SysTick interrupt** fires every 100ms (10 Hz)
2. **ADC samples** the potentiometer voltage on PA0
3. **ISR stores** the sample in a mailbox and sets a flag
4. **Main loop** detects the flag, reads the mailbox, and clears the flag
5. **Conversion** maps ADC value (0-4095) to position (0.000-2.000 cm)
6. **LCD displays** the position with format "Pos: X.XXX cm"

## Hardware Requirements

- STM32F051R8Tx microcontroller (STM32F0DISCOVERY board)
- 16x2 LCD display (RGB backlight, black on RGB - Part# 485-398)
- 10kΩ slide potentiometer (Part# 652-PTA2432015DPB103)
- Breadboard and jumper wires
- Current-limiting resistors for LCD contrast adjustment

## Pin Configuration

### ADC Input
- **PA0**: ADC_IN0 (analog input from slide potentiometer wiper)

Connect potentiometer:
- One end → 3.3V
- Wiper (middle pin) → PA0
- Other end → GND

### LCD Display (16x2, 4-bit mode)
- **PA8**: RS (register select)
- **PA9**: E (enable)
- **PC0**: D4 (data bit 4)
- **PC1**: D5 (data bit 5)
- **PC2**: D6 (data bit 6)
- **PC3**: D7 (data bit 7)

### Status LED
- **PC8**: Heartbeat LED (toggles at 10 Hz during sampling)

## Project Structure
```
Position_Acquisition_System/
├── Core/
│   ├── Inc/
│   │   ├── ADC_Driver.h       # ADC driver header
│   │   ├── LCD.h              # LCD driver header
│   │   └── main.h             # Main program header
│   └── Src/
│       ├── ADC_Driver.c       # 12-bit ADC driver
│       ├── LCD.c              # 16x2 LCD driver (4-bit mode)
│       ├── main.c             # Mailbox system and sampling ISR
│       └── [HAL files]        # STM32 HAL support files
└── README.md
```

## Software Architecture

### Interrupt Service Routine (ISR)
**SysTick Interrupt** - fires every 1ms
- Counts to 100ms (10 Hz rate)
- Calls `ADC_In()` to take a sample
- Stores sample in `ADC_Mailbox`
- Sets `ADC_MailboxFlag`
- Toggles heartbeat LED

### Main Loop
1. **Wait** for mailbox flag to be set
2. **Atomically read** mailbox and clear flag (interrupts disabled)
3. **Convert** ADC sample to fixed-point position
4. **Display** on LCD: "Pos: X.XXX cm"
5. **Repeat**

### Mailbox Communication
Prevents race conditions between ISR and main loop:
```c
// ISR (background) - writes data
ADC_Mailbox = sample;
ADC_MailboxFlag = 1;

// Main loop (foreground) - reads data
__disable_irq();
sample = ADC_Mailbox;
ADC_MailboxFlag = 0;
__enable_irq();
```

## Author

[dsalas560](https://github.com/dsalas560)

## Acknowledgments

- Lab adapted from Dr. John Faller, California State University, Fullerton
- Demonstrates ADC sampling, interrupt-driven data acquisition, mailbox communication, and fixed-point arithmetic
- Based on principles from "Embedded Systems - Shape the World" textbook
