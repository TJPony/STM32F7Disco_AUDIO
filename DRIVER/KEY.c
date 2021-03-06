#include "KEY.h"
#include "timer.h"
#include <stdio.h>

static T_KEYOptr g_tKeyOptr = {
	 .value = KEY_NULL_VALUE,
	 .event = 0,
	 .init  = KEY_Init,
	 .GetEvent = GetKeyEvent,
};

static key_func_desc key_func_desc_array[KEY_FUNC_NUM];

s8 register_key_func(u8 value,u8 event,Key_FUNC func)
{
	u8 i;
	for(i = 0; i < KEY_FUNC_NUM; i++)
	{
		if(!key_func_desc_array[i].func)
		{
			key_func_desc_array[i].value = value;
			key_func_desc_array[i].event = event;
			key_func_desc_array[i].func   = func;
			return 0;
		}
	}
	return -1;
}

static void keyscan_timer_func(void)
{
	    u8 i;
		  if(g_tKeyOptr.GetEvent() == 0)
			{
				switch(g_tKeyOptr.event)
				{
					case KEY_PRESS: switch(g_tKeyOptr.value)
													{
															case KEY_WK_VALUE:
																   for(i = 0;i < KEY_FUNC_NUM;i++)
																	 {
																		 if((key_func_desc_array[i].value == KEY_WK_VALUE)&&(key_func_desc_array[i].event == KEY_PRESS)&&(key_func_desc_array[i].func))
																		 {
																			 key_func_desc_array[i].func();
																		 }
																	 }
																	 break;
														  default:break;
													}
													break;
					case KEY_LONG:  switch(g_tKeyOptr.value)
													{
															case KEY_WK_VALUE:
																   for(i = 0;i < KEY_FUNC_NUM;i++)
																	 {
																		 if((key_func_desc_array[i].value == KEY_WK_VALUE)&&(key_func_desc_array[i].event == KEY_LONG)&&(key_func_desc_array[i].func))
																		 {
																			 key_func_desc_array[i].func();
																		 }
																	 }
																	 break;
														  default:break;
													}
													break;
					case KEY_CONTINUE:switch(g_tKeyOptr.value)
													{
															case KEY_WK_VALUE:
																	 for(i = 0;i < KEY_FUNC_NUM;i++)
																	 {
																		 if((key_func_desc_array[i].value == KEY_WK_VALUE)&&(key_func_desc_array[i].event == KEY_CONTINUE)&&(key_func_desc_array[i].func))
																		 {
																			 key_func_desc_array[i].func();
																		 }
																	 }
																	 break;
														  default:break;
													}
													break;
					case KEY_UP:    switch(g_tKeyOptr.value)
													{
															case KEY_WK_VALUE:
																   for(i = 0;i < KEY_FUNC_NUM;i++)
																	 {
																		 if((key_func_desc_array[i].value == KEY_WK_VALUE)&&(key_func_desc_array[i].event == KEY_UP)&&(key_func_desc_array[i].func))
																		 {
																			 key_func_desc_array[i].func();
																		 }
																	 }
																	 break;
														  default:break;
													}
													break;
					default:break;
				}
			}
}

static void WakeUpKey_press_common_func(void)
{
	
}

static void WakeUpKey_long_common_func(void)
{
	printf("Wake up key long press!\r\n");
}

static void WakeUpKey_continue_common_func(void)
{
	printf("Wake up key continue press!\r\n");
}

static void WakeUpKey_release_common_func(void)
{
	
}

static void KEY_Init(void)
{
  GPIO_InitTypeDef GPIO_Initure;
	GPIO_Initure.Pin=KEY_WK_GPIO_PIN;  
	GPIO_Initure.Mode=GPIO_MODE_INPUT;  	    
	GPIO_Initure.Pull=GPIO_PULLDOWN;     
	GPIO_Initure.Speed=GPIO_SPEED_HIGH;  	
	HAL_GPIO_Init(KEY_WK_GPIO,&GPIO_Initure);
	
	register_timer_func((u8 *)"keyscan_timer_func",keyscan_timer_func);
	register_key_func(KEY_WK_VALUE,KEY_PRESS,WakeUpKey_press_common_func);
	register_key_func(KEY_WK_VALUE,KEY_LONG,WakeUpKey_long_common_func);
	register_key_func(KEY_WK_VALUE,KEY_CONTINUE,WakeUpKey_continue_common_func);
	register_key_func(KEY_WK_VALUE,KEY_UP,WakeUpKey_release_common_func);
}

static u8 Key_Value(void)
{
    if(HAL_GPIO_ReadPin(KEY_WK_GPIO,KEY_WK_GPIO_PIN) == KEY_WK_PRESS)  return KEY_WK_VALUE;
	  return KEY_NULL_VALUE;
}
static u8 Key_Scan(void)
{
       static u8 KeyState = KEY_INIT_STATE;
       static u16 KeyCounter = 0;
       static u8 KeyValueTemp = 0;
       u8 KeyValue = KEY_NULL_VALUE;        
        if(IS_KEY_DONW)
        {
               if(KeyState == KEY_INIT_STATE)
               {
                   KeyState = KEY_WOBBLE_STATE;
                   return KEY_NULL_VALUE;
               }
               else if(KeyState == KEY_WOBBLE_STATE)                /* 消抖 */
               {
                   KeyState = KEY_PRESS_STATE;
                   return KEY_NULL_VALUE;
               }
               else if(KeyState == KEY_PRESS_STATE)                /* 有按键按下，返回按键值 */
               {
                   KeyValue = Key_Value();
                   KeyValueTemp = KeyValue;
                   KeyState = KEY_LONG_STATE;                  
                   return (KeyValueTemp|KEY_PRESS);
               }
               else if(KeyState == KEY_LONG_STATE)                        /* 长按键 */
               {
                    KeyCounter ++;
                    if(KeyCounter == KEY_LONG_PERIOD)
                    {
                           KeyCounter = 0;
                           KeyState = KEY_CONTINUE_STATE;
                           return (KeyValueTemp | KEY_LONG);
                    }
               }
               else if(KeyState == KEY_CONTINUE_STATE)        /* 连续按键 */
               {
                   KeyCounter ++;
                   if(KeyCounter == KEY_CONTINUE_PERIOD)
                    {
                            KeyCounter = 0;
                            return (KeyValueTemp | KEY_CONTINUE);
                    }
               }
        }
        else if(IS_KEY_UP)
        {
            KeyState = KEY_INIT_STATE;        /* 误触发，返回到初始状态 */
				    KeyCounter=0;
					  if(g_tKeyOptr.event == KEY_UP)
						{
							g_tKeyOptr.event = 0;
						}
					  if(g_tKeyOptr.event)
						{
							return (KeyValueTemp | KEY_UP);
						}
        }
        return KEY_NULL_VALUE;
}

static int GetKeyEvent(void)
{
	u8 KeyValue;
	KeyValue = Key_Scan();
	if(KeyValue == KEY_NULL_VALUE)
	{
		return -1;
	}
	else
	{
		g_tKeyOptr.value = KeyValue & 0x0f;
		g_tKeyOptr.event = KeyValue & 0xf0;
		return 0;
	}
}

PT_KEYOptr Get_KEYOptr(void)
{
	PT_KEYOptr ptTemp;
	ptTemp = &g_tKeyOptr;
	return ptTemp;
}	

