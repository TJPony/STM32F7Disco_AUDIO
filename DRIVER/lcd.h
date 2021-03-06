/*****************************************************************************
文件名:  lcd.c
描述:    该文件为LCD屏底层驱动头文件 
作者:    马小龙
版本:    V1.0
时间:    2019年8月2日
修改记录:

*****************************************************************************/

#ifndef _LCD_H
#define _LCD_H
#include "sys.h"

#define INTERFACE_RGB        0
#define INTERFACE_MIPI       1

#define LCD_INTERFACE        INTERFACE_MIPI

#define BL_CTRL     PIout(14)   //LCD背光引脚	    

//画笔颜色
#define WHITE         	 0xFFFFFFFF
#define BLACK         	 0xFF000000	  
#define BLUE         	   0xFF0000FF  
#define RED           	 0xFFFF0000
#define GREEN         	 0xFF00FF00
#define YELLOW        	 0xFFFFFF00

#define LCD_PIXEL_FORMAT_ARGB8888       0X00    
#define LCD_PIXEL_FORMAT_RGB888         0X01    
#define LCD_PIXEL_FORMAT_RGB565         0X02       
#define LCD_PIXEL_FORMAT_ARGB1555       0X03      
#define LCD_PIXEL_FORMAT_ARGB4444       0X04     
#define LCD_PIXEL_FORMAT_L8             0X05     
#define LCD_PIXEL_FORMAT_AL44           0X06     
#define LCD_PIXEL_FORMAT_AL88           0X07 

//定义颜色像素格式为RGB565
#define LCD_PIXFORMAT				            LCD_PIXEL_FORMAT_ARGB8888	
//LCD帧缓冲区首地址
#define LCD_FRAME_BUF_ADDR			        0XC0000000  
//LCD设备 ID
#define LCD_OTM8009A_ID                 0

typedef enum
{
  LCD_ORIENTATION_PORTRAIT  = 0x00,  //垂直 肖像模式
  LCD_ORIENTATION_LANDSCAPE = 0x01,  //水平 景观模式
  LCD_ORIENTATION_INVALID   = 0x02   //无效
} LCD_OrientationTypeDef;

typedef struct _lcd_param
{							 
	u32 pwidth;				//LCD面板物理宽度
	u32 pheight;			//LCD面板物理高度
	u16 hsw;					//水平同步宽度
	u16 vsw;					//垂直同步宽度
	u16 hbp;					//水平后廊
	u16 vbp;					//垂直后廊
	u16 hfp;					//水平前廊
	u16 vfp;					//垂直前廊 
	u32 pixsize;			//每个像素所占字节数
}lcd_param; 
        
typedef struct t_LCDOptr
{
	u16 width;			  //LCD 宽度
	u16 height;			  //LCD 高度
	LCD_OrientationTypeDef  orientation;		//LCD方向
	
  u32 pen_color;    //画笔颜色
	u32 back_color;   //背景颜色
	u8  font_size;    //字体尺寸
	
	u8  cur_line;     //当前行，LCD_printf函数专用
	u8  cur_pos;      //当前位置，LCD_printf函数专用
	
	u8 activelayer;		//当前层编号:0/1	
	
	u32 *buf_addr[2];	//framebuffer地址
	
	lcd_param *param; //LCD参数
	
	void (*init)(void);
	void (*drawpoint)(u16 x,u16 y,u32 color);	
}T_LCDOptr,*PT_LCDOptr;

static void LCD_Init(void);
static void LCD_Draw_Point(u16 x,u16 y,u32 color);
void LCD_DisplayOn(void);
void LCD_DisplayOff(void);
void LCD_SetBrightness(u8 BrightnessValue);
void LCD_Display_Dir(LCD_OrientationTypeDef Orientation);
void LCD_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u32 color);
void LCD_Color_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u32 *color);
void LCD_Clear(u32 color);
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2);
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2);
void LCD_Draw_Circle(u16 x0,u16 y0,u8 r);
void LCD_ShowChar(u16 x,u16 y,u8 num,u8 size,u8 mode);
void LCD_ShowNum(u16 x,u16 y,u32 num,u8 len,u8 size);
void LCD_ShowString(u16 x,u16 y,u16 width,u16 height,u8 size,u8 *p);
void LCD_printf(const char *format, ...);
void DSI_IO_WriteCmd(uint32_t NbrParams, uint8_t *pParams);
PT_LCDOptr Get_LCDOptr(void);

#endif 
