#include "sdcard.h"
#include "string.h"

__align(4) u8 SDCard_temp_buffer[512];

static u8 SDCard_Init(void);
static u8 SDCard_Read_DMA(u32 *pData, u32 ReadAddr, u32 NumOfBlocks);
static u8 SDCard_Write_DMA(u32 *pData, u32 WriteAddr, u32 NumOfBlocks);
static u8 SD_ReadDisk(u8* buf,u32 sector,u32 cnt);
static u8 SD_WriteDisk(u8* buf,u32 sector,u32 cnt);

static T_SDCardOptr g_tSDCardOptr = {
	.RD_finish = FALSE,
	.WR_finish = FALSE,
	.init   = SDCard_Init,
	.read   = SD_ReadDisk,
	.write  = SD_WriteDisk,
};

static bool SDCard_Is_Detected(void)
{
  if(HAL_GPIO_ReadPin(GPIOI, GPIO_PIN_15) == GPIO_PIN_RESET)
  {
    return TRUE;
  }
  return FALSE;
}

static void SDCard_Detect_Pin_Init(void)
{
  GPIO_InitTypeDef GPIO_Initure;
	GPIO_Initure.Pin=GPIO_PIN_15;  
	GPIO_Initure.Mode=GPIO_MODE_INPUT;  			//通用推完输出
	GPIO_Initure.Pull=GPIO_PULLUP;            //上拉
	GPIO_Initure.Speed=GPIO_SPEED_HIGH;       //高速
	HAL_GPIO_Init(GPIOI,&GPIO_Initure);
}

static SD_HandleTypeDef SDCard_Handler;
static u8 SDCard_Init(void)
{
	  SDCard_Detect_Pin_Init();
			
		if(SDCard_Is_Detected() == FALSE)
		{
			return SDCARD_NO_DETECTED;
		}
	
		SDCard_Handler.Instance 								= SDMMC2;
    SDCard_Handler.Init.ClockEdge           = SDMMC_CLOCK_EDGE_RISING;               //时钟上升沿采样
    SDCard_Handler.Init.ClockBypass         = SDMMC_CLOCK_BYPASS_DISABLE;            //时钟旁路禁止，采用分频模式
    SDCard_Handler.Init.ClockPowerSave      = SDMMC_CLOCK_POWER_SAVE_DISABLE;        //时钟空闲时保持
    SDCard_Handler.Init.BusWide             = SDMMC_BUS_WIDE_1B;                     //1位数据模式
    SDCard_Handler.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;   //关闭硬件流控
    SDCard_Handler.Init.ClockDiv            = SDMMC_TRANSFER_CLK_DIV;                //SD数据传输最高25MHz
	
		if(HAL_SD_Init(&SDCard_Handler) != HAL_OK)
		{
			return SDCARD_ERROR;
		}
		
		if(HAL_SD_ConfigWideBusOperation(&SDCard_Handler, SDMMC_BUS_WIDE_4B) != HAL_OK)
    {
      return SDCARD_ERROR;
    }
		
		HAL_SD_GetCardInfo(&SDCard_Handler, &g_tSDCardOptr.info);
		
		return SDCARD_OK;
}

static DMA_HandleTypeDef SDCard_DMA_RX_Handler;
static DMA_HandleTypeDef SDCard_DMA_TX_Handler;

void HAL_SD_MspInit(SD_HandleTypeDef *hsd)
{
    GPIO_InitTypeDef GPIO_Initure;

     __HAL_RCC_SDMMC2_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();
      
    GPIO_Initure.Pin=GPIO_PIN_3|GPIO_PIN_4;
    GPIO_Initure.Mode=GPIO_MODE_AF_PP;      
    GPIO_Initure.Pull=GPIO_PULLUP;          
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;     
    GPIO_Initure.Alternate=GPIO_AF10_SDMMC2;  
    HAL_GPIO_Init(GPIOB,&GPIO_Initure);     
    
		GPIO_Initure.Pin=GPIO_PIN_6|GPIO_PIN_7;
    GPIO_Initure.Alternate=GPIO_AF11_SDMMC2;  
    HAL_GPIO_Init(GPIOD,&GPIO_Initure);     
	
		GPIO_Initure.Pin=GPIO_PIN_9|GPIO_PIN_10;
	  HAL_GPIO_Init(GPIOG,&GPIO_Initure);     

#if (SD_DMA_MODE==1)                        
	
	  HAL_NVIC_SetPriority(SDMMC2_IRQn, 3, 0);
		HAL_NVIC_EnableIRQ(SDMMC2_IRQn);  
	
    SDCard_DMA_RX_Handler.Instance=DMA2_Stream0;
    SDCard_DMA_RX_Handler.Init.Channel=DMA_CHANNEL_11;
    SDCard_DMA_RX_Handler.Init.Direction=DMA_PERIPH_TO_MEMORY;
    SDCard_DMA_RX_Handler.Init.PeriphInc=DMA_PINC_DISABLE;
    SDCard_DMA_RX_Handler.Init.MemInc=DMA_MINC_ENABLE;
    SDCard_DMA_RX_Handler.Init.PeriphDataAlignment=DMA_PDATAALIGN_WORD;
    SDCard_DMA_RX_Handler.Init.MemDataAlignment=DMA_MDATAALIGN_WORD;
    SDCard_DMA_RX_Handler.Init.Mode=DMA_PFCTRL;
    SDCard_DMA_RX_Handler.Init.Priority=DMA_PRIORITY_VERY_HIGH;
    SDCard_DMA_RX_Handler.Init.FIFOMode=DMA_FIFOMODE_ENABLE;
    SDCard_DMA_RX_Handler.Init.FIFOThreshold=DMA_FIFO_THRESHOLD_FULL;
    SDCard_DMA_RX_Handler.Init.MemBurst=DMA_MBURST_INC4;
    SDCard_DMA_RX_Handler.Init.PeriphBurst=DMA_PBURST_INC4;

    __HAL_LINKDMA(hsd, hdmarx, SDCard_DMA_RX_Handler); //和SD卡操作句柄建立连接
    HAL_DMA_DeInit(&SDCard_DMA_RX_Handler);
    HAL_DMA_Init(&SDCard_DMA_RX_Handler);              //初始化接收DMA
    
    SDCard_DMA_TX_Handler.Instance=DMA2_Stream5;
    SDCard_DMA_TX_Handler.Init.Channel=DMA_CHANNEL_11;
    SDCard_DMA_TX_Handler.Init.Direction=DMA_MEMORY_TO_PERIPH;
    SDCard_DMA_TX_Handler.Init.PeriphInc=DMA_PINC_DISABLE;
    SDCard_DMA_TX_Handler.Init.MemInc=DMA_MINC_ENABLE;
    SDCard_DMA_TX_Handler.Init.PeriphDataAlignment=DMA_PDATAALIGN_WORD;
    SDCard_DMA_TX_Handler.Init.MemDataAlignment=DMA_MDATAALIGN_WORD;
    SDCard_DMA_TX_Handler.Init.Mode=DMA_PFCTRL;
    SDCard_DMA_TX_Handler.Init.Priority=DMA_PRIORITY_VERY_HIGH;
    SDCard_DMA_TX_Handler.Init.FIFOMode=DMA_FIFOMODE_ENABLE;
    SDCard_DMA_TX_Handler.Init.FIFOThreshold=DMA_FIFO_THRESHOLD_FULL;
    SDCard_DMA_TX_Handler.Init.MemBurst=DMA_MBURST_INC4;
    SDCard_DMA_TX_Handler.Init.PeriphBurst=DMA_PBURST_INC4;
    
    __HAL_LINKDMA(hsd, hdmatx, SDCard_DMA_TX_Handler); //和SD卡操作句柄建立连接
    HAL_DMA_DeInit(&SDCard_DMA_TX_Handler);
    HAL_DMA_Init(&SDCard_DMA_TX_Handler);              //初始化发送DMA 
  

    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 3, 0);  	 //接收DMA中断优先级
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
    HAL_NVIC_SetPriority(DMA2_Stream5_IRQn, 3, 0);  	 //发送DMA中断优先级
    HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);
#endif
}

#if (SD_DMA_MODE==1) 

static u8 SDCard_Read_DMA(u32 *pData, u32 ReadAddr, u32 NumOfBlocks)
{
	while(HAL_SD_GetCardState(&SDCard_Handler) != HAL_SD_CARD_TRANSFER);
	
	SCB_CleanInvalidateDCache();
	if(HAL_SD_ReadBlocks_DMA(&SDCard_Handler, (u8 *)pData, ReadAddr, NumOfBlocks) != HAL_OK)
  {
    return SDCARD_ERROR;
  }
  else
  {
		while(g_tSDCardOptr.RD_finish == FALSE);
		g_tSDCardOptr.RD_finish  = FALSE;
    return SDCARD_OK;
  }
}

static u8 SDCard_Write_DMA(u32 *pData, u32 WriteAddr, u32 NumOfBlocks)
{
	while(HAL_SD_GetCardState(&SDCard_Handler) != HAL_SD_CARD_TRANSFER);
	
	SCB_CleanInvalidateDCache();
	if(HAL_SD_WriteBlocks_DMA(&SDCard_Handler, (u8 *)pData, WriteAddr, NumOfBlocks) != HAL_OK)
  {
    return SDCARD_ERROR;
  }
  else
  {
		while(g_tSDCardOptr.WR_finish == FALSE);
		g_tSDCardOptr.WR_finish  = FALSE;
    return SDCARD_OK;
  }
}

static u8 SD_ReadDisk(u8* buf,u32 sector,u32 cnt)
{
    u8 sta = SDCARD_OK;
    u32 cur_sector;
    u32 i;

		cur_sector = sector;
	
    if((u32)buf%4 != 0)                //缓冲区地址不是4字节对齐的
    {
        for(i = 0;i < cnt;i++)
        {
            sta = SDCard_Read_DMA((u32 *)SDCard_temp_buffer,cur_sector + i,1);
            memcpy(buf,SDCard_temp_buffer,512);
            buf += 512;
        }
    }
		else
    {
        sta = SDCard_Read_DMA((uint32_t*)buf,cur_sector,cnt);
    }
    return sta;
}

static u8 SD_WriteDisk(u8* buf,u32 sector,u32 cnt)
{
    u8 sta = SDCARD_OK;
    u32 cur_sector;
    u32 i;

		cur_sector = sector;
	
    if((u32)buf%4 != 0)                //缓冲区地址不是4字节对齐的
    {
        for(i = 0;i < cnt;i++)
        {
					  memcpy(SDCard_temp_buffer,buf,512);
            sta = SDCard_Write_DMA((u32 *)SDCard_temp_buffer,cur_sector + i,1);          
            buf += 512;
        }
    }
		else
    {
        sta = SDCard_Write_DMA((uint32_t*)buf,cur_sector,cnt);
    }
    return sta;
} 


void DMA2_Stream0_IRQHandler(void)
{
   HAL_DMA_IRQHandler(SDCard_Handler.hdmarx);
}

void DMA2_Stream5_IRQHandler(void)
{
   HAL_DMA_IRQHandler(SDCard_Handler.hdmatx);
}

void SDMMC2_IRQHandler(void)
{
  HAL_SD_IRQHandler(&SDCard_Handler);
}

void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd)
{
  SCB_CleanInvalidateDCache();
	g_tSDCardOptr.WR_finish = TRUE;
}

void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd)
{
  SCB_CleanInvalidateDCache();
	g_tSDCardOptr.RD_finish = TRUE;
}

#else 
static u8 SD_ReadDisk(u8* buf,u32 sector,u32 cnt)
{
    u8 sta = SDCARD_OK;
    u32 cur_sector;
    u32 i;

		cur_sector = sector;
	
    INTX_DISABLE();//关闭总中断(POLLING模式,严禁中断打断SDIO读写操作!!!)
    if((u32)buf%4!=0)
    {
        for(i=0;i<cnt;i++)
        {
            sta = HAL_SD_ReadBlocks(&SDCard_Handler,SDCard_temp_buffer,cur_sector + i,1,SD_TIMEOUT);//单个sector的读操作
            memcpy(buf,SDCard_temp_buffer,512);
            buf += 512;
        }
    }
		else
    {
        sta = HAL_SD_ReadBlocks(&SDCard_Handler,buf,cur_sector,cnt,SD_TIMEOUT);//单个sector的读操作
    }
    INTX_ENABLE();//开启总中断
    return sta;
}  

//写SD卡
//buf:写数据缓存区
//sector:扇区地址
//cnt:扇区个数	
//返回值:错误状态;0,正常;其他,错误代码;	
static u8 SD_WriteDisk(u8* buf,u32 sector,u32 cnt)
{   
    u8 sta = SDCARD_OK;
    u32 cur_sector;
    u32 i;
	
	  cur_sector = sector;
	
    INTX_DISABLE();//关闭总中断(POLLING模式,严禁中断打断SDIO读写操作!!!)
    if((u32)buf%4!=0)
    {
        for(i=0;i<cnt;i++)
        {
            memcpy(SDCard_temp_buffer,buf,512);
            sta = HAL_SD_WriteBlocks(&SDCard_Handler,SDCard_temp_buffer,cur_sector + i,1,SD_TIMEOUT);//单个sector的写操作
            buf+=512;
        }
    }
		else
    {
        sta = HAL_SD_WriteBlocks(&SDCard_Handler,SDCard_temp_buffer,cur_sector,cnt,SD_TIMEOUT);
    }
    INTX_ENABLE();//开启总中断
    return sta;
} 

#endif

PT_SDCardOptr Get_SDCardOptr(void)
{
	PT_SDCardOptr pTtemp;
	pTtemp = &g_tSDCardOptr;
	return pTtemp;
}

