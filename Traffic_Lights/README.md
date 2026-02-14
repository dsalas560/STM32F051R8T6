# Traffic Light System with Pedestrian Crosswalk

A finite state machine-based traffic control system for a two-way intersection with pedestrian crosswalk support, built on the STM32F051R8Tx microcontroller.

## Overview

This project implements a realistic traffic intersection controller with two one-way streets (North and East) and a pedestrian crosswalk. The system uses a finite state machine (FSM) stored in ROM to manage traffic lights, car sensors, and pedestrian walk requests safely and efficiently.

### Features

- **Two-way traffic control**: Independent North and East traffic light management
- **Pedestrian crosswalk**: Walk/Don't Walk lights with realistic walk sequences
- **Car sensors**: Button inputs detect waiting vehicles at the intersection
- **Walk button**: 2-second press-and-hold to request pedestrian crossing
- **Safety-first design**: Prevents car collisions and pedestrian conflicts
- **Shift register interface**: SN74HC595N reduces pin usage for traffic and crosswalk lights
- **LCD display**: Shows current system state and intersection status

## How It Works

### Traffic Light Behavior
- Only one direction has a green light at a time (prevents collisions)
- Yellow light transition period before red
- Fair cycling through all requests (cars and pedestrians)

### Pedestrian Crosswalk
1. **Walk**: Green LED - pedestrians can cross
2. **Hurry up**: Flashing red LED - finish crossing
3. **Don't walk**: Solid red LED - do not enter intersection

### Walk Request
- Press and hold the walk button for **at least 2 seconds**
- Release button - walk request is remembered
- System will service the request when safe

## Hardware Requirements

- STM32F051R8Tx microcontroller (STM32F0DISCOVERY board)
- SN74HC595N shift register (traffic light control)
- 16x2 LCD display (RGB backlight, black on RGB)
- 3 tactile switches (North sensor, East sensor, Walk button)
- 6 LEDs for traffic lights (2 red, 2 yellow, 2 green)
- 2 LEDs for crosswalk (1 green walk, 1 red don't walk)
- Resistors, breadboard, jumper wires

## Pin Configuration

### Shift Register (SN74HC595N)
Shift register outputs connect to 6 traffic LEDs + 2 crosswalk LEDs.

### Input Sensors
- **PA0**: North car sensor (button, active-low with pull-up)
- **PA1**: East car sensor (button, active-low with pull-up)
- **PA2**: Walk button (button, active-low with pull-up)

### LCD Display (16x2)
- **PA8**: RS (register select)
- **PA9**: E (enable)
- **PC0-PC3**: D4-D7 (4-bit data mode)

## Finite State Machine Design

The FSM uses a linked data structure stored in ROM with:
- **15-30 states** representing different traffic/crosswalk combinations
- **3 inputs** (North sensor, East sensor, Walk button) = 8 possible transitions per state
- **State outputs**: Traffic light patterns and crosswalk light patterns
- **State timing**: Configurable delays for each state

Example states:
- `goN`: North green, East red, Don't walk
- `waitN`: North yellow, East red, Don't walk
- `goE`: North red, East green, Don't walk
- `walk`: North red, East red, Walk (green)
- `hurry`: North red, East red, Flashing don't walk (red)

## Building from Source

### Prerequisites

1. [Visual Studio Code](https://code.visualstudio.com/)
2. [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html)
3. ARM GCC toolchain
4. ST-LINK programmer (included on STM32F0DISCOVERY board)


## Project Structure
```
Traffic_Lights/
├── Core/
│   ├── Inc/
│   │   ├── LCD.h              # LCD driver header
│   │   └── main.h             # Main program header
│   └── Src/
│       ├── LCD.c              # 16x2 LCD driver (4-bit mode)
│       ├── main.c             # FSM implementation and main loop
│       └── [HAL files]        # STM32 HAL support files
└── README.md
```

## Technologies Used

- **C**: Low-level embedded programming
- **STM32 HAL**: Hardware abstraction layer
- **STM32CubeMX**: Peripheral initialization
- **ARM GCC**: Compiler toolchain
- **Finite State Machines**: Control logic design

## System Requirements

### Safety Requirements
- Traffic lights must prevent collisions (never two greens simultaneously)
- Pedestrians must not be allowed to walk when traffic has green light
- All traffic lights must be red during pedestrian walk phase

### Timing Requirements
- State transitions fast enough for testing (~2-5 seconds per state)
- Walk button requires 2-second press-and-hold
- Flashing "don't walk" gives pedestrians time to clear intersection

### Fairness Requirements
- System cycles through all pending requests
- No request should be starved indefinitely

## Author

[dsalas560](https://github.com/dsalas560)

## Acknowledgments

- Based on coursework from "Embedded Systems - Shape the World"
- Demonstrates FSM design, shift register interfacing, and real-time embedded control
- Traffic intersection design inspired by standard civil engineering practices
