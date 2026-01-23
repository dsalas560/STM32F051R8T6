#ifndef __LCD_H
#define __LCD_H
#include "main.h"

// Public functions
void LCD_Init(void);
void LCD_Clear(void);
void LCD_OutCmd(uint8_t cmd);
void LCD_OutChar(char data);
void LCD_OutString(const char *s);
void LCD_SetCursor(uint8_t row, uint8_t col);   // row: 0 or 1, col: 0..15
#endif

