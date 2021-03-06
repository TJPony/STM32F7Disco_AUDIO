/*****************************************************************************
文件名:  timer.h
描述:    该文件为timer.c的头文件 时间长度宏定义 函数声明
作者:    马小龙
版本:    V1.0
时间:    2019年7月2日
修改记录:

*****************************************************************************/

#ifndef _TIMER_H
#define _TIMER_H
#include "sys.h"

#define TIMER_FUNC_NUM  20

#define TIME_1S        100
#define TIME_5S        500
#define TIME_500MS     50
#define TIME_250MS     25
#define TIME_100MS     10

typedef void(*ProcessTimer_FUNC)(void);

typedef struct timer_func_desc {
	u8 *name;
	ProcessTimer_FUNC func;
}timer_func_desc, *p_timer_func_desc;

s8 register_timer_func(u8 *name, ProcessTimer_FUNC func);
void ProcessTimer_Init(u16 arr,u16 psc);

#endif

