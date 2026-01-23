#ifndef __DAC_H__
#define __DAC_H__

#include <stdint.h>

/*
 * Simple 4-bit software DAC driver using GPIOC pins PC0..PC3.
 *
 * Usage:
 *   1) Configure PC0..PC3 as push-pull outputs in CubeMX.
 *   2) Call DAC_Init() once at startup.
 *   3) Call DAC_Out(value) with value in range [0..15].
 *
 * Hardware:
 *   PC0..PC3 drive a 4-bit binary-weighted resistor ladder.
 *   The summed node of the resistors is the DAC output (Vout),
 *   which goes to the tip of the 3.5mm jack. Ground goes to sleeve.
 */

#define DAC_MAX_VALUE   15u

void DAC_Init(void);
void DAC_Out(uint8_t value);

#endif /* __DAC_H__ */
