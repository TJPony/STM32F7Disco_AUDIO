#include "led.h"
#include "timer.h"

static void LED_Init(void);
static void LED_On(u8 num);
static void LED_Off(u8 num);
static void LED_Toggle(u8 num);

static T_LedOptr g_tLedOptr = {
	.init   = LED_Init,
	.on     = LED_On,
	.off    = LED_Off,
	.toggle = LED_Toggle,
};

static void led_timer_func(void)
{
	static u32 counter1 = 0;
	static u32 counter2 = 0;
	if(++counter1 > TIME_500MS)
	{
		counter1 = 0;
		g_tLedOptr.toggle(LD_USER1);
	}
	if(++counter2 > TIME_500MS)
	{
		counter2 = 0;
		g_tLedOptr.toggle(LD_USER2);
	}
}

static void LED_Init(void)
{
	GPIO_InitTypeDef GPIO_Initure;
	GPIO_Initure.Pin=GPIO_PIN_5 | GPIO_PIN_13;  
	GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  	//通用推完输出
	GPIO_Initure.Pull=GPIO_PULLDOWN;          //下拉
	GPIO_Initure.Speed=GPIO_SPEED_HIGH;       //高速
	HAL_GPIO_Init(GPIOJ,&GPIO_Initure);
	
	LED_Off(LD_USER1);
	LED_Off(LD_USER2);
	
	register_timer_func((u8 *)"led_timer_func",led_timer_func);
}

static void LED_On(u8 num)
{
	switch(num)
	{
		case LD_USER1:HAL_GPIO_WritePin(GPIOJ,GPIO_PIN_13,GPIO_PIN_SET);break;
		case LD_USER2:HAL_GPIO_WritePin(GPIOJ,GPIO_PIN_5,GPIO_PIN_SET);break;
		default:break;
	}
}

static void LED_Off(u8 num)
{
	switch(num)
	{
		case LD_USER1:HAL_GPIO_WritePin(GPIOJ,GPIO_PIN_13,GPIO_PIN_RESET);break;
		case LD_USER2:HAL_GPIO_WritePin(GPIOJ,GPIO_PIN_5,GPIO_PIN_RESET);break;
		default:break;
	}
}

static void LED_Toggle(u8 num)
{
	switch(num)
	{
		case LD_USER1:HAL_GPIO_TogglePin(GPIOJ,GPIO_PIN_13);break;
		case LD_USER2:HAL_GPIO_TogglePin(GPIOJ,GPIO_PIN_5);break;
		default:break;
	}
}
PT_LedOptr Get_LedOptr(void)
{
	PT_LedOptr ptTemp;
	ptTemp = &g_tLedOptr;
	return ptTemp;
}
