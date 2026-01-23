## Project Overview

This project implements a seven-segment display device driver in C for the STM32F0DISCOVERY microcontroller. The system interfaces with a seven-segment display using GPIO, external interrupts, and the SysTick timer for switch debouncing.

The design supports user-controlled increment and decrement of displayed values through external buttons and extends to a four-digit display using time-multiplexing. A SN74HC595N 8-bit shift register is used with SPI communication to efficiently drive the display hardware.

This project demonstrates embedded device driver development, interrupt-driven input handling, software-based display multiplexing, and real-time timing control.
