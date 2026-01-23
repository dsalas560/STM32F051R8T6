## Project Overview

This project implements a real-time traffic intersection controller using the STM32F0DISCOVERY board. The system is developed entirely in C and models a finite state machine to manage traffic lights, vehicle sensors, and pedestrian crosswalk signals.

The controller uses the SysTick timer for fixed-time delays and button debouncing, supports vehicle detection on two intersecting roads, and enforces safe traffic behavior. An extended phase adds pedestrian crossing functionality, including walk/don’t-walk signals with timed and flashing states to ensure pedestrian safety.

This project demonstrates real-time embedded system design, synchronization, and state-based control logic.

