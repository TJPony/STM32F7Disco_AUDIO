/*****************************************************************************
文件名:  sys.h
描述:    该文件为sys.c的头文件 DEBUG调试宏定义 ，变量类型定义 ，位带操作定义 ，函数声明
作者:    马小龙
版本:    V1.0
时间:    2018年11月1日
修改记录:

*****************************************************************************/

#ifndef _SYS_H
#define _SYS_H

#include "stm32f7xx.h"
#include "core_cm7.h"

//#define DEBUG  1            //需要调试时开启该宏

#ifdef DEBUG
	#define DEBUG_LOG(fmt,...)  printf(fmt,__VA_ARGS__)
#else
	#define DEBUG_LOG(fmt,...)  
#endif

//定义一些常用的数据类型短关键字 
typedef int32_t  s32;
typedef int16_t s16;
typedef int8_t  s8;

typedef const int32_t sc32;  
typedef const int16_t sc16;  
typedef const int8_t sc8;  

typedef __IO int32_t  vs32;
typedef __IO int16_t  vs16;
typedef __IO int8_t   vs8;

typedef __I int32_t vsc32;  
typedef __I int16_t vsc16; 
typedef __I int8_t vsc8;   

typedef uint32_t  u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef const uint32_t uc32;  
typedef const uint16_t uc16;  
typedef const uint8_t uc8; 

typedef __IO uint32_t  vu32;
typedef __IO uint16_t vu16;
typedef __IO uint8_t  vu8;

typedef __I uint32_t vuc32;  
typedef __I uint16_t vuc16; 
typedef __I uint8_t vuc8;  

#define ON	1
#define OFF	0
#define Write_Through() (*(__IO uint32_t*)0XE000EF9C=1UL<<2)//Cache透写模式

typedef enum{FALSE = 0,TRUE} bool; 

void Cache_Enable(void);                                    //使能STM32F7的L1-Cahce
void Stm32_Clock_Init(u32 plln,u32 pllm,u32 pllp,u32 pllq); //配置系统时钟
u8 Get_ICacheSta(void);
u8 Get_DCacheSta(void);
//以下为汇编函数
void WFI_SET(void);		
void INTX_DISABLE(void);
void INTX_ENABLE(void);	
void MSR_MSP(u32 addr);	

#endif
