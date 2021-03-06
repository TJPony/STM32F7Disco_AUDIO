#include "usb_bsp.h"
#include "delay.h"

void USB_OTG_BSP_Init(USB_OTG_CORE_HANDLE *pdev)
{
	  GPIO_InitTypeDef  GPIO_InitStruct;
	
    __HAL_RCC_USB_OTG_HS_CLK_ENABLE();              //使能OTG HS时钟
    __HAL_RCC_USB_OTG_HS_ULPI_CLK_ENABLE();         //使能OTG HS ULPI时钟
	
    GPIO_InitStruct.Pin=GPIO_PIN_3|GPIO_PIN_5;      //PA3 5
    GPIO_InitStruct.Mode=GPIO_MODE_AF_PP;           //复用
    GPIO_InitStruct.Pull=GPIO_NOPULL;               //无上下拉
    GPIO_InitStruct.Speed=GPIO_SPEED_HIGH;          //高速
    GPIO_InitStruct.Alternate=GPIO_AF10_OTG_HS;     //复用为OTG FS
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);         //初始化
	
		GPIO_InitStruct.Pin=GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_5|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13;      //PB0 1 5 10 11 12 13
	  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);         //初始化
	
		GPIO_InitStruct.Pin=GPIO_PIN_0;                 //PC0
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);         //初始化
	  
	  GPIO_InitStruct.Pin=GPIO_PIN_4;                 //PH4
		HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);         //初始化
	
	  GPIO_InitStruct.Pin=GPIO_PIN_11;                //PI11
		HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);         //初始化
}

void USB_OTG_BSP_EnableInterrupt(USB_OTG_CORE_HANDLE *pdev)
{
    HAL_NVIC_SetPriority(OTG_HS_IRQn,0,2);          //抢占优先级0，子优先级2
    HAL_NVIC_EnableIRQ(OTG_HS_IRQn);                //使能OTG USB HS中断 
}

void USB_OTG_BSP_uDelay (const uint32_t usec)
{
    delay_us(usec);
}

void USB_OTG_BSP_mDelay (const uint32_t msec)
{
    delay_ms(msec);
}
