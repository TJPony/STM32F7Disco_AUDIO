#include "usart.h"

#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
void _sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{ 	
	while((USART6->ISR&0X40)==0);//循环发送,直到发送完毕   
	USART6->TDR = (u8) ch;      
	return ch;
}
#endif 


static void uart6_init(u32 bound);
static void uart6_send(u8 dat);
static void uart6_sendbuf(u8 *pdata,u16 num);

static T_COMOptr g_tComOptr ={
	 .isReFinish       = FALSE,
	 .rxnum            = 0,
	 .init             = uart6_init,
	 .send             = uart6_sendbuf,
};
static void uart6_send(u8 dat)
{
	while((USART6->ISR&0X40)==0);
	USART6->TDR = dat;      
}

static void uart6_sendbuf(u8 *pdata,u16 num)
{
	u16 i;
	for(i = 0;i < num;i++)
	{
		uart6_send(*(pdata+i));
	}
}


static UART_HandleTypeDef USART6_Handler; //UART句柄
static DMA_HandleTypeDef  hdma_uart6_rx;
//初始化IO 串口1 
//bound:波特率
static void uart6_init(u32 bound)
{	
	//UART 初始化设置
	USART6_Handler.Instance=USART6;					    //USART6
	USART6_Handler.Init.BaudRate=bound;				    //波特率
	USART6_Handler.Init.WordLength=UART_WORDLENGTH_8B;   //字长为8位数据格式
	USART6_Handler.Init.StopBits=UART_STOPBITS_1;	    //一个停止位
	USART6_Handler.Init.Parity=UART_PARITY_NONE;		    //无奇偶校验位
	USART6_Handler.Init.HwFlowCtl=UART_HWCONTROL_NONE;   //无硬件流控
	USART6_Handler.Init.Mode=UART_MODE_TX_RX;		    //收发模式
	HAL_UART_Init(&USART6_Handler);					    //HAL_UART_Init()会使能UART1

	HAL_DMA_Start_IT(&hdma_uart6_rx, (uint32_t)&USART6_Handler.Instance->RDR, (uint32_t)(g_tComOptr.rxbuf), UART_MAX_LEN);
	SET_BIT(USART6_Handler.Instance->CR3, USART_CR3_DMAR);
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    //GPIO端口设置
	GPIO_InitTypeDef GPIO_Initure;
	
	if(huart->Instance==USART6)							//如果是串口6，进行串口6 MSP初始化
	{
		__HAL_RCC_USART6_CLK_ENABLE();			    //使能USART6时钟
		__HAL_RCC_DMA2_CLK_ENABLE();
	
		GPIO_Initure.Pin=GPIO_PIN_6|GPIO_PIN_7;			    
		GPIO_Initure.Mode=GPIO_MODE_AF_PP;		//复用推挽输出
		GPIO_Initure.Pull=GPIO_PULLUP;			  //上拉
		GPIO_Initure.Speed=GPIO_SPEED_FREQ_HIGH;		//高速
		GPIO_Initure.Alternate=GPIO_AF8_USART6;	
		HAL_GPIO_Init(GPIOC,&GPIO_Initure);	  
    
		hdma_uart6_rx.Instance = DMA2_Stream1;
		hdma_uart6_rx.Init.Channel = DMA_CHANNEL_5;
    hdma_uart6_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_uart6_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_uart6_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_uart6_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_uart6_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_uart6_rx.Init.Mode = DMA_CIRCULAR;
    hdma_uart6_rx.Init.Priority = DMA_PRIORITY_HIGH;
		hdma_uart6_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_uart6_rx);

    __HAL_LINKDMA(huart,hdmarx,hdma_uart6_rx);
   
		HAL_NVIC_EnableIRQ(USART6_IRQn);				//使能USART6中断通道
		HAL_NVIC_SetPriority(USART6_IRQn,2,1);			//抢占优先级2，子优先级1
		__HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);
	}
}

void USART6_IRQHandler(void)                	
{ 
	if((__HAL_UART_GET_FLAG(&USART6_Handler,UART_FLAG_IDLE)!=RESET)) 
	{
		__HAL_UART_CLEAR_IT(&USART6_Handler,UART_FLAG_IDLE);            //清除中断 
		
		 g_tComOptr.rxnum = UART_MAX_LEN - hdma_uart6_rx.Instance->NDTR;
		 g_tComOptr.isReFinish = TRUE;
		 __HAL_UNLOCK(&hdma_uart6_rx);
		 hdma_uart6_rx.State = HAL_DMA_STATE_READY;
		 HAL_DMA_Start_IT(&hdma_uart6_rx, (uint32_t)&USART6_Handler.Instance->RDR, (uint32_t)(g_tComOptr.rxbuf), UART_MAX_LEN);
		 SCB_InvalidateDCache(); //使Catch无效 防止RAM和CACHE数据不一致
	}	
}  

PT_COMOptr Get_COMOptr(void)
{
	PT_COMOptr pTtemp;
	pTtemp = &g_tComOptr;
	return pTtemp;
}


