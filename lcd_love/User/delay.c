/*
 * @Author: wushengran
 * @Date: 2024-05-24 15:40:21
 * @Description: 
 * 
 * Copyright (c) 2024 by atguigu, All Rights Reserved. 
 */
#include "delay.h"

// 延时函数，微秒作为单位，利用系统嘀嗒定时器，72MHz，一次嘀嗒 1/72 us
void Delay_us(uint16_t us)
{
	// 1. 装载一个计数值，72 * us
	SysTick->LOAD = 72 * us;

	// 2. 配置，使用系统时钟（1），计数结束不产生中断（0），使能定时器（1）
	SysTick->CTRL = 0x05;

	// 3. 等待计数值变为0，判断CTRL标志位COUNTFLAG是否为1
	while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG) == 0)
	{}
	
	// 4. 关闭定时器
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE;
}

void Delay_ms(uint16_t ms)
{
	while (ms--)
	{
		Delay_us(1000);
	}
}

void Delay_s(uint16_t s)
{
	while (s--)
	{
		Delay_ms(1000);
	}
}
