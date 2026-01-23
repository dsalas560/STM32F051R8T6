#include "DAC.h"
#include "main.h"   // gives GPIOC, HAL stuff

/*
 * Initialize the DAC GPIO outputs.
 * Assumes PC0..PC3 are already configured as outputs by MX_GPIO_Init().
 */
void DAC_Init(void)
{
    // Start with output value 0
    GPIOC->ODR &= ~(0x0F);   // clear bits PC0..PC3
}

/*
 * Output a 4-bit value (0..15) on PC0..PC3.
 */
void DAC_Out(uint8_t value)
{
    value &= 0x0F;                   // only lower 4 bits used

    // Clear PC0..PC3
    GPIOC->ODR &= ~(0x0F);

    // Write new value to PC0..PC3
    GPIOC->ODR |= value;
}
