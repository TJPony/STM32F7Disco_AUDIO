#ifndef __KEY_H
#define __KEY_H	 
#include "sys.h"

#define KEY_WK_GPIO        (GPIOA)
#define KEY_WK_GPIO_PIN    (GPIO_PIN_0)
#define KEY_WK_PRESS       (GPIO_PIN_SET)
#define KEY_WK_RELEASE     (GPIO_PIN_RESET)

/* 定义按键值 */
#define        KEY_NULL_VALUE       0
#define        KEY_WK_VALUE         0X01
#define        KEY_0_VALUE          0X02
#define        KEY_1_VALUE          0X04
#define        KEY_2_VALUE          0X08
/* 定义按键状态值 */
#define        KEY_PRESS            0X10
#define        KEY_LONG             0X20
#define        KEY_CONTINUE         0X40
#define        KEY_UP               0X80
/* 定义按键处理状态 */
#define        KEY_INIT_STATE       0
#define        KEY_WOBBLE_STATE     1
#define        KEY_PRESS_STATE      2
#define        KEY_LONG_STATE       3
#define        KEY_CONTINUE_STATE   4
#define        KEY_RELEASE_STATE    5
/* 长按时间 */
#define        KEY_LONG_PERIOD      200        /* 2S */
#define        KEY_CONTINUE_PERIOD  50         /* 500ms */
#define        IS_KEY_DONW         (HAL_GPIO_ReadPin(KEY_WK_GPIO,KEY_WK_GPIO_PIN) == KEY_WK_PRESS)
#define        IS_KEY_UP           (HAL_GPIO_ReadPin(KEY_WK_GPIO,KEY_WK_GPIO_PIN) == KEY_WK_RELEASE)

#define        KEY_FUNC_NUM        20

typedef struct 
{
	 u8 value;
	 u8 event;
	 void (*init)(void);
	 int (*GetEvent)(void);
}T_KEYOptr,*PT_KEYOptr;

typedef void(*Key_FUNC)(void);

typedef struct key_func_desc {
	u8 value;
	u8 event;
	Key_FUNC func;
}key_func_desc, *p_key_func_desc;

static void KEY_Init(void);//初始化
static u8 Key_Value(void);
static u8 Key_Scan(void);	
static int GetKeyEvent(void);

s8 register_key_func(u8 value,u8 event,Key_FUNC func);
PT_KEYOptr Get_KEYOptr(void);

#endif
