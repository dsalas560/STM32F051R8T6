#include "lcd.h"

/* --- PIN MAP (match wiring) --- */
#define LCD_RS_PORT   GPIOA
#define LCD_RS_PIN    GPIO_PIN_8
#define LCD_E_PORT    GPIOA
#define LCD_E_PIN     GPIO_PIN_9

#define LCD_D_PORT    GPIOC
/* D4..D7 on PC0..PC3 (lower nibble) */
#define LCD_D_SHIFT   0u
#define LCD_D_MASK    (0x0Fu << LCD_D_SHIFT)

/* --- tiny delays --- */
static inline void delay_us(uint32_t us){
  /* crude busy-wait ~ ok for blind-cycle */
 uint32_t loops_per_us = SystemCoreClock / 8000000U;  // ~= cycles/8
  if (loops_per_us == 0) loops_per_us = 1;

  while (us--) {
    for (volatile uint32_t i = 0; i < loops_per_us; ++i) {
      __NOP(); __NOP(); __NOP(); __NOP();
      __NOP(); __NOP(); __NOP(); __NOP();
    }
  }
}

static inline void LCD_E_Pulse(void){
  HAL_GPIO_WritePin(LCD_E_PORT, LCD_E_PIN, GPIO_PIN_SET);
  /* ~1–2 us pulse */
  delay_us(8);
  HAL_GPIO_WritePin(LCD_E_PORT, LCD_E_PIN, GPIO_PIN_RESET);
  delay_us(8);
}

/* Write a 4-bit nibble to PC0..PC3 */
static inline void LCD_Write4(uint8_t nibble){
  uint32_t odr = LCD_D_PORT->ODR;
  odr &= ~LCD_D_MASK;
  odr |= ((nibble & 0x0F) << LCD_D_SHIFT);
  LCD_D_PORT->ODR = odr;
  LCD_E_Pulse();
}

/* --- public funcs (unchanged prototypes) --- */
void LCD_OutCmd(uint8_t cmd){
  HAL_GPIO_WritePin(LCD_RS_PORT, LCD_RS_PIN, GPIO_PIN_RESET);
  LCD_Write4(cmd >> 4);
  LCD_Write4(cmd & 0x0F);
  HAL_Delay(2);  // blind-cycle: plenty of time
}

void LCD_OutChar(char data){
  HAL_GPIO_WritePin(LCD_RS_PORT, LCD_RS_PIN, GPIO_PIN_SET);
  LCD_Write4((uint8_t)data >> 4);
  LCD_Write4((uint8_t)data & 0x0F);
  HAL_Delay(1);
}

void LCD_Clear(void){
  LCD_OutCmd(0x01);   // clear display
  HAL_Delay(3);
}

void LCD_Init(void){
  HAL_Delay(40);  // power-on wait

  HAL_GPIO_WritePin(LCD_RS_PORT, LCD_RS_PIN, GPIO_PIN_RESET);

  /* 4-bit init ritual */
  LCD_Write4(0x03); HAL_Delay(5);
  LCD_Write4(0x03); HAL_Delay(5);
  LCD_Write4(0x03); HAL_Delay(1);
  LCD_Write4(0x02); HAL_Delay(1); // 4-bit

  LCD_OutCmd(0x28); // function set: 4-bit, 2-line, 5x8
  LCD_OutCmd(0x0C); // display ON, cursor OFF, blink OFF
  LCD_OutCmd(0x06); // entry mode: increment, no shift
  LCD_Clear();
}

void LCD_OutString(const char *s){
  while (*s) LCD_OutChar(*s++);
}
