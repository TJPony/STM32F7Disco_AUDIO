/*****************************************************************************
文件名:  lcd.c
描述:    该文件为LCD屏底层驱动代码
作者:    马小龙
版本:    V1.0
时间:    2019年8月2日
修改记录:

*****************************************************************************/

#include "lcd.h"
#include "delay.h"
#include "otm8009a.h"
#include "font.h"
#include "text.h"
#include <stdarg.h>
#include <stdio.h>

/* 定义LCD所需的帧缓存数组大小 分辨率为800*480 ARGB888 */
u32 lcd_framebuf[480][800] __attribute__((at(LCD_FRAME_BUF_ADDR)));	

static lcd_param discovery_lcd_param;	

/* LCD操作结构体示例 */
static T_LCDOptr g_tLCDOptr = {
	.pen_color  = BLUE,
	.back_color = WHITE,
	.font_size  = 24,
	.cur_line   = 1,
	.cur_pos    = 8,
	.param      = &discovery_lcd_param,
	.init       = LCD_Init,
	.drawpoint  = LCD_Draw_Point,
};

static DSI_HandleTypeDef   DSI_Handler;       //DSI_HOST句柄
static LTDC_HandleTypeDef  LTDC_Handler;	    //LTDC句柄
static DMA2D_HandleTypeDef DMA2D_Handler; 	  //DMA2D句柄

/************************************************
名    称: DSI_IO_WriteCmd
功    能: DSI接口发送命令
输入参数: NbrParams--无符号32位整型 DCS指令参数个数
          pParams--无符号8位整型指针 DCS参数数组
输出参数: 无
返回值:   void
************************************************/
void DSI_IO_WriteCmd(uint32_t NbrParams, uint8_t *pParams)
{
  if(NbrParams <= 1)
  {
   HAL_DSI_ShortWrite(&DSI_Handler,LCD_OTM8009A_ID, DSI_DCS_SHORT_PKT_WRITE_P1, pParams[0], pParams[1]); 
  }
  else
  {
   HAL_DSI_LongWrite(&DSI_Handler,LCD_OTM8009A_ID, DSI_DCS_LONG_PKT_WRITE, NbrParams, pParams[NbrParams], pParams); 
  } 
}

/************************************************
名    称: LTDC_Switch
功    能: LTDC控制器使能函数
输入参数: sw--无符号8位整型 1:打开 0:关闭
输出参数: 无
返回值:   void
************************************************/
static void LTDC_Switch(u8 sw)
{
	if(sw==1) __HAL_LTDC_ENABLE(&LTDC_Handler);
	else if(sw==0)__HAL_LTDC_DISABLE(&LTDC_Handler);
}

/************************************************
名    称: LTDC_Layer_Switch
功    能: LTDC层使能函数
输入参数: layerx--无符号8位整型 0:第0层 1:第一层
          sw--无符号8位整型 1:打开 0:关闭
输出参数: 无
返回值:   void
************************************************/
static void LTDC_Layer_Switch(u8 layerx,u8 sw)
{
	if(sw==1) __HAL_LTDC_LAYER_ENABLE(&LTDC_Handler,layerx);
	else if(sw==0) __HAL_LTDC_LAYER_DISABLE(&LTDC_Handler,layerx);
	__HAL_LTDC_RELOAD_CONFIG(&LTDC_Handler);
}

/************************************************
名    称: LTDC_Select_Layer
功    能: LTDC层选择函数
输入参数: layerx--无符号8位整型 0:第0层 1:第一层
输出参数: 无
返回值:   void
************************************************/
static void LTDC_Select_Layer(u8 layerx)
{
	g_tLCDOptr.activelayer = layerx;
}

/************************************************
名    称: LCD_Draw_Point
功    能: LCD描点函数
输入参数: x--无符号16位整型 LCD显示横坐标
          y--无符号16位整型 LCD显示纵坐标
          color--无符号32位整型 点颜色
输出参数: 无
返回值:   void
************************************************/
static void LCD_Draw_Point(u16 x,u16 y,u32 color)
{ 
#if LCD_INTERFACE == INTERFACE_MIPI  //DSI接口屏 无需进行坐标变换
	*(u32*)((u32)g_tLCDOptr.buf_addr[g_tLCDOptr.activelayer]+g_tLCDOptr.param->pixsize*(g_tLCDOptr.param->pwidth*y+x))=color;
#else																 //RGB接口屏 进行坐标变换
  if(g_tLCDOptr.orientation)	       //横屏
	{
        *(u32*)((u32)g_tLCDOptr.buf_addr[g_tLCDOptr.activelayer]+g_tLCDOptr.param->pixsize*(g_tLCDOptr.param->pwidth*y+x))=color;
	}
	else 			               					 //竖屏
	{
        *(u32*)((u32)g_tLCDOptr.buf_addr[g_tLCDOptr.activelayer]+g_tLCDOptr.param->pixsize*(g_tLCDOptr.param->pwidth*(g_tLCDOptr.param->pheight-x-1)+y))=color; 
	}
#endif
}


/************************************************
名    称: LTDC_Clk_Set
功    能: LCD像素时钟设置函数
输入参数: pllsain--无符号32位整型 SAI时钟倍频系数N,取值范围:50~432.  
          pllsair--无符号32位整型 SAI时钟的分频系数R,取值范围:2~7
          pllsaidivr--无符号32位整型 LCD时钟分频系数,取值范围:RCC_PLLSAIDIVR_2/4/8/16,对应分频2~16 
输出参数: 无
返回值:   无符号8位整型 0:成功 1:失败
************************************************/
static u8 LTDC_Clk_Set(u32 pllsain,u32 pllsair,u32 pllsaidivr)
{
	RCC_PeriphCLKInitTypeDef PeriphClkIniture;
	
  PeriphClkIniture.PeriphClockSelection=RCC_PERIPHCLK_LTDC;	//LTDC时钟 	
	PeriphClkIniture.PLLSAI.PLLSAIN=pllsain;    
	PeriphClkIniture.PLLSAI.PLLSAIR=pllsair;  
	PeriphClkIniture.PLLSAIDivR=pllsaidivr;
	if(HAL_RCCEx_PeriphCLKConfig(&PeriphClkIniture)==HAL_OK)  //配置像素时钟
  {
      return 0;   //成功
  }
  else return 1;  //失败    
}

/************************************************
名    称: LTDC_Layer_Window_Config
功    能: LCD层窗口设置函数 以物理坐标为基准 此函数必须在LTDC_Layer_Parameter_Config之后再设置.
输入参数: layerx--无符号8位整型 0:第0层 1:第一层
          sx--无符号16位整型 窗口起始横坐标
          sy--无符号16位整型 窗口起始纵坐标
          width--无符号16位整型 窗口宽度
          height--无符号16位整型 窗口高度
输出参数: 无
返回值:   无
************************************************/
static void LTDC_Layer_Window_Config(u8 layerx,u16 sx,u16 sy,u16 width,u16 height)
{
    HAL_LTDC_SetWindowPosition(&LTDC_Handler,sx,sy,layerx);  //设置窗口的位置
    HAL_LTDC_SetWindowSize(&LTDC_Handler,width,height,layerx);//设置窗口大小    
}

/************************************************
名    称: LTDC_Layer_Parameter_Config
功    能: LCD层参数设置函数 
输入参数: layerx   --无符号8位整型  0:第0层 1:第一层
          bufaddr  --无符号32位整型 framebuffer地址
          pixformat--无符号8位整型  像素格式
          alpha    --无符号8位整型  层颜色Alpha值,0,全透明;255,不透明
          alpha0   --无符号16位整型 默认颜色Alpha值,0,全透明;255,不透明
          bfac1    --无符号8位整型  混合系数1,4(100),恒定的Alpha;6(101),像素Alpha*恒定Alpha
          bfac2    --无符号8位整型  混合系数2,5(101),恒定的Alpha;7(111),像素Alpha*恒定Alpha
          bkcolor  --无符号32位整型 层默认颜色,32位,低24位有效,RGB888格式
输出参数: 无
返回值:   无
************************************************/
static void LTDC_Layer_Parameter_Config(u8 layerx,u32 bufaddr,u8 pixformat,u8 alpha,u8 alpha0,u8 bfac1,u8 bfac2,u32 bkcolor)
{
	LTDC_LayerCfgTypeDef pLayerCfg;

	pLayerCfg.WindowX0=0;                       					//窗口起始X坐标
	pLayerCfg.WindowY0=0;                       					//窗口起始Y坐标
	pLayerCfg.WindowX1=g_tLCDOptr.param->pwidth;          //窗口终止X坐标
	pLayerCfg.WindowY1=g_tLCDOptr.param->pheight;         //窗口终止Y坐标
	pLayerCfg.PixelFormat=pixformat;		        					//像素格式
	pLayerCfg.Alpha=alpha;				              					//Alpha值设置，0~255,255为完全不透明
	pLayerCfg.Alpha0=alpha0;			              					//默认Alpha值
	pLayerCfg.BlendingFactor1=(u32)bfac1<<8;    					//设置层混合系数
	pLayerCfg.BlendingFactor2=(u32)bfac2<<8;	  					//设置层混合系数
	pLayerCfg.FBStartAdress=bufaddr;	          					//设置层颜色帧缓存起始地址
	pLayerCfg.ImageWidth=g_tLCDOptr.param->pwidth;        //设置颜色帧缓冲区的宽度    
	pLayerCfg.ImageHeight=g_tLCDOptr.param->pheight;      //设置颜色帧缓冲区的高度
	pLayerCfg.Backcolor.Red=(u8)(bkcolor&0X00FF0000)>>16; //背景颜色红色部分
	pLayerCfg.Backcolor.Green=(u8)(bkcolor&0X0000FF00)>>8;//背景颜色绿色部分
	pLayerCfg.Backcolor.Blue=(u8)bkcolor&0X000000FF;      //背景颜色蓝色部分
	HAL_LTDC_ConfigLayer(&LTDC_Handler,&pLayerCfg,layerx);//设置所选中的层
}  

/************************************************
名    称: LCD_ResetPin_Init
功    能: LCD_硬件复位引脚初始化
输入参数: 无
输出参数: 无
返回值:   无
************************************************/
static void LCD_ResetPin_Init(void)
{
	GPIO_InitTypeDef GPIO_Initure;
     
	GPIO_Initure.Pin=GPIO_PIN_15;               //PB14推挽输出，控制背光
	GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;      //推挽输出
	GPIO_Initure.Pull=GPIO_PULLUP;              //上拉        
	GPIO_Initure.Speed=GPIO_SPEED_HIGH;         //高速
	HAL_GPIO_Init(GPIOJ,&GPIO_Initure);
}

/************************************************
名    称: LCD_Reset
功    能: LCD硬件复位
输入参数: 无
输出参数: 无
返回值:   无
************************************************/
static void LCD_Reset(void)
{
	HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_15, GPIO_PIN_RESET);
	delay_ms(20);
	HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_15, GPIO_PIN_SET);
	delay_ms(10);
}

/************************************************
名    称: DSI_HOST_LPCmd_Enable
功    能: DSI_HOST LP命令发送模式配置函数
输入参数: enable--无符号8位整型 0-HS模式发送 1-LP模式发送
输出参数: 无
返回值:   无
************************************************/
static void DSI_HOST_LPCmd_Enable(u8 enable)
{
	DSI_LPCmdTypeDef   		DsiHostLPCmd;
	
	if(enable)
	{
		DsiHostLPCmd.LPGenShortWriteNoP    					= DSI_LP_GSW0P_ENABLE;          //零参数通用短写使用LP模式
		DsiHostLPCmd.LPGenShortWriteOneP   					= DSI_LP_GSW1P_ENABLE;          //一参数通用短写使用LP模式
		DsiHostLPCmd.LPGenShortWriteTwoP   					= DSI_LP_GSW2P_ENABLE;          //两参数通用短写使用LP模式
		DsiHostLPCmd.LPGenShortReadNoP     					= DSI_LP_GSR0P_ENABLE;          //零参数通用短读使用LP模式
		DsiHostLPCmd.LPGenShortReadOneP    					= DSI_LP_GSR1P_ENABLE;          //一参数通用短读使用LP模式
		DsiHostLPCmd.LPGenShortReadTwoP    					= DSI_LP_GSR2P_ENABLE;          //两参数通用短读使用LP模式
		DsiHostLPCmd.LPGenLongWrite        					= DSI_LP_GLW_ENABLE;            //通用长写使用LP模式
		DsiHostLPCmd.LPDcsShortWriteNoP    					= DSI_LP_DSW0P_ENABLE;          //零参数DCS短写使用LP模式
		DsiHostLPCmd.LPDcsShortWriteOneP   					= DSI_LP_DSW1P_ENABLE;          //一参数DCS短写使用LP模式
		DsiHostLPCmd.LPDcsShortReadNoP     					= DSI_LP_DSR0P_ENABLE;          //零参数DCS短读使用LP模式
		DsiHostLPCmd.LPDcsLongWrite        					= DSI_LP_DLW_ENABLE;            //DCS长写使用LP模式
		DsiHostLPCmd.AcknowledgeRequest             = DSI_ACKNOWLEDGE_DISABLE;      //禁止应答触发
		
		HAL_DSI_ConfigCommand(&DSI_Handler,&DsiHostLPCmd);
	}
	else
	{
		DsiHostLPCmd.LPGenShortWriteNoP    					= DSI_LP_GSW0P_DISABLE;					//零参数通用短写使用HS模式
		DsiHostLPCmd.LPGenShortWriteOneP   					= DSI_LP_GSW1P_DISABLE;					//一参数通用短写使用HS模式
		DsiHostLPCmd.LPGenShortWriteTwoP   					= DSI_LP_GSW2P_DISABLE;					//两参数通用短写使用HS模式
		DsiHostLPCmd.LPGenShortReadNoP     					= DSI_LP_GSR0P_DISABLE;					//零参数通用短读使用HS模式
		DsiHostLPCmd.LPGenShortReadOneP    					= DSI_LP_GSR1P_DISABLE;				  //一参数通用短读使用HS模式
		DsiHostLPCmd.LPGenShortReadTwoP    					= DSI_LP_GSR2P_DISABLE;					//两参数通用短读使用HS模式
		DsiHostLPCmd.LPGenLongWrite        					= DSI_LP_GLW_DISABLE;						//通用长写使用HS模式
		DsiHostLPCmd.LPDcsShortWriteNoP    					= DSI_LP_DSW0P_DISABLE;					//零参数DCS短写使用HS模式
		DsiHostLPCmd.LPDcsShortWriteOneP   					= DSI_LP_DSW1P_DISABLE;					//一参数DCS短写使用HS模式
		DsiHostLPCmd.LPDcsShortReadNoP     					= DSI_LP_DSR0P_DISABLE;					//零参数DCS短读使用HS模式
		DsiHostLPCmd.LPDcsLongWrite        					= DSI_LP_DLW_DISABLE;						//DCS长写使用HS模式
		DsiHostLPCmd.AcknowledgeRequest             = DSI_ACKNOWLEDGE_DISABLE;      //禁止应答触发
		
		HAL_DSI_ConfigCommand(&DSI_Handler, &DsiHostLPCmd);
	}
}

/************************************************
名    称: LTDC_Init
功    能: LCD初始化函数
输入参数: 无
输出参数: 无
返回值:   无
************************************************/
static void LTDC_Init(void)
{   
		LTDC_Clk_Set(417,5,RCC_PLLSAIDIVR_2); 											//设置像素时钟41.7MHz DSI两个数据通道 每通道500MHz 24bit数据模式 500*2/24 = 41.7Mhz

    LTDC_Handler.Instance=LTDC;
    LTDC_Handler.Init.HSPolarity=LTDC_HSPOLARITY_AL;          	//水平同步极性
    LTDC_Handler.Init.VSPolarity=LTDC_VSPOLARITY_AL;          	//垂直同步极性
    LTDC_Handler.Init.DEPolarity=LTDC_DEPOLARITY_AL;          	//数据使能极性
    LTDC_Handler.Init.PCPolarity=LTDC_PCPOLARITY_IPC;         	//像素时钟极性
    LTDC_Handler.Init.HorizontalSync=g_tLCDOptr.param->hsw;           	//水平同步宽度
    LTDC_Handler.Init.VerticalSync=g_tLCDOptr.param->vsw;             	//垂直同步宽度
    LTDC_Handler.Init.AccumulatedHBP=g_tLCDOptr.param->hsw+g_tLCDOptr.param->hbp; //水平同步后沿宽度
    LTDC_Handler.Init.AccumulatedVBP=g_tLCDOptr.param->vsw+g_tLCDOptr.param->vbp; //垂直同步后沿高度
    LTDC_Handler.Init.AccumulatedActiveW=g_tLCDOptr.param->hsw+g_tLCDOptr.param->hbp+g_tLCDOptr.param->pwidth;//有效宽度
    LTDC_Handler.Init.AccumulatedActiveH=g_tLCDOptr.param->vsw+g_tLCDOptr.param->vbp+g_tLCDOptr.param->pheight;//有效高度
    LTDC_Handler.Init.TotalWidth=g_tLCDOptr.param->hsw+g_tLCDOptr.param->hbp+g_tLCDOptr.param->pwidth+g_tLCDOptr.param->hfp;   //总宽度
    LTDC_Handler.Init.TotalHeigh=g_tLCDOptr.param->vsw+g_tLCDOptr.param->vbp+g_tLCDOptr.param->pheight+g_tLCDOptr.param->vfp;  //总高度
    LTDC_Handler.Init.Backcolor.Red=0;           //屏幕背景层红色部分
    LTDC_Handler.Init.Backcolor.Green=0;         //屏幕背景层绿色部分
    LTDC_Handler.Init.Backcolor.Blue=0;          //屏幕背景色蓝色部分
    HAL_LTDC_Init(&LTDC_Handler);
}

/************************************************
名    称: DSI_HOST_Init
功    能: DSI_HOST初始化函数
输入参数: 无
输出参数: 无
返回值:   无
************************************************/
static void DSI_HOST_Init(void)
{
	DSI_PLLInitTypeDef 		DsiHostPLL;
	DSI_CmdCfgTypeDef  		DsiHostCmdCfg;
	DSI_PHY_TimerTypeDef  DsiHostPhyTimings;
	
	DSI_Handler.Instance = DSI;
	
  HAL_DSI_DeInit(&DSI_Handler);
	
	/* 设置DSI_HOST时钟为500M */
	DsiHostPLL.PLLNDIV   = 100;
  DsiHostPLL.PLLIDF    = DSI_PLL_IN_DIV5;
  DsiHostPLL.PLLODF    = DSI_PLL_OUT_DIV1;  

	/* 配置DSI_HOST全局参数 */
  DSI_Handler.Init.NumberOfLanes = DSI_TWO_DATA_LANES;         								//两条数据通道
  DSI_Handler.Init.TXEscapeCkdiv = 4;                          								//TX逸出时钟为62.5/4 = 15.625MHz
  HAL_DSI_Init(&DSI_Handler,&DsiHostPLL);
	
	/* 适配指令模式配置 */
  DsiHostCmdCfg.VirtualChannelID      				= LCD_OTM8009A_ID;              //虚拟通道ID为0 一个LCD设备
  DsiHostCmdCfg.HSPolarity                    = DSI_HSYNC_ACTIVE_HIGH; 				//水平同步极性和LTDC相同 适配命令模式下无需设置
  DsiHostCmdCfg.VSPolarity            				= DSI_VSYNC_ACTIVE_HIGH; 				//垂直同步极性和LTDC相同 适配命令模式下无需设置
  DsiHostCmdCfg.DEPolarity            				= DSI_DATA_ENABLE_ACTIVE_HIGH;  //DE信号极性和LTDC相反   适配命令模式下无需设置
  DsiHostCmdCfg.ColorCoding           				= DSI_RGB888;                   //配置DSI输出信号格式为24bit
  DsiHostCmdCfg.CommandSize           				= g_tLCDOptr.width;     				//配置命令的最大尺寸，以像素为单位
  DsiHostCmdCfg.TearingEffectSource   				= DSI_TE_DSILINK;               //配置撕裂效应触发通过链路报告
  DsiHostCmdCfg.TearingEffectPolarity 				= DSI_TE_RISING_EDGE;           //配置撕裂效应触发引脚极性(未使用)
  DsiHostCmdCfg.VSyncPol                      = DSI_VSYNC_FALLING;            //配置LTDC停止的垂直同步边沿为下降沿
  DsiHostCmdCfg.AutomaticRefresh      				= DSI_AR_ENABLE;                //自动刷新 
  DsiHostCmdCfg.TEAcknowledgeRequest  				= DSI_TE_ACKNOWLEDGE_ENABLE;    //撕裂效应请求使能
  HAL_DSI_ConfigAdaptedCommandMode(&DSI_Handler,&DsiHostCmdCfg);
	
	/* 配置LP模式指令传输使能 */
	DSI_HOST_LPCmd_Enable(1);                     
	
	/* 配置DSI-PHY时序参数 */
	DsiHostPhyTimings.ClockLaneHS2LPTime 				= 35;                           //配置时钟通道HS->LP模式转换最长时间为35个Lane_Byte_CLK
  DsiHostPhyTimings.ClockLaneLP2HSTime			  = 35;                           //配置时钟通道LP->HS模式转换最长时间为35个Lane_Byte_CLK
  DsiHostPhyTimings.DataLaneHS2LPTime 				= 35;                           //配置数据通道HS->LP模式转换最长时间为35个Lane_Byte_CLK
  DsiHostPhyTimings.DataLaneLP2HSTime 				= 35;                           //配置数据通道LP->HS模式转换最长时间为35个Lane_Byte_CLK
  DsiHostPhyTimings.DataLaneMaxReadTime 			= 0;                            //配置读取命令的最长时间为0个Lane_Byte_CLK
  DsiHostPhyTimings.StopWaitTime 							= 10;                           //配置停止等待最小时间为10个Lane_Byte_CLK
  HAL_DSI_ConfigPhyTimer(&DSI_Handler,&DsiHostPhyTimings);  
	
	HAL_DSI_Start(&DSI_Handler);
}

/************************************************
名    称: HAL_LTDC_MspInit
功    能: LTDC Msp初始化函数 对相关功能进行时钟、中断、IO的初始化
输入参数: hltdc--LTDC控制器句柄
输出参数: 无
返回值:   void
************************************************/
void HAL_LTDC_MspInit(LTDC_HandleTypeDef* hltdc)
{
    __HAL_RCC_LTDC_CLK_ENABLE();                //使能LTDC时钟
    __HAL_RCC_DMA2D_CLK_ENABLE();               //使能DMA2D时钟
}

/************************************************
名    称: HAL_DSI_MspInit
功    能: DSI_HOST Msp初始化函数 对相关功能进行时钟、中断、IO的初始化
输入参数: hdsi--DSI控制器句柄
输出参数: 无
返回值:   void
************************************************/
void HAL_DSI_MspInit(DSI_HandleTypeDef *hdsi)
{
    __HAL_RCC_DSI_CLK_ENABLE();                 //使能DSI_HOST时钟

	  HAL_NVIC_SetPriority(DSI_IRQn, 3, 0);       
		HAL_NVIC_EnableIRQ(DSI_IRQn);               //使能DSI_HOST中断
}

/************************************************
名    称: LCD_Param_Init
功    能: LCD参数初始化函数 设置LCD面板物理尺寸 同步时序参数 像素尺寸 framebuffer基地址
输入参数: 无
输出参数: 无
返回值:   void
************************************************/
static void LCD_Param_Init(void)
{
	/* 适配命令模式不需要设置同步参数，均设为最小值即可 */
		g_tLCDOptr.param->pwidth  = 800;			  										//物理宽度,单位:像素
		g_tLCDOptr.param->pheight = 480;		  											//物理高度,单位:像素
		g_tLCDOptr.param->hsw     = 1;				    									//水平同步宽度
		g_tLCDOptr.param->vsw     = 1;				    									//垂直同步宽度
		g_tLCDOptr.param->hbp     = 1;				    									//水平后廊
		g_tLCDOptr.param->vbp     = 1;				    									//垂直后廊
		g_tLCDOptr.param->hfp     = 1;			      									//水平前廊
		g_tLCDOptr.param->vfp     = 1;				    									//垂直前廊
	  g_tLCDOptr.param->pixsize = 4;				                   	 	//每个像素占4个字节
	  g_tLCDOptr.buf_addr[0]    = (u32*)&lcd_framebuf;          	//设置framebuffer地址
}

/************************************************
名    称: Set_TE_Scanline
功    能: 设置撕裂效应触发行
输入参数: line--无符号16位整型 行数 0~pwidth-1
输出参数: 无
返回值:   无
************************************************/
static void Set_TE_Scanline(u16 line)
{
	u8 params[2];
	params[0] = line>>8;
	params[1] = line;
	HAL_DSI_LongWrite(&DSI_Handler,LCD_OTM8009A_ID,DSI_DCS_LONG_PKT_WRITE,2,OTM8009A_CMD_WRTESCN,params);
}

/************************************************
名    称: Set_TE_On
功    能: 设置撕裂效应报告使能
输入参数: 无
输出参数: 无
返回值:   无
************************************************/
static void Set_TE_On(void)
{
	HAL_DSI_ShortWrite(&DSI_Handler,LCD_OTM8009A_ID,DSI_DCS_SHORT_PKT_WRITE_P1,OTM8009A_CMD_TEEON,0x00);
}

/************************************************
名    称: LCD_Init
功    能: LCD初始化函数
输入参数: 无
输出参数: 无
返回值:   无
************************************************/
static void LCD_Init(void)
{
	/* LCD参数初始化 */
	LCD_Param_Init();
	/* LCD方向设置 默认横屏 */
	LCD_Display_Dir(LCD_ORIENTATION_PORTRAIT);			
	/* LCD硬件复位引脚初始化 */
	LCD_ResetPin_Init();
  /* LCD硬件复位 */	
	LCD_Reset();   
  /* LTDC接口初始化 */	
	LTDC_Init();       
  /* DSI_HOST初始化 */	
	DSI_HOST_Init(); 
  /* OTM8009A初始化 */	
	OTM8009A_Init(OTM8009A_COLMOD_RGB888, g_tLCDOptr.orientation);
	/* 配置LP模式指令传输禁止 */
	DSI_HOST_LPCmd_Enable(0);
	/* DSI协议流控制 总线转向BTA使能 */
  HAL_DSI_ConfigFlowControl(&DSI_Handler,DSI_FLOW_CONTROL_BTA);	
	/* 关闭OTM8009A显示 */
	LCD_DisplayOff();
	/* 刷新显示 */
	HAL_DSI_Refresh(&DSI_Handler);
	/* LTDC层参数配置 */
	LTDC_Layer_Parameter_Config(0,(u32)g_tLCDOptr.buf_addr[0],LCD_PIXFORMAT,255,0,6,7,0X000000);
	/* LTDC层窗口配置 */
	LTDC_Layer_Window_Config(0,0,0,g_tLCDOptr.param->pwidth,g_tLCDOptr.param->pheight);
	/* 选择第一层 */
	LTDC_Select_Layer(0);
	/* 设置撕裂效应触发行 */
	Set_TE_Scanline(479);	
	/* 打开OTM8009A显示 */
  LCD_DisplayOn();
	/* 清屏 */
	LCD_Clear(g_tLCDOptr.back_color);
  /* 刷新显示 */
  HAL_DSI_Refresh(&DSI_Handler);
}

void DSI_IRQHandler(void)
{
	/* TE触发中断 */
	if (__HAL_DSI_GET_FLAG(&DSI_Handler, DSI_FLAG_TE) != 0U)
  {
      __HAL_DSI_CLEAR_FLAG(&DSI_Handler, DSI_FLAG_TE);
	}
	/* End Of Refresh中断 */
	if (__HAL_DSI_GET_FLAG(&DSI_Handler, DSI_FLAG_ER) != 0U)
  {
      __HAL_DSI_CLEAR_FLAG(&DSI_Handler, DSI_FLAG_ER);
		  Set_TE_On();	
	}
}
/************************************************
名    称: LCD_Display_Dir
功    能: LCD显示方向设置
输入参数: dir--无符号8位整型 HORIZONTAL:横屏 VERTICAL:竖屏
输出参数: 无
返回值:   void
************************************************/
void LCD_Display_Dir(LCD_OrientationTypeDef Orientation)
{
  g_tLCDOptr.orientation = Orientation; 	
	
#if LCD_INTERFACE == INTERFACE_MIPI  									//DSI接口屏 根据方向重新定义屏物理尺寸
	if(Orientation == LCD_ORIENTATION_PORTRAIT)			    //竖屏
	{
		g_tLCDOptr.param->pwidth  = 480;
		g_tLCDOptr.param->pheight = 800;	
		g_tLCDOptr.width  = g_tLCDOptr.param->pwidth;
		g_tLCDOptr.height = g_tLCDOptr.param->pheight;
	}
	else if(Orientation == LCD_ORIENTATION_LANDSCAPE)	  //横屏
	{
		g_tLCDOptr.param->pwidth  = 800;
		g_tLCDOptr.param->pheight = 480;
    g_tLCDOptr.width  = g_tLCDOptr.param->pwidth;
		g_tLCDOptr.height = g_tLCDOptr.param->pheight;		
	}
#else 																								//RGB接口屏 物理尺寸固定
	if(Orientation == LCD_ORIENTATION_PORTRAIT)			  	//竖屏
	{
		g_tLCDOptr.width  = g_tLCDOptr.param->pheight;
		g_tLCDOptr.height = g_tLCDOptr.param->pwidth;	
	}
	else if(Orientation == LCD_ORIENTATION_LANDSCAPE)	  //横屏
	{
		g_tLCDOptr.width  = g_tLCDOptr.param->pwidth;
		g_tLCDOptr.height = g_tLCDOptr.param->pheight;
	}
#endif
}

/************************************************
名    称: LCD_Fill
功    能: LCD单色填充函数 利用硬件DMA2D进行填充
输入参数: sx--无符号16位整型 填充区起始横坐标
          sy--无符号16位整型 填充区起始纵坐标
          ex--无符号16位整型 填充区结束横坐标
          ey--无符号16位整型 填充区结束纵坐标
					color--无符号32位整型 填充颜色
输出参数: 无
返回值:   void
************************************************/
void LCD_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u32 color)
{ 
	u32 psx,psy,pex,pey;	    				 //以LCD面板物理坐标
	u32 timeout=0; 
	u16 offline;
	u32 addr; 
	
#if LCD_INTERFACE == INTERFACE_MIPI  //DSI接口屏 无需进行坐标变换
	psx = sx;
	psy = sy;
	pex = ex;
	pey = ey;
	
#else 															 //RGB接口屏 坐标系转换
/*if(g_tLCDOptr.orientation)  		   //横屏
	{
		psx = sx;
		psy = sy;
		pex = ex;
		pey = ey;
	}else			                         //竖屏
	{
		psx = sy;
		psy = g_tLCDOptr.param->pheight-ex-1;
		pex = ey;
		pey = g_tLCDOptr.param->pheight-sx-1;
	}*/
#endif
	offline = g_tLCDOptr.param->pwidth-(pex-psx+1);
	addr    = ((u32)g_tLCDOptr.buf_addr[g_tLCDOptr.activelayer]+g_tLCDOptr.param->pixsize*(g_tLCDOptr.param->pwidth*psy+psx));
	
	__HAL_RCC_DMA2D_CLK_ENABLE();								//使能DM2D时钟
	DMA2D->CR&=~(DMA2D_CR_START);								//先停止DMA2D
	DMA2D->CR=DMA2D_R2M;												//寄存器到存储器模式
	DMA2D->OPFCCR=LCD_PIXFORMAT;								//设置颜色格式
	DMA2D->OOR=offline;													//设置行偏移 
	DMA2D->OMAR=addr;														//输出存储器地址
	DMA2D->NLR=(pey-psy+1)|((pex-psx+1)<<16);		//设定行数寄存器
	DMA2D->OCOLR=color;													//设定输出颜色寄存器 
	DMA2D->CR|=DMA2D_CR_START;									//启动DMA2D
	while((DMA2D->ISR&(DMA2D_FLAG_TC))==0)			//等待传输完成
	{
		timeout++;
		if(timeout>0X1FFFFF)break;								//超时退出
	} 
	DMA2D->IFCR|=DMA2D_FLAG_TC;									//清除传输完成标志 		
}

/************************************************
名    称: LCD_Color_Fill
功    能: LCD单色填充函数 利用硬件DMA2D进行填充
输入参数: sx--无符号16位整型 填充区起始横坐标
          sy--无符号16位整型 填充区起始纵坐标
          ex--无符号16位整型 填充区结束横坐标
          ey--无符号16位整型 填充区结束纵坐标
					color--无符号16位整型指针 填充数组
输出参数: 无
返回值:   void
************************************************/
void LCD_Color_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u32 *color)
{
	u32 psx,psy,pex,pey;	    				 //以LCD面板物理坐标
	u32 timeout=0; 
	u16 offline;
	u32 addr;
	
#if LCD_INTERFACE == INTERFACE_MIPI  //DSI接口屏 无需进行坐标变换
	psx = sx;
	psy = sy;
	pex = ex;
	pey = ey;
	
#else 															 //RGB接口屏 坐标系转换
/*if(g_tLCDOptr.orientation)  			 //横屏
	{
		psx = sx;
		psy = sy;
		pex = ex;
		pey = ey;
	}
	else											   			 //竖屏
	{
		psx = sy;
		psy = g_tLCDOptr.param->pheight-ex-1;
		pex = ey;
		pey = g_tLCDOptr.param->pheight-sx-1;
	}*/
#endif	
	offline = g_tLCDOptr.param->pwidth-(pex-psx+1);
	addr    = ((u32)g_tLCDOptr.buf_addr[g_tLCDOptr.activelayer]+g_tLCDOptr.param->pixsize*(g_tLCDOptr.param->pwidth*psy+psx));
	
	__HAL_RCC_DMA2D_CLK_ENABLE();							//使能DM2D时钟
	DMA2D->CR&=~(DMA2D_CR_START);							//先停止DMA2D
	DMA2D->CR=DMA2D_M2M;											//存储器到存储器模式
	DMA2D->FGPFCCR=LCD_PIXFORMAT;							//设置颜色格式
	DMA2D->FGOR=0;														//前景层行偏移为0
	DMA2D->OOR=offline;												//设置行偏移 
	DMA2D->FGMAR=(u32)color;									//源地址
	DMA2D->OMAR=addr;													//输出存储器地址
	DMA2D->NLR=(pey-psy+1)|((pex-psx+1)<<16);	//设定行数寄存器 
	DMA2D->CR|=DMA2D_CR_START;								//启动DMA2D
	while((DMA2D->ISR&(DMA2D_FLAG_TC))==0)		//等待传输完成
	{
		timeout++;
		if(timeout>0X1FFFFF)break;							//超时退出
	} 
	DMA2D->IFCR|=DMA2D_FLAG_TC;								//清除传输完成标志  	
}  

/************************************************
名    称: LCD_DisplayOn
功    能: LCD显示开启
输入参数: 无
输出参数: 无
返回值:   void
************************************************/
void LCD_DisplayOn(void)
{					   
	HAL_DSI_ShortWrite(&DSI_Handler,LCD_OTM8009A_ID,DSI_DCS_SHORT_PKT_WRITE_P1,OTM8009A_CMD_DISPON,0x00);
}	 

/************************************************
名    称: LCD_DisplayOff
功    能: LCD显示关闭
输入参数: 无
输出参数: 无
返回值:   void
************************************************/
void LCD_DisplayOff(void)
{	   
	HAL_DSI_ShortWrite(&DSI_Handler,LCD_OTM8009A_ID,DSI_DCS_SHORT_PKT_WRITE_P1,OTM8009A_CMD_DISPOFF,0x00);
}   

/************************************************
名    称: LCD_SetBrightness
功    能: LCD背光亮度设置
输入参数: BrightnessValue--无符号8位整型 亮度值0~100
输出参数: 无
返回值:   void
************************************************/
void LCD_SetBrightness(u8 BrightnessValue)
{
	HAL_DSI_ShortWrite(&DSI_Handler,LCD_OTM8009A_ID,DSI_DCS_SHORT_PKT_WRITE_P1,OTM8009A_CMD_WRDISBV,(u16)(BrightnessValue * 255)/100);
}
/************************************************
名    称: LCD_Clear
功    能: LCD清屏函数
输入参数: color--无符号32位整型 清屏颜色
输出参数: 无
返回值:   void
************************************************/
void LCD_Clear(u32 color)
{
	LCD_Fill(0,0,g_tLCDOptr.width-1,g_tLCDOptr.height-1,color);
}

/************************************************
名    称: LCD_DrawLine
功    能: LCD画线函数
输入参数: x1--无符号16位整型 线段起始横坐标
          y1--无符号16位整型 线段起始纵坐标
          x2--无符号16位整型 线段结束横坐标
          y2--无符号16位整型 线段结束纵坐标
输出参数: 无
返回值:   void
************************************************/
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)
{
	u16 t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance; 
	int incx,incy,uRow,uCol; 
	delta_x=x2-x1; //计算坐标增量 
	delta_y=y2-y1; 
	uRow=x1; 
	uCol=y1; 
	if(delta_x>0)incx=1; //设置单步方向 
	else if(delta_x==0)incx=0;//垂直线 
	else {incx=-1;delta_x=-delta_x;} 
	if(delta_y>0)incy=1; 
	else if(delta_y==0)incy=0;//水平线 
	else{incy=-1;delta_y=-delta_y;} 
	if( delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴 
	else distance=delta_y; 
	for(t=0;t<=distance+1;t++ )//画线输出 
	{  
		LCD_Draw_Point(uRow,uCol,g_tLCDOptr.pen_color);//画点 
		xerr+=delta_x ; 
		yerr+=delta_y ; 
		if(xerr>distance) 
		{ 
			xerr-=distance; 
			uRow+=incx; 
		} 
		if(yerr>distance) 
		{ 
			yerr-=distance; 
			uCol+=incy; 
		} 
	}  
}    

/************************************************
名    称: LCD_DrawRectangle
功    能: LCD画矩形
输入参数: x1--无符号16位整型 矩形左上角横坐标
          y1--无符号16位整型 矩形左上角纵坐标
          x2--无符号16位整型 矩形右下角横坐标
          y2--无符号16位整型 矩形右下角纵坐标
输出参数: 无
返回值:   void
************************************************/
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2)
{
	LCD_DrawLine(x1,y1,x2,y1);
	LCD_DrawLine(x1,y1,x1,y2);
	LCD_DrawLine(x1,y2,x2,y2);
	LCD_DrawLine(x2,y1,x2,y2);
}

/************************************************
名    称: LCD_Draw_Circle
功    能: LCD Bresenham算法画圆     
输入参数: x0--无符号16位整型 圆心横坐标
          y0--无符号16位整型 圆心纵坐标
          r--无符号8位整型 半径
输出参数: 无
返回值:   void
************************************************/
void LCD_Draw_Circle(u16 x0,u16 y0,u8 r)
{
	int a,b;
	int di;
	a=0;b=r;	  
	di=3-(r<<1);             //判断下个点位置的标志
	while(a<=b)
	{
		LCD_Draw_Point(x0+a,y0-b,g_tLCDOptr.pen_color);             //5
 		LCD_Draw_Point(x0+b,y0-a,g_tLCDOptr.pen_color);             //0           
		LCD_Draw_Point(x0+b,y0+a,g_tLCDOptr.pen_color);             //4               
		LCD_Draw_Point(x0+a,y0+b,g_tLCDOptr.pen_color);             //6 
		LCD_Draw_Point(x0-a,y0+b,g_tLCDOptr.pen_color);             //1       
 		LCD_Draw_Point(x0-b,y0+a,g_tLCDOptr.pen_color);             
		LCD_Draw_Point(x0-a,y0-b,g_tLCDOptr.pen_color);             //2             
  	LCD_Draw_Point(x0-b,y0-a,g_tLCDOptr.pen_color);             //7     	         
		a++;
		
		if(di<0)di +=4*a+6;	  
		else
		{
			di+=10+4*(a-b);   
			b--;
		} 						    
	}
} 									  

/************************************************
名    称: LCD_ShowChar
功    能: LCD显示单字符
输入参数: x--无符号16位整型 横坐标
          y--无符号16位整型 纵坐标
          num--无符号8位整型 要显示的字符
					size--无符号8位整型 字体大小  12:1206 16:1608 24:2412 32:3216
					mode--无符号8位整型 0:非叠加显示 1:叠加显示
输出参数: 无
返回值:   无
************************************************/
void LCD_ShowChar(u16 x,u16 y,u8 num,u8 size,u8 mode)
{  							  
    u8 temp,t1,t;
	u16 y0=y;
	u8 csize=(size/8+((size%8)?1:0))*(size/2);		//得到字体一个字符对应点阵集所占的字节数	
 	num=num-' ';//得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库）
	for(t=0;t<csize;t++)
	{   
		if(size==12)temp=asc2_1206[num][t]; 	 	//调用1206字体
		else if(size==16)temp=asc2_1608[num][t];	//调用1608字体
		else if(size==24)temp=asc2_2412[num][t];	//调用2412字体
		else if(size==32)temp=asc2_3216[num][t];	//调用3216字体
		else return;								//没有的字库
		for(t1=0;t1<8;t1++)
		{			    
			if(temp&0x80)LCD_Draw_Point(x,y,g_tLCDOptr.pen_color);
			else if(mode==0)LCD_Draw_Point(x,y,g_tLCDOptr.back_color);
			temp<<=1;
			y++;
			if(y>=g_tLCDOptr.height)return;		//超区域了
			if((y-y0)==size)
			{
				y=y0;
				x++;
				if(x>=g_tLCDOptr.width)return;	//超区域了
				break;
			}
		}  	 
	}  	    	   	 	  
}   

/************************************************
名    称: LCD_Pow
功    能: 计算m的n次幂
输入参数: m--无符号8位整型 底数
          n--无符号8位整型 指数
输出参数: 无
返回值:   无符号32位整型 结果
************************************************/
static u32 LCD_Pow(u8 m,u8 n)
{
	u32 result=1;	 
	while(n--)result*=m;    
	return result;
}			 

/************************************************
名    称: LCD_ShowNum
功    能: LCD显示数字
输入参数: x--无符号16位整型 横坐标
          y--无符号16位整型 纵坐标
          num--无符号32位整型 要显示的数字
					size--无符号8位整型 字体大小  12:1206 16:1608 24:2412 32:3216
输出参数: 无
返回值:   无
************************************************/
void LCD_ShowNum(u16 x,u16 y,u32 num,u8 len,u8 size)
{         	
	u8 t,temp;
	u8 enshow=0;						   
	for(t=0;t<len;t++)
	{
		temp=(num/LCD_Pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				LCD_ShowChar(x+(size/2)*t,y,' ',size,0);
				continue;
			}else enshow=1; 
		 	 
		}
	 	LCD_ShowChar(x+(size/2)*t,y,temp+'0',size,0); 
	}
} 
/************************************************
名    称: LCD_ShowString
功    能: LCD显示字符串
输入参数: x--无符号16位整型 起始位置横坐标
          y--无符号16位整型 起始位置纵坐标
          width--无符号16位整型 显示宽度
          height--无符号16位整型 显示高度
					size--无符号8位整型 字体大小  12:1206 16:1608 24:2412 32:3216
					p--无符号8位整型指针 字符串
输出参数: 无
返回值:   无
************************************************/	  
void LCD_ShowString(u16 x,u16 y,u16 width,u16 height,u8 size,u8 *p)
{         
	u8 x0=x;
	width+=x;
	height+=y;
    while((*p<='~')&&(*p>=' '))//判断是不是非法字符!
    {
        if(x>=width){x=x0;y+=size;}
        if(y>=height)break;//退出
        LCD_ShowChar(x,y,*p,size,0);
        x+=size/2;
        p++;
    }  
}

/************************************************
名    称: Get_HzMat
功    能: 获取指定字体的汉字点阵数据
输入参数: code--无符号8位整型指针 汉字GBK码
          mat--无符号16位整型指针 点阵数据
					size--无符号8位整型 字体大小  12:1206 16:1608 24:2412 32:3216
输出参数: 无
返回值:   无
************************************************/	 
void Get_HzMat(unsigned char *code,unsigned char *mat,u8 size)
{		    
	unsigned char qh,ql;
	unsigned char i;					  
	unsigned long foffset; 
	u8 csize = (size/8+((size%8)?1:0))*(size);	//得到字体一个字符对应点阵集所占的字节数	 
	
	qh = *code;
	ql = *(++code);
	
	if(qh<0x81||ql<0x40||ql==0xff||qh==0xff)		//非常用汉字
	{   		    
	    for(i=0;i<csize;i++)*mat++=0x00;				//填充满格
	    return; 
	}
	
	if(ql < 0x7f)
	{
		ql -= 0x40;
	}
	else 
	{
		ql -= 0x41;
	}
	qh -= 0x81;
	
	foffset = ((unsigned long)190 * qh + ql) * csize;	//得到字库中的字节偏移量  		  
	switch(size)
	{
		case 12:
			MX25L512_Read(mat,foffset+ftinfo.f12addr,csize);
			break;
		case 16:
			MX25L512_Read(mat,foffset+ftinfo.f16addr,csize);
			break;
		case 24:
			MX25L512_Read(mat,foffset+ftinfo.f24addr,csize);
			break;
		case 32:
			MX25L512_Read(mat,foffset+ftinfo.f32addr,csize);
			break;
		default:break;
	}     												    
} 

/************************************************
名    称: LCD_ShowFont
功    能: LCD显示汉字
输入参数: x--无符号16位整型 起始位置横坐标
          y--无符号16位整型 起始位置纵坐标
          font--无符号8位整型指针 汉字GBK码
					size--无符号8位整型 字体大小  12:1206 16:1608 24:2412 32:3216
					mode--无符号8位整型指针 0:非叠加显示 1:叠加显示
输出参数: 无
返回值:   无
************************************************/	 
void LCD_ShowFont(u16 x,u16 y,u8 *font,u8 size,u8 mode)
{
	u8 temp,t,t1;
	u16 y0 = y;
	u8 dzk[128];	
	u8 csize = (size/8+((size%8)?1:0))*(size);								//得到字体一个字符对应点阵集所占的字节数
	
	if(size!=12&&size!=16&&size!=24&&size!=32)
	{
		return;																									//不支持的size
	}
	
	Get_HzMat(font,dzk,size);																	//得到相应大小的点阵数据 
	
	for(t=0;t<csize;t++)
	{   												   
		temp = dzk[t];																				  //得到点阵数据                          
		for(t1 = 0;t1 < 8;t1++)
		{
			if(temp&0x80)LCD_Draw_Point(x,y,g_tLCDOptr.pen_color);
			else if(mode==0)LCD_Draw_Point(x,y,g_tLCDOptr.back_color); 
			temp<<=1;
			y++;
			if((y-y0)==size)
			{
				y=y0;
				x++;
				break;
			}
		}  	 
	}  
}


/************************************************
名    称: LCD_ShowStream
功    能: LCD显示字符串流 包括ASCII码和汉字
输入参数: x--无符号16位整型 起始位置横坐标
          y--无符号16位整型 起始位置纵坐标
          width--无符号16位整型 显示宽度
          height--无符号16位整型 显示高度
          str--无符号8位整型指针 字符串
					size--无符号8位整型 字体大小  12:1206 16:1608 24:2412 32:3216
输出参数: 无
返回值:   无
************************************************/	 
void LCD_ShowStream(u16 x,u16 y,u16 width,u16 height,u8* str,u8 size)
{											  	  
  u8 bHz=0;     
	
	while((*str!=0)&&(*str!='\n')&&(*str!='\r'))
	{ 
		if(!bHz)
		{
			if(*str > 0x80)
			{
				bHz=1;												//中文
			}				
			else              							//字符
			{      							    					
				LCD_ShowChar(x,y,*str,size,0);
				str++; 
				x += size/2; 								  //字符,为全字的一半 
			}
		}
		else															//中文 
		{     
			bHz=0;		
			LCD_ShowFont(x,y,str,size,0);  
			str += 2; 
			x += size;										  //下一个汉字偏移	    
		}						 
	}   
}

/************************************************
名    称: LCD_printf
功    能: LCD打印格式化字符串
输入参数: format--const字符型指针 格式化的字符串
          ...--可变参数
输出参数: 无
返回值:   无
************************************************/
void LCD_printf(const char *format, ...)
{
	u8 num;
  char tmp[74];  //字体大小16   600/8=75个
  va_list arg; 
  va_start(arg, format); 
  num = vsprintf(tmp,format,arg);
  va_end(arg);

  if(g_tLCDOptr.cur_line > (g_tLCDOptr.height/g_tLCDOptr.font_size))
	{
		g_tLCDOptr.cur_line = 1;
		LCD_Clear(g_tLCDOptr.back_color);
	}
	LCD_ShowStream(g_tLCDOptr.cur_pos,(g_tLCDOptr.cur_line-1)*g_tLCDOptr.font_size,g_tLCDOptr.width - 8,g_tLCDOptr.font_size,(u8 *)tmp,g_tLCDOptr.font_size);
	if((tmp[num-1]=='\n')&&(tmp[num-2]=='\r'))
	{
	 g_tLCDOptr.cur_line++;
	 g_tLCDOptr.cur_pos = 8;
	}
	else if(tmp[num-1]=='\r')
	{
		g_tLCDOptr.cur_pos = 8;
	}
	else
	{
   g_tLCDOptr.cur_pos += num*(g_tLCDOptr.font_size/2);
	 if(g_tLCDOptr.cur_pos > g_tLCDOptr.width)
	 {
		 g_tLCDOptr.cur_pos = 8;
		 g_tLCDOptr.cur_line++;
	 }
	}
}

/************************************************
名    称: Get_LCDOptr
功    能: 获取LCD操作实例
输入参数: 无
输出参数: 无
返回值:   PT_LCDOptr--LCD操作实例指针
************************************************/
PT_LCDOptr Get_LCDOptr(void)
{
	PT_LCDOptr pTtemp;
	pTtemp = &g_tLCDOptr;
	return pTtemp;
}
