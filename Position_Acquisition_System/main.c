/* Lab 3 Phase 2 – Step 4: 10 Hz timer, mailbox, LCD display */

#include "main.h"
#include "stm32f0xx_hal.h"
#include <stdint.h>

#include "LCD.h"
#include "ADC_Driver.h"

/* Global ADC handle (CubeMX) */
ADC_HandleTypeDef hadc;

/* Prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC_Init(void);

/* -------- Mailbox & heartbeat -------- */
volatile uint16_t ADC_Mailbox = 0;
volatile uint8_t  ADC_MailboxFlag = 0;

/* Convert ADC sample, ADC is 12 bits 2^12 = 4096... 0 to 4095 */
static uint32_t Position_FromSample(uint16_t sample){
  /* linear map: 0 → 0.000 cm, 4095 → ~2.000 cm */
  return ((uint32_t)sample * 2000u + 2047u) / 4095u;
}

/* Called by HAL every 1 ms from SysTick_Handler */
void HAL_SYSTICK_Callback(void){
  static uint32_t msCount = 0;
  msCount++;
  if (msCount >= 100) {          // 100 ms -> 10 Hz
    msCount = 0;

    uint16_t s = ADC_In();       // take one ADC sample
    ADC_Mailbox = s;
    ADC_MailboxFlag = 1;

    /* heartbeat LED on PC8 */
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_8);
  }
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_ADC_Init();

  ADC_DriverInit();

  HAL_Delay(100);
  LCD_Init();
  LCD_Clear();
 LCD_OutString("Pos: 0.000 cm");

  //uint32_t lastPos = 0xFFFFFFFFu;

while (1) {
  /* 1) Wait for the mailbox flag to be set */
  while (!ADC_MailboxFlag) {
    // busy-wait; nothing else in foreground
  }

  /* 2 & 3) Read ADC sample from mailbox and clear flag (atomically) */
  uint16_t sample;
  __disable_irq();
  sample = ADC_Mailbox;
  ADC_MailboxFlag = 0;
  __enable_irq();

  /* 4) Convert ADC sample to fixed-point position (0.001 cm units) */
  uint32_t pos = Position_FromSample(sample);  // 0..2000 -> 0.000–2.000 cm

  /* 5) Output fixed-point number on LCD with units of cm */
  LCD_Clear();
  LCD_OutString("Pos: ");
  LCD_OutUFix(pos);       // prints X.XXX
  LCD_OutString(" cm");

    
    /* no blocking delays needed; loop just waits for new samples */
  }
}

/* ================= Clock & peripheral init (same as CubeMX) =============== */

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSI14;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI14CalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { Error_Handler(); }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
    Error_Handler();
  }
}

static void MX_ADC_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  hadc.Instance = ADC1;
  hadc.Init.ClockPrescaler        = ADC_CLOCK_ASYNC_DIV1;
  hadc.Init.Resolution            = ADC_RESOLUTION_12B;
  hadc.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
  hadc.Init.ScanConvMode          = ADC_SCAN_DIRECTION_FORWARD;
  hadc.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
  hadc.Init.LowPowerAutoWait      = DISABLE;
  hadc.Init.LowPowerAutoPowerOff  = DISABLE;
  hadc.Init.ContinuousConvMode    = DISABLE;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
  hadc.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.DMAContinuousRequests = DISABLE;
  hadc.Init.Overrun               = ADC_OVR_DATA_PRESERVED;
  if (HAL_ADC_Init(&hadc) != HAL_OK) { Error_Handler(); }

  sConfig.Channel      = ADC_CHANNEL_0;           // PA0
  sConfig.Rank         = ADC_RANK_CHANNEL_NUMBER;
  sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK) { Error_Handler(); }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* LCD: PA8 (RS), PA9 (E) */
  GPIO_InitStruct.Pin   = GPIO_PIN_8 | GPIO_PIN_9;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* LCD data PC0..PC3 + heartbeat PC8 */
  GPIO_InitStruct.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 |
                          GPIO_PIN_3 | GPIO_PIN_8;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* default low */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8 | GPIO_PIN_9, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0 | GPIO_PIN_1 |
                           GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_8,
                    GPIO_PIN_RESET);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}
#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{ (void)file; (void)line; }
#endif
