#ifndef SSEG_H
#define SSEG_H

#include "main.h"
#include <stdint.h>

/*
 * SSEG_Init
 * Initializes the single-digit seven-segment output (PB0..PB7).
 * Call once after MX_GPIO_Init().
 */
void SSEG_Init(void);

/*
 * SSEG_Out
 * Displays a decimal digit [0..9] on the enabled digit (your D2 with T4→GND).
 */
void SSEG_Out(uint8_t num);

#endif
