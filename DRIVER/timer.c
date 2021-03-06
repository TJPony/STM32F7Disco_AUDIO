/*****************************************************************************
文件名:  timer.c
描述:    该文件为主定时器处理代码 ，所有与时间有关的功能都要注册定时器功能函数，定时器中断处理函数中依次对他们进行调用
作者:    马小龙
版本:    V1.0
时间:    2019年7月2日
修改记录:

*****************************************************************************/

#include "timer.h"

/* 定时器功能描述结构体数组 */
static timer_func_desc timer_array[TIMER_FUNC_NUM];

TIM_HandleTypeDef TIM7_Handler;         //定时器7句柄

/************************************************
名    称: ProcessTimer_Init
功    能: 基本定时器7初始化函数
输入参数: arr--无符号16位整型 自动重装在寄存器值 ARR
          psc--无符号16位整型 定时器时钟分频器值 PRESCALER
输出参数: 无
返回值:   void
************************************************/
void ProcessTimer_Init(u16 arr,u16 psc)
{ 
    __HAL_RCC_TIM7_CLK_ENABLE();                        //使能TIM7时钟
    
    TIM7_Handler.Instance=TIM7;                          //通用定时器7
    TIM7_Handler.Init.Prescaler=psc;                     //分频系数
    TIM7_Handler.Init.CounterMode=TIM_COUNTERMODE_UP;    //向上计数器
    TIM7_Handler.Init.Period=arr;                        //自动装载值
    TIM7_Handler.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;//时钟分频因子
    HAL_TIM_Base_Init(&TIM7_Handler);
    
    HAL_NVIC_SetPriority(TIM7_IRQn,1,0);    //设置中断优先级，抢占优先级1，子优先级0
    HAL_NVIC_EnableIRQ(TIM7_IRQn);          //开启ITM7中断   
	  
	  HAL_TIM_Base_Start_IT(&TIM7_Handler);   //使能定时器7和定时器7更新中断：TIM_IT_UPDATE   
}

/************************************************
名    称: register_timer_func
功    能: 定时器功能注册函数
输入参数: name--无符号8位字符指针，功能名称
          func--ProcessTimer_FUNC型函数指针，功能
输出参数: 无
返回值:   0:成功 -1:失败
************************************************/
s8 register_timer_func(u8 *name, ProcessTimer_FUNC func)
{
	u8 i;
	for(i = 0; i < TIMER_FUNC_NUM; i++)
	{
		if(!timer_array[i].func)
		{
			timer_array[i].name = name;
			timer_array[i].func   = func;
			return 0;
		}
	}
	return -1;
}

/************************************************
名    称: TIM7_IRQHandler
功    能: 基本定时器7中断处理函数，这里处理定时器更新中断
输入参数: 无
输出参数: 无
返回值:   void
************************************************/
void TIM7_IRQHandler(void)
{
	  u8 i;
    if(__HAL_TIM_GET_FLAG(&TIM7_Handler,TIM_FLAG_UPDATE)!=RESET)    //更新中断
    {
        __HAL_TIM_CLEAR_IT(&TIM7_Handler,TIM_IT_UPDATE);            //清除中断
			 
				for(i = 0; i < TIMER_FUNC_NUM; i++)
				{
					if (timer_array[i].func)                                  //如果有功能函数则调用
					{
						timer_array[i].func();
					}
				}
		}
} 

