/*****************************************************************************
文件名:  sys.c
描述:    该文件主要作用为初始化Stm32的时钟
作者:    马小龙
版本:    V1.0
时间:    2018年11月1日
修改记录:

*****************************************************************************/

#include "sys.h"

/************************************************
名    称: Cache_Enable
功    能: 使能数据和指令Cache
输入参数: 无
输出参数: 无
返回值:   void
************************************************/
void Cache_Enable(void)
{
  SCB_EnableICache();//使能I-Cache
  SCB_EnableDCache();//使能D-Cache    
	SCB->CACR|=1<<2;   //强制D-Cache透写,如不开启,实际使用中可能遇到各种问题
}	

/************************************************
名    称: Stm32_Clock_Init
功    能: Stm32时钟初始化函数
输入参数: plln--无符号32位整型 主PLL倍频系数(PLL倍频),取值范围:64~432.
					pllm--无符号32位整型 主PLL和音频PLL分频系数(PLL之前的分频),取值范围:2~63.
					pllp--无符号32位整型 系统时钟的主PLL分频系数(PLL之后的分频),取值范围:2,4,6,8
					pllq--无符号32位整型 USB/SDIO/随机数产生器等的主PLL分频系数(PLL之后的分频),取值范围:2~15.
输出参数: 无
返回值:   void
************************************************/
void Stm32_Clock_Init(u32 plln,u32 pllm,u32 pllp,u32 pllq)
{
    HAL_StatusTypeDef ret = HAL_OK;
    RCC_OscInitTypeDef RCC_OscInitStructure; 
    RCC_ClkInitTypeDef RCC_ClkInitStructure;
	
    __HAL_RCC_PWR_CLK_ENABLE(); //使能PWR时钟
 
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);//设置调压器输出电压级别1，以便在器件未以最大频率工作时使性能与功耗实现平衡
      
    RCC_OscInitStructure.OscillatorType=RCC_OSCILLATORTYPE_HSE;    //时钟源为HSE
    RCC_OscInitStructure.HSEState=RCC_HSE_ON;                      //打开HSE
    RCC_OscInitStructure.PLL.PLLState=RCC_PLL_ON;				   //打开PLL
    RCC_OscInitStructure.PLL.PLLSource=RCC_PLLSOURCE_HSE;          //PLL时钟源选择HSE
    RCC_OscInitStructure.PLL.PLLM=pllm;	//主PLL和音频PLL分频系数(PLL之前的分频)
    RCC_OscInitStructure.PLL.PLLN=plln; //主PLL倍频系数(PLL倍频)
    RCC_OscInitStructure.PLL.PLLP=pllp; //系统时钟的主PLL分频系数(PLL之后的分频)
    RCC_OscInitStructure.PLL.PLLQ=pllq; //USB/SDIO/随机数产生器等的主PLL分频系数(PLL之后的分频)
	  RCC_OscInitStructure.PLL.PLLR=7;
    ret=HAL_RCC_OscConfig(&RCC_OscInitStructure);//初始化
    if(ret!=HAL_OK) while(1);
    
    ret=HAL_PWREx_EnableOverDrive(); //开启Over-Driver功能
    if(ret!=HAL_OK) while(1);
    
    //选中PLL作为系统时钟源并且配置HCLK,PCLK1和PCLK2
    RCC_ClkInitStructure.ClockType=(RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStructure.SYSCLKSource=RCC_SYSCLKSOURCE_PLLCLK;//设置系统时钟时钟源为PLL
    RCC_ClkInitStructure.AHBCLKDivider=RCC_SYSCLK_DIV1;//AHB分频系数为1
    RCC_ClkInitStructure.APB1CLKDivider=RCC_HCLK_DIV4;//APB1分频系数为4
    RCC_ClkInitStructure.APB2CLKDivider=RCC_HCLK_DIV4;//APB2分频系数为4
    
    ret=HAL_RCC_ClockConfig(&RCC_ClkInitStructure,FLASH_LATENCY_7);//同时设置FLASH延时周期为7WS，也就是8个CPU周期。
    if(ret!=HAL_OK) while(1);
}

/************************************************
名    称: Get_ICacheSta
功    能: 判断ICache是否打开
输入参数: 无
输出参数: 无
返回值:   无符号8位整型 1打开 0关闭
************************************************/
u8 Get_ICacheSta(void)
{
    u8 sta;
    sta=((SCB->CCR)>>17)&0X01;
    return sta;
}

/************************************************
名    称: Get_DCacheSta
功    能: 判断DCache是否打开
输入参数: 无
输出参数: 无
返回值:   无符号8位整型 1打开 0关闭
************************************************/
u8 Get_DCacheSta(void)
{
    u8 sta;
    sta=((SCB->CCR)>>16)&0X01;
    return sta;
}

//THUMB指令不支持汇编内联
//采用如下方法实现执行汇编指令WFI  
__asm void WFI_SET(void)
{
	WFI;		  
}
//关闭所有中断(但是不包括fault和NMI中断)
__asm void INTX_DISABLE(void)
{
	CPSID   I
	BX      LR	  
}
//开启所有中断
__asm void INTX_ENABLE(void)
{
	CPSIE   I
	BX      LR  
}
//设置栈顶地址
//addr:栈顶地址
__asm void MSR_MSP(u32 addr) 
{
	MSR MSP, r0 			//set Main Stack value
	BX r14
}
