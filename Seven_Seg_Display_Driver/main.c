/* USER CODE BEGIN Header */
/* ... keep the CubeMX header ... */
/* USER CODE END Header */
#include "main.h"
#include "SSEG.h"   // <-- add this

/* Private variables ---------------------------------------------------------*/
volatile uint8_t g_num = 0;   // current digit 0..9

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);

/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();

  SSEG_Init();        // show 0 on the 2nd digit (T4->GND)

  while (1) {
    __WFI();          // sleep; wake on button interrupt
  }
}

/* ----- Button ISR callback (PA1 = INC, PA2 = DEC, active-low) ----- */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_1 || GPIO_Pin == GPIO_PIN_2) {
    HAL_Delay(20);    // debounce ~20 ms

    GPIO_TypeDef *port = GPIOA;
    uint16_t pin = (GPIO_Pin == GPIO_PIN_1) ? GPIO_PIN_1 : GPIO_PIN_2;

    if (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET) {  // still pressed?
      if (GPIO_Pin == GPIO_PIN_1) {       // PA1 -> increment
        g_num = (g_num + 1) % 10;
      } else {                            // PA2 -> decrement
        g_num = (g_num == 0) ? 9 : (g_num - 1);
      }
      SSEG_Out(g_num);                    // update segments
    }
  }
}

/* --- keep the CubeMX-generated SystemClock_Config() and MX_GPIO_Init() --- */
/* --- keep stm32f0xx_it.c calling HAL_GPIO_EXTI_IRQHandler for EXTI0_1 & EXTI2_3 --- */
