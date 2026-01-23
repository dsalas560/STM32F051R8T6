#include "ADC_Driver.h"

/* Use the global handle CubeMX created */
extern ADC_HandleTypeDef hadc;

void ADC_DriverInit(void){
 
  HAL_ADCEx_Calibration_Start(&hadc);
}
/*
 * ADC_In
 * ---------
 * Starts one single ADC conversion on channel PA0 (ADC_IN0),
 * waits until the conversion is finished,
 * reads the 12-bit result (0–4095),
 * and stops the ADC.
 */
uint16_t ADC_In(void){
  HAL_ADC_Start(&hadc);
  HAL_ADC_PollForConversion(&hadc, HAL_MAX_DELAY);
  uint16_t val = (uint16_t)HAL_ADC_GetValue(&hadc); // 0..4095
  HAL_ADC_Stop(&hadc);
  return val;
}
