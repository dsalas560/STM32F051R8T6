#include "LCD.h"

/* ===== Pin map (match CubeMX) ===== */
#define LCD_RS_PORT   GPIOA
#define LCD_RS_PIN    GPIO_PIN_8
#define LCD_E_PORT    GPIOA
#define LCD_E_PIN     GPIO_PIN_9

#define LCD_D_PORT    GPIOC
/* D4..D7 on PC0..PC3 */
#define LCD_D_SHIFT   0u
#define LCD_D_MASK    (0x0Fu << LCD_D_SHIFT)

/* ===== Tiny delay helpers (blind-cycle timing) ===== */
/* Small delay used for Enable pulses */
static inline void delay_us(uint32_t us){
  uint32_t loops_per_us = SystemCoreClock / 8000000U;
  if (loops_per_us == 0) loops_per_us = 1;
  while (us--) {
    for (volatile uint32_t i = 0; i < loops_per_us; ++i) {
      __NOP(); __NOP(); __NOP(); __NOP();
      __NOP(); __NOP(); __NOP(); __NOP();
    }
  }
}
/* Pulse E to latch a 4-bit nibble */
static inline void LCD_E_Pulse(void){
  HAL_GPIO_WritePin(LCD_E_PORT, LCD_E_PIN, GPIO_PIN_SET);
  delay_us(8);
  HAL_GPIO_WritePin(LCD_E_PORT, LCD_E_PIN, GPIO_PIN_RESET);
  delay_us(8);
}

/* Write a 4-bit nibble to D4..D7 */
static inline void LCD_Write4(uint8_t nibble){
  uint32_t odr = LCD_D_PORT->ODR;
  odr &= ~LCD_D_MASK;
  odr |= ((nibble & 0x0F) << LCD_D_SHIFT);
  LCD_D_PORT->ODR = odr;
  LCD_E_Pulse();
}

/* ===== Low-level I/O ===== */
/* Send an 8-bit LCD command */
void LCD_OutCmd(uint8_t cmd){
  HAL_GPIO_WritePin(LCD_RS_PORT, LCD_RS_PIN, GPIO_PIN_RESET);
  LCD_Write4(cmd >> 4);
  LCD_Write4(cmd & 0x0F);
  HAL_Delay(2);
}

/* Send one character to LCD */
void LCD_OutChar(char data){
  HAL_GPIO_WritePin(LCD_RS_PORT, LCD_RS_PIN, GPIO_PIN_SET);
  LCD_Write4(((uint8_t)data) >> 4);
  LCD_Write4(((uint8_t)data) & 0x0F);
  HAL_Delay(1);
}

/* ===== High-level API ===== */
/* Clear the display */
void LCD_Clear(void){
  LCD_OutCmd(0x01);      // clear display
  HAL_Delay(3);
}

void LCD_Init(void){
  HAL_Delay(40);                 // power-on wait
  HAL_GPIO_WritePin(LCD_RS_PORT, LCD_RS_PIN, GPIO_PIN_RESET);

  /* 4-bit init sequence (HD44780) */
  LCD_Write4(0x03); HAL_Delay(5);
  LCD_Write4(0x03); HAL_Delay(5);
  LCD_Write4(0x03); HAL_Delay(1);
  LCD_Write4(0x02); HAL_Delay(1);  // set 4-bit mode

  LCD_OutCmd(0x28); // function set: 4-bit, 2-line, 5x8 font
  LCD_OutCmd(0x0C); // display ON, cursor OFF, blink OFF
  LCD_OutCmd(0x06); // entry mode: increment, no shift
  LCD_Clear();
}

void LCD_OutString(const char *s){
  while (*s) { LCD_OutChar(*s++); }
}

/* ---- Lab 3 additions ---- */
void LCD_OutUDec(uint32_t n){
  char buf[10];
  int i = 0;
  if (n == 0){ LCD_OutChar('0'); return; }
  while (n > 0 && i < (int)sizeof(buf)){
    buf[i++] = (char)('0' + (n % 10u));
    n /= 10u;
  }
  while (--i >= 0) LCD_OutChar(buf[i]);
}

void LCD_OutUFix(uint32_t number){
  if (number >= 10000u){
    LCD_OutString("*.***");
    return;
  }
  uint32_t ip = number / 1000u;    // 0..9
  uint32_t fp = number % 1000u;    // 0..999
  LCD_OutChar((char)('0' + (char)ip));
  LCD_OutChar('.');
  LCD_OutChar((char)('0' + (char)((fp/100u)%10u)));
  LCD_OutChar((char)('0' + (char)((fp/10u)%10u)));
  LCD_OutChar((char)('0' + (char)( fp%10u )));
}
