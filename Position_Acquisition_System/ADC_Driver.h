#ifndef __ADC_DRIVER_H__
#define __ADC_DRIVER_H__

#include "stm32f0xx_hal.h"
#include <stdint.h>

/* Call after MX_ADC_Init() */
void ADC_DriverInit(void);

/* Do one blocking conversion on PA0 (ADC1_IN0) */
uint16_t ADC_In(void);

#endif
