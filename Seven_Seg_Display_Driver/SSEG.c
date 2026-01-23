#include "SSEG.h"

/* Common-cathode: driving PBx HIGH turns the segment ON (through your 1kΩ). */
/* Bit order in LUT: bit0=a, bit1=b, bit2=c, bit3=d, bit4=e, bit5=f, bit6=g, bit7=dp */
static const uint8_t digitLUT[10] = {
/*0*/ 0b00111111,
/*1*/ 0b00000110,
/*2*/ 0b01011011,
/*3*/ 0b01001111,
/*4*/ 0b01100110,
/*5*/ 0b01101101,
/*6*/ 0b01111101,
/*7*/ 0b00000111,
/*8*/ 0b01111111,
/*9*/ 0b01101111
};

/* Write the 8 segment lines PB0..PB7 according to 'bits' */
static inline void writeSegments(uint8_t bits)
{
    const uint16_t ALL =
        GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|
        GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;

    /* Clear all, then set the required ones */
    HAL_GPIO_WritePin(GPIOB, ALL, GPIO_PIN_RESET);

    if (bits & (1u<<0)) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); // a
    if (bits & (1u<<1)) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET); // b
    if (bits & (1u<<2)) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET); // c
    if (bits & (1u<<3)) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET); // d
    if (bits & (1u<<4)) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET); // e
    if (bits & (1u<<5)) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET); // f
    if (bits & (1u<<6)) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET); // g
    if (bits & (1u<<7)) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET); // dp
}

void SSEG_Init(void)
{
    /* GPIOB was already configured by MX_GPIO_Init().
       Just show 0 at startup. */
    SSEG_Out(0);
}

void SSEG_Out(uint8_t num)
{
    if (num > 9) num = 0;
    uint8_t bits = digitLUT[num];

    /* Uncomment to light the decimal point on digit 2: */
    // bits |= (1u << 7);

    writeSegments(bits);
}
