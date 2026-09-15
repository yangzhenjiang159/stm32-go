/*
 * @Author: wushengran
 * @Date: 2024-06-17 14:25:00
 * @Description: 
 * 
 * Copyright (c) 2024 by atguigu, All Rights Reserved. 
 */
#ifndef __LCD_H
#define __LCD_H

#include "fsmc.h"
#include "delay.h"
#include <math.h>

// 宏定义
// 定义写命令和写数据对应的地址指针
#define SRAM_BANK1_4 0x6C000000
#define LCD_ADDR_CMD (uint16_t *)SRAM_BANK1_4
#define LCD_ADDR_DATA (uint16_t *)(SRAM_BANK1_4 + (1 << 11))
// #define LCD_ADDR_DATA (uint16_t *)(0x6cfffffe)

// 定义显示屏的宽和高
#define LCD_W 320
#define LCD_H 480

/* 常见颜色 */
#define WHITE 0xFFFF
#define BLACK 0x0000
#define BLUE 0x001F
#define BRED 0XF81F
#define GRED 0XFFE0
#define GBLUE 0X07FF
#define RED 0xF800
#define MAGENTA 0xF81F
#define GREEN 0x07E0
#define CYAN 0x7FFF
#define YELLOW 0xFFE0
#define BROWN 0XBC40 // 棕色
#define BRRED 0XFC07 // 棕红色
#define GRAY 0X8430  // 灰色

// 1. 基本控制操作
// 1.1 初始化
void LCD_Init(void);

// 1.1.1 复位
void LCD_Reset(void);

// 1.1.2 开关背光
void LCD_BGOn(void);
void LCD_BGOff(void);

// 1.1.3 初始化LCD寄存器
void LCD_RegConfig(void);

// 1.2 写命令（发出一个指令）
void LCD_WriteCmd(uint16_t cmd);

// 1.3 写数据（发出一个数据）
void LCD_WriteData(uint16_t data);

// 1.4 读数据
uint16_t LCD_ReadData(void);

// 2. 具体的命令操作

// 2.1 返回ID信息
uint32_t LCD_ReadID(void);

// 2.2 清屏（设置全屏背景颜色）
void LCD_ClearAll(uint16_t color);
// 设置区域范围，给定起始点的坐标（行列号），以及区域的宽和高
void LCD_SetArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

void LCD_WriteAsciiChar(uint16_t x, uint16_t y, uint16_t height, uint8_t c, uint16_t fColor, uint16_t bColor);

void LCD_WriteAsciiString(uint16_t x, uint16_t y, uint16_t height, uint8_t * str, uint16_t fColor, uint16_t bColor);

void LCD_WriteChineseChar(uint16_t x, uint16_t y, uint16_t height, uint8_t index, uint16_t fColor, uint16_t bColor);

void LCD_DisplayAtguiguLogo(uint16_t x, uint16_t y);

void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t w, uint16_t color);

void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t w, uint16_t color);

void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t w, uint16_t color);

void LCD_DrawCircle(uint16_t xCenter, uint16_t yCenter, uint16_t r, uint16_t w, uint16_t color);
void LCD_DrawCircle_Pro(uint16_t xCenter, uint16_t yCenter, uint16_t r, uint16_t w, uint16_t color);

void LCD_DrawFilledCircle(uint16_t xCenter, uint16_t yCenter, uint16_t r, uint16_t w, uint16_t bColor, uint16_t fColor);
void LCD_DrawFilledCircle_Pro(uint16_t xCenter, uint16_t yCenter, uint16_t r, uint16_t w, uint16_t bColor, uint16_t fColor);
#endif

