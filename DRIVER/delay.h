/*****************************************************************************
文件名:  delay.h
描述:    该文件为delay.c的头文件 函数声明
作者:    马小龙
版本:    V1.0
时间:    2018年11月1日
修改记录:

*****************************************************************************/

#ifndef _DELAY_H
#define _DELAY_H
#include <sys.h>	  

void delay_init(u8 SYSCLK);
void delay_ms(u16 nms);
void delay_us(u32 nus);
#endif
