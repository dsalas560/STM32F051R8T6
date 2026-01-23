#ifndef __LCD_H__
#define __LCD_H__

#include "stm32f0xx_hal.h"
#include <stdint.h>

/* Public API */
void LCD_Init(void);
void LCD_Clear(void);
void LCD_OutCmd(uint8_t cmd);
void LCD_OutChar(char data);
void LCD_OutString(const char *s);
void LCD_OutUDec(uint32_t n);
void LCD_OutUFix(uint32_t number);

#endif /* __LCD_H__ */
