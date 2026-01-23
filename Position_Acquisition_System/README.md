## Project Overview

This project implements an LCD device driver and a position acquisition system using the STM32F0DISCOVERY microcontroller. All software is developed in C and runs on hardware consisting of a 16×2 character LCD and a slide potentiometer.

The system interfaces with an HD44780-based LCD using a custom driver that supports string output, unsigned decimal numbers, and fixed-point numerical display. Timing and synchronization are handled using software delays and the SysTick timer to ensure reliable LCD communication.

An analog-to-digital converter (ADC) is used to measure the position of the slide potentiometer, converting the analog signal into a fixed-point position value with millimeter-level resolution. Periodic interrupts establish real-time sampling, and measured values are displayed on the LCD in a human-readable format.

This project demonstrates embedded device driver development, ADC-based data acquisition, fixed-point arithmetic, and real-time system timing.

