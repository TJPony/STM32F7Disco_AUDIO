#include "usbd_usr.h"
#include "lcd.h"

vu8 bDeviceState=0;		//默认没有连接  
extern bool usb_flag;

extern USB_OTG_CORE_HANDLE  USB_OTG_dev;

//USB OTG 中断服务函数
//处理所有USB中断
void OTG_HS_IRQHandler(void)
{
  	USBD_OTG_ISR_Handler(&USB_OTG_dev);
}  
//指向DEVICE_PROP结构体
//USB Device 用户回调函数. 
USBD_Usr_cb_TypeDef USR_cb =
{
  USBD_USR_Init,
  USBD_USR_DeviceReset,
  USBD_USR_DeviceConfigured,
  USBD_USR_DeviceSuspended,
  USBD_USR_DeviceResumed,
  USBD_USR_DeviceConnected,
  USBD_USR_DeviceDisconnected,    
};
//USB Device 用户自定义初始化函数
void USBD_USR_Init(void)
{
	
} 
//USB Device 复位
//speed:USB速度,0,高速;1,全速;其他,错误.
void USBD_USR_DeviceReset (uint8_t speed)
{
	switch (speed)
	{
		case USB_OTG_SPEED_HIGH:
			LCD_printf("USB HS Device reset!\r\n");
			break; 
		case USB_OTG_SPEED_FULL: 
			LCD_printf("USB FS Device reset!\r\n");
			break;
		default:
			LCD_printf("USB Device Library v1.1.0  [??]\r\n"); 
			break;
	}
}
//USB Device 配置成功
void USBD_USR_DeviceConfigured (void)
{
  bDeviceState=1;
	usb_flag = TRUE;
	LCD_printf("CostumHID Interface started.\r\n"); 
} 
//USB Device挂起
void USBD_USR_DeviceSuspended(void)
{
  bDeviceState=0;
	LCD_printf("Device In suspend mode.\r\n");
} 
//USB Device恢复
void USBD_USR_DeviceResumed(void)
{ 
	LCD_printf("Device Resumed\r\n");
}
//USB Device连接成功
void USBD_USR_DeviceConnected (void)
{
	bDeviceState=1;
	LCD_printf("USB Device Connected.\r\n");
}
//USB Device未连接
void USBD_USR_DeviceDisconnected (void)
{
	bDeviceState=0;
	LCD_printf("USB Device Disconnected.\r\n");
} 
