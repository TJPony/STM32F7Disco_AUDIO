/*****************************************************************************
文件名:  sdram.h
描述:    该文件为MT48LC4M32B2B5-6A底层驱动头文件
作者:    马小龙
版本:    V1.0
时间:    2019年8月1日
修改记录:

*****************************************************************************/
#ifndef _SDRAM_H
#define _SDRAM_H
#include "sys.h"

#define SDRAM_DEVICE_ADDR    ((uint32_t)(0XC0000000)) //SDRAM开始地址
#define SDRAM_DEVICE_SIZE  	 ((uint32_t)(0x1000000))  //SDRAM尺寸16MB

//SDRAM配置参数
#define SDRAM_MODEREG_BURST_LENGTH_1             ((u16)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2             ((u16)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4             ((u16)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8             ((u16)0x0004)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((u16)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((u16)0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_2              ((u16)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((u16)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((u16)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((u16)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((u16)0x0200)
 
void SDRAM_Init(void);

#endif
