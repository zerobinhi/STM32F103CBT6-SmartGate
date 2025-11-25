#ifndef OLED_H_
#define OLED_H_

#include "oled_font.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define OLED_I2C_ADDR (0x3C << 1) // 默认 0x78

#define OLED_WIDTH 128
#define OLED_HEIGHT 64

extern I2C_HandleTypeDef hi2c1;

/* 初始化与控制 */
HAL_StatusTypeDef OLED_Init(); // 初始化寄存器

/* 显存操作 */
HAL_StatusTypeDef OLED_Refresh(); // 全屏刷新：按页发送
void OLED_Clear(uint8_t color);    // 清屏或填充
HAL_StatusTypeDef OLED_SetContrast(uint8_t contrast);
HAL_StatusTypeDef OLED_InvertDisplay(bool invert);

/* 基本绘制（写入显存） */
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t color);
void OLED_DrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color);
void OLED_DrawRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color);
void OLED_FillRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color);

/* 文本/数字/小数/中文/位图（写入显存） */
void OLED_ShowChar(uint8_t x, uint8_t y, char chr, uint8_t size, uint8_t color);
void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size, uint8_t color);
void OLED_ShowNum(uint8_t x, uint8_t y, int32_t num, uint8_t len, uint8_t size, uint8_t color);
void OLED_ShowDecimal(uint8_t x, uint8_t y, float val, uint8_t z_len, uint8_t f_len, uint8_t size, uint8_t color);
void OLED_ShowChinese(uint8_t x, uint8_t y, uint8_t no, uint8_t color);
void OLED_DrawBitmap(uint8_t x, uint8_t y, const uint8_t *bmp, uint8_t w, uint8_t h, uint8_t color);

#endif /* OLED_H_ */