#include "Piano.h"
#include "main.h"   // includes GPIO definitions

void Piano_Init(void)
{
    //PB0/PB1/PB2 as input with pull-ups, taken care of by CubeMx
}

uint8_t Piano_In(void)
{
    uint8_t result = 0;

    if ((GPIOB->IDR & GPIO_PIN_0) == 0) {
        result = 1;  // key 1 pressed
    }
    else if ((GPIOB->IDR & GPIO_PIN_1) == 0) {
        result = 2;  // key 2 pressed
    }
    else if ((GPIOB->IDR & GPIO_PIN_2) == 0) {
        result = 3;  // key 3 pressed
    }

    return result;
}
