
/*****************************************************************************
文件名:  delay.c
描述:    该文件为延时功能代码，主要利用CortexM7内核Systick定时器完成延时功能
作者:    马小龙
版本:    V1.0
时间:    2018年11月1日
修改记录:

*****************************************************************************/
#include "delay.h"
#include "sys.h"

static u32 fac_us=0;							

/************************************************
名    称: delay_init
功    能: 延时初始化，设置SysTick的时钟源为HCLK
输入参数: SYSCLK--无符号8位整型，设置1us的时钟周期
输出参数: 无
返回值:   void
************************************************/
void delay_init(u8 SYSCLK)
{
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);     //SysTick频率为HCLK
	fac_us=SYSCLK;						    
}								    

/************************************************
名    称: delay_us
功    能: 延时us函数 
输入参数: nus--无符号32位整型，需要延时的us值SysTick为24位定时器,因此nus的最大值为2^24/SYSCLK
输出参数: 无
返回值:   void
************************************************/
void delay_us(u32 nus)
{		
	u32 ticks;
	u32 told,tnow,tcnt=0;
	u32 reload=SysTick->LOAD;				
	ticks=nus*fac_us; 						
	told=SysTick->VAL;        				
	while(1)
	{
		tnow=SysTick->VAL;	
		if(tnow!=told)
		{	    
			if(tnow<told)tcnt+=told-tnow;	
			else tcnt+=reload-tnow+told;	    
			told=tnow;
			if(tcnt>=ticks)break;			
		}  
	};
}

/************************************************
名    称: delay_ms
功    能: 延时ms函数 
输入参数: nms--无符号16位整型，需要延时的ms值
输出参数: 无
返回值:   void
************************************************/
void delay_ms(u16 nms)
{
	u32 i;
	for(i=0;i<nms;i++) delay_us(1000);
}
