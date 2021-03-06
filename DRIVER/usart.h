#ifndef _USART_H
#define _USART_H
#include "sys.h"
#include "stdio.h"	

#define UART_MAX_LEN 256

typedef struct t_COMOptr
{
	 bool isReFinish;
	 u8 rxbuf[UART_MAX_LEN];
	 u8 rxnum;
	 void (*init)(u32 bound);
	 void (*send)(u8 *pdata,u16 num);
}T_COMOptr,*PT_COMOptr;

PT_COMOptr Get_COMOptr(void);

#endif
