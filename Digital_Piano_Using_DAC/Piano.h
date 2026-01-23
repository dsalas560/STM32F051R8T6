#ifndef __PIANO_H__
#define __PIANO_H__

#include <stdint.h>

/*
 * Piano driver for 3 digital inputs.
 * Uses PB0, PB1, PB2 as active-low keys.
 *
 * PBx = 1 → not pressed
 * PBx = 0 → pressed
 *
 * Returns:
 *   0 = no key
 *   1 = key 1
 *   2 = key 2
 *   3 = key 3
 */

void Piano_Init(void);
uint8_t Piano_In(void);

#endif
