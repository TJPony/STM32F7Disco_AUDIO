#include "jpeghwdecode.h"
#include "lcd.h"
#include "file.h"
#include "malloc.h"

jpeg_codec_typedef Jpeg_HW_Decoder;

JPEG_HandleTypeDef  JPEG_Handler;    			//JPEG句柄
DMA_HandleTypeDef   JPEGDMAIN_Handler;		//JPEG数据输入DMA
DMA_HandleTypeDef   JPEGDMAOUT_Handler;		//JPEG数据输出DMA

static void JPEG_DMA_Start(void)
{ 
	__HAL_DMA_ENABLE(&JPEGDMAIN_Handler);		//打开JPEG数据输入DMA
	__HAL_DMA_ENABLE(&JPEGDMAOUT_Handler); 	//打开JPEG数据输出DMA 
	JPEG->CR|=3<<11; 												//使能输入输出DMA
}

static void JPEG_DMA_Stop(void)
{
	JPEG->CR     &= ~(3<<11); 					  	//禁止输入输出DMA
	JPEG->CONFR0 &= ~(1<<0);					    	//停止JPEG编解码
	__HAL_JPEG_DISABLE_IT(&JPEG_Handler,JPEG_IT_IFT|JPEG_IT_IFNF|JPEG_IT_OFT|JPEG_IT_OFNE|JPEG_IT_EOC|JPEG_IT_HPD);		//关闭所有中断  			
	JPEG->CFR     = 3<<5;										//清空标志  
}

static void JPEG_Core_Destroy(jpeg_codec_typedef *decoder)
{
	u8 i; 
	JPEG_DMA_Stop();												//停止DMA传输
	for(i = 0;i < NB_INPUT_DATA_BUFFERS;i++)
	{
		myfree(SRAM0,decoder->inbuf[i].buf);	//释放内存
	}
	for(i = 0;i < NB_OUTPUT_DATA_BUFFERS;i++)
	{
		myfree(SRAM0,decoder->outbuf[i].buf);	//释放内存
	}
}

static PIC_StatusTypeDef JPEG_Core_Init(jpeg_codec_typedef *decoder)
{
	u8 i;
	
	for(i = 0;i < NB_INPUT_DATA_BUFFERS;i++)
	{
		decoder->inbuf[i].buf = mymalloc(SRAM0,JPEG_DMA_INBUF_LEN);
		if(decoder->inbuf[i].buf == NULL)
		{
			JPEG_Core_Destroy(decoder);
			return PIC_MEM_ERR;
		}   
	} 
	for(i = 0;i < NB_OUTPUT_DATA_BUFFERS;i++)
	{
		decoder->outbuf[i].buf = mymalloc(SRAM0,JPEG_DMA_OUTBUF_LEN);
		if(decoder->outbuf[i].buf == NULL)		
		{
			JPEG_Core_Destroy(decoder);
			return PIC_MEM_ERR;
		}   
	}
	return PIC_NO_ERR;
}

void HAL_JPEG_MspInit(JPEG_HandleTypeDef *hjpeg)
{
	__HAL_RCC_JPEG_CLK_ENABLE();            //使能JPEG时钟
	
	HAL_NVIC_SetPriority(JPEG_IRQn,0,2);    //JPEG中断优先级
	HAL_NVIC_EnableIRQ(JPEG_IRQn);
	
	__HAL_RCC_DMA2_CLK_ENABLE();                                    		//使能DMA2时钟
	
	JPEGDMAIN_Handler.Instance 								 = DMA2_Stream3;          //DMA2数据流0                   
	JPEGDMAIN_Handler.Init.Channel						 = DMA_CHANNEL_9;         //通道9
	JPEGDMAIN_Handler.Init.Direction					 = DMA_MEMORY_TO_PERIPH;  //存储器到外设
	JPEGDMAIN_Handler.Init.PeriphInc					 = DMA_PINC_DISABLE;      //外设非增量模式
	JPEGDMAIN_Handler.Init.MemInc							 = DMA_MINC_ENABLE;       //存储器增量模式
	JPEGDMAIN_Handler.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;   //外设数据长度:32位
	JPEGDMAIN_Handler.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;   //存储器数据长度:32位
	JPEGDMAIN_Handler.Init.Mode								 = DMA_NORMAL;            //普通模式
	JPEGDMAIN_Handler.Init.Priority            = DMA_PRIORITY_HIGH;     //高优先级
	JPEGDMAIN_Handler.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;   //使能FIFO
	JPEGDMAIN_Handler.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;//全FIFO
	JPEGDMAIN_Handler.Init.MemBurst            = DMA_MBURST_INC4;       //存储器4节拍增量突发传输
	JPEGDMAIN_Handler.Init.PeriphBurst         = DMA_PBURST_INC4;       //外设4节拍增量突发传输
	
	__HAL_LINKDMA(hjpeg, hdmain, JPEGDMAIN_Handler);
	HAL_DMA_DeInit(&JPEGDMAIN_Handler);
	HAL_DMA_Init(&JPEGDMAIN_Handler);	                               		//初始化DMA
  
	HAL_NVIC_SetPriority(DMA2_Stream3_IRQn,0,3);  											//抢占2，子优先级3
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);        											//使能中断

	JPEGDMAOUT_Handler.Instance								 = DMA2_Stream4;          //DMA2数据流1                 
	JPEGDMAOUT_Handler.Init.Channel						 = DMA_CHANNEL_9;         //通道9
	JPEGDMAOUT_Handler.Init.Direction					 = DMA_PERIPH_TO_MEMORY;  //外设到存储器
	JPEGDMAOUT_Handler.Init.PeriphInc					 = DMA_PINC_DISABLE;      //外设非增量模式
	JPEGDMAOUT_Handler.Init.MemInc             = DMA_MINC_ENABLE;       //存储器增量模式
	JPEGDMAOUT_Handler.Init.PeriphDataAlignment= DMA_PDATAALIGN_WORD;   //外设数据长度:32位
	JPEGDMAOUT_Handler.Init.MemDataAlignment   = DMA_MDATAALIGN_WORD;   //存储器数据长度:32位
	JPEGDMAOUT_Handler.Init.Mode							 = DMA_NORMAL;            //普通模式
	JPEGDMAOUT_Handler.Init.Priority					 = DMA_PRIORITY_VERY_HIGH;//极高优先级
	JPEGDMAOUT_Handler.Init.FIFOMode					 = DMA_FIFOMODE_ENABLE;   //使能FIFO
	JPEGDMAOUT_Handler.Init.FIFOThreshold			 = DMA_FIFO_THRESHOLD_FULL;//全FIFO
	JPEGDMAOUT_Handler.Init.MemBurst					 = DMA_MBURST_INC4;       //存储器4节拍增量突发传输
	JPEGDMAOUT_Handler.Init.PeriphBurst				 = DMA_PBURST_INC4;       //外设4节拍增量突发传输
    
  __HAL_LINKDMA(hjpeg, hdmaout, JPEGDMAOUT_Handler);
	HAL_DMA_DeInit(&JPEGDMAOUT_Handler);
	HAL_DMA_Init(&JPEGDMAOUT_Handler);	                               	//初始化DMA
	
	HAL_NVIC_SetPriority(DMA2_Stream4_IRQn,0,3);  											//抢占2，子优先级3，组2
  HAL_NVIC_EnableIRQ(DMA2_Stream4_IRQn);        											//使能中断
}

static void JPEG_Decode_Init(jpeg_codec_typedef *decoder)
{ 
	u8 i;
	
	decoder->inbuf_read_ptr   = 0;
	decoder->inbuf_write_ptr  = 0;
	decoder->indma_pause      = 0;
	decoder->outbuf_read_ptr  = 0;
	decoder->outbuf_write_ptr = 0;	
	decoder->outdma_pause			= 0;	
	decoder->state						= JPEG_STATE_NOHEADER;		
	decoder->blkindex					=	0;			
	decoder->total_blks				= 0;		
	
	for(i = 0;i < NB_INPUT_DATA_BUFFERS;i++)
	{
		decoder->inbuf[i].sta   = JPEG_BUFFER_EMPTY;
		decoder->inbuf[i].size  = 0;
	}
	for(i = 0;i < NB_OUTPUT_DATA_BUFFERS;i++)
	{
		decoder->outbuf[i].sta  = JPEG_BUFFER_EMPTY;
		decoder->outbuf[i].size = 0;
	}	
}

u8 Jpeg_HW_Decode(u8* pname,u16 x,u16 y)
{
	FIL* ftemp; 
	u32* ARGB_buf;
	vu32 timecnt=0;
	u8 fileover=0;
	u32 i=0;
	u8 res;
	u32 mcublkindex=0;  
	u32 ConvertedDataCount;
	
	Jpeg_HW_Decoder.JpegProcessing_End = 0;
	
	res = JPEG_Core_Init(&Jpeg_HW_Decoder);						//初始化JPEG内核
	if(res)
	{
		return res;
	}
	
	ftemp = (FIL*)mymalloc(SRAM0,sizeof(FIL));	
  if(ftemp == NULL)
	{
		return PIC_MEM_ERR;
	}	
	
	if(f_open(ftemp,(char*)pname,FA_READ) != FR_OK)	  //打开图片失败
  {
		JPEG_Core_Destroy(&Jpeg_HW_Decoder);            
		myfree(SRAM0,ftemp);														//释放内存
		return PIC_FILE_ERR;
	} 
	
	ARGB_buf = mymalloc(SRAM1,800*480*4);						  //从外部SDRAM申请整帧内存
	
	JPEG_Decode_Init(&Jpeg_HW_Decoder);								//初始化硬件JPEG解码器
	  	
	for(i = 0;i < NB_INPUT_DATA_BUFFERS;i++)
	{
		res = f_read(ftemp,Jpeg_HW_Decoder.inbuf[i].buf,JPEG_DMA_INBUF_LEN,(u32 *)&Jpeg_HW_Decoder.inbuf[i].size);//填满所有输入数据缓冲区
		if(res == FR_OK && Jpeg_HW_Decoder.inbuf[i].size)
		{
			Jpeg_HW_Decoder.inbuf[i].sta = JPEG_BUFFER_FULL;																									//标记buf满 
		}
		else if(Jpeg_HW_Decoder.inbuf[i].size == 0)                                                         //如果已经读取完毕则直接跳出for循环
		{
			break;
		}			
	}
	HAL_JPEG_Decode_DMA(&JPEG_Handler,Jpeg_HW_Decoder.inbuf[0].buf,Jpeg_HW_Decoder.inbuf[0].size,Jpeg_HW_Decoder.outbuf[0].buf,JPEG_DMA_OUTBUF_LEN);
	do
	{
			if(Jpeg_HW_Decoder.inbuf[Jpeg_HW_Decoder.inbuf_write_ptr].sta == JPEG_BUFFER_EMPTY)
			{
				if(f_read(ftemp,Jpeg_HW_Decoder.inbuf[Jpeg_HW_Decoder.inbuf_write_ptr].buf,JPEG_DMA_INBUF_LEN,(u32*)&(Jpeg_HW_Decoder.inbuf[Jpeg_HW_Decoder.inbuf_write_ptr].size)) == FR_OK)
				{  
					Jpeg_HW_Decoder.inbuf[Jpeg_HW_Decoder.inbuf_write_ptr].sta = JPEG_BUFFER_FULL;
				}
				else
				{
					break;
				}
				
				if((Jpeg_HW_Decoder.indma_pause == 1) && (Jpeg_HW_Decoder.inbuf[Jpeg_HW_Decoder.inbuf_read_ptr].sta == JPEG_BUFFER_FULL))
				{
					Jpeg_HW_Decoder.indma_pause = 0;
					HAL_JPEG_ConfigInputBuffer(&JPEG_Handler,Jpeg_HW_Decoder.inbuf[Jpeg_HW_Decoder.inbuf_read_ptr].buf,Jpeg_HW_Decoder.inbuf[Jpeg_HW_Decoder.inbuf_read_ptr].size);    			
					HAL_JPEG_Resume(&JPEG_Handler,JPEG_PAUSE_RESUME_INPUT); 
				}
				
				Jpeg_HW_Decoder.inbuf_write_ptr++;
				if(Jpeg_HW_Decoder.inbuf_write_ptr >= NB_INPUT_DATA_BUFFERS)
				{
					Jpeg_HW_Decoder.inbuf_write_ptr = 0;
				}            
			}
  
			if(Jpeg_HW_Decoder.outbuf[Jpeg_HW_Decoder.outbuf_read_ptr].sta == JPEG_BUFFER_FULL)
			{  
				mcublkindex += Jpeg_HW_Decoder.yuv2rgb(Jpeg_HW_Decoder.outbuf[Jpeg_HW_Decoder.outbuf_read_ptr].buf,(u8 *)ARGB_buf,mcublkindex,Jpeg_HW_Decoder.outbuf[Jpeg_HW_Decoder.outbuf_read_ptr].size,&ConvertedDataCount);   
				
				Jpeg_HW_Decoder.outbuf[Jpeg_HW_Decoder.outbuf_read_ptr].sta  = JPEG_BUFFER_EMPTY;
				Jpeg_HW_Decoder.outbuf[Jpeg_HW_Decoder.outbuf_read_ptr].size = 0;
				
				Jpeg_HW_Decoder.outbuf_read_ptr++;
				if(Jpeg_HW_Decoder.outbuf_read_ptr >= NB_OUTPUT_DATA_BUFFERS)
				{
					Jpeg_HW_Decoder.outbuf_read_ptr = 0;
				}
				
				if(mcublkindex == Jpeg_HW_Decoder.total_blks)
				{
					Jpeg_HW_Decoder.JpegProcessing_End = 1;
				}
			}
			else if((Jpeg_HW_Decoder.outdma_pause == 1) && (Jpeg_HW_Decoder.outbuf[Jpeg_HW_Decoder.outbuf_write_ptr].sta == JPEG_BUFFER_EMPTY))
			{
				Jpeg_HW_Decoder.outdma_pause = 0;
				HAL_JPEG_Resume(&JPEG_Handler, JPEG_PAUSE_RESUME_OUTPUT);            
			}
	}while(Jpeg_HW_Decoder.JpegProcessing_End == 0);
	
  HAL_JPEG_GetInfo(&JPEG_Handler, &Jpeg_HW_Decoder.info);
	for(i = 0;i<800*400;i++)
	{
		ARGB_buf[i] |= 0xFF000000;
	}
	LCD_Color_Fill(x,y,x + Jpeg_HW_Decoder.info.ImageWidth-1,y + Jpeg_HW_Decoder.info.ImageHeight-1,ARGB_buf);
	
	myfree(SRAM0,ftemp);
	myfree(SRAM1,ARGB_buf);	
	JPEG_Core_Destroy(&Jpeg_HW_Decoder); 
	
	return 0;
}

PIC_StatusTypeDef Show_Picture(const u8 *filename,u16 x,u16 y,u16 width,u16 height)
{	
	PIC_StatusTypeDef res;
	u8 filetype;	
	PT_LCDOptr pt_lcd;
	
	pt_lcd = Get_LCDOptr();
	
	if((x + width) > pt_lcd->width)
	{
		return PIC_WINDOW_ERR;
	}
	if((y + height) > pt_lcd->height)
	{
		return PIC_WINDOW_ERR;
	}		
	if(width == 0 || height == 0)
	{
		return PIC_WINDOW_ERR;
	}
	 
	filetype = f_typetell((u8*)filename);	
	
	switch(filetype)
	{											  
		case T_BMP:
			/* 解码bmp */  	  
			break;
		case T_JPG:
		case T_JPEG:
			/* 解码jpeg */  	  
				res = Jpeg_HW_Decode((u8*)filename,x,y);//采用硬解码JPG/JPEG
			break;
		case T_GIF:
			/* 解码gif*/
			break;
		default:
	 		res = PIC_FORMAT_ERR;  						
			break;
	}  											   
	return res;
}

void JPEG_IRQHandler(void)
{
  HAL_JPEG_IRQHandler(&JPEG_Handler);
}

void DMA2_Stream3_IRQHandler(void)
{
  HAL_DMA_IRQHandler(JPEG_Handler.hdmain);
}

void DMA2_Stream4_IRQHandler(void)
{
  HAL_DMA_IRQHandler(JPEG_Handler.hdmaout);
}

void HAL_JPEG_InfoReadyCallback(JPEG_HandleTypeDef *hjpeg, JPEG_ConfTypeDef *pInfo)
{
  if(JPEG_GetDecodeColorConvertFunc(pInfo, &Jpeg_HW_Decoder.yuv2rgb, &Jpeg_HW_Decoder.total_blks) != HAL_OK)
  {
		LCD_printf("JPEG_GetDecodeColorConvertFunc error! \r\n");
	}  
}

void HAL_JPEG_GetDataCallback(JPEG_HandleTypeDef *hjpeg, uint32_t NbDecodedData)
{
  if(NbDecodedData == Jpeg_HW_Decoder.inbuf[Jpeg_HW_Decoder.inbuf_read_ptr].size)
  {  
    Jpeg_HW_Decoder.inbuf[Jpeg_HW_Decoder.inbuf_read_ptr].sta  = JPEG_BUFFER_EMPTY;
    Jpeg_HW_Decoder.inbuf[Jpeg_HW_Decoder.inbuf_read_ptr].size = 0;
  
    Jpeg_HW_Decoder.inbuf_read_ptr++;
    if(Jpeg_HW_Decoder.inbuf_read_ptr >= NB_INPUT_DATA_BUFFERS)
    {
      Jpeg_HW_Decoder.inbuf_read_ptr = 0;        
    }
  
    if(Jpeg_HW_Decoder.inbuf[Jpeg_HW_Decoder.inbuf_read_ptr].sta == JPEG_BUFFER_EMPTY)
    {
      HAL_JPEG_Pause(hjpeg, JPEG_PAUSE_RESUME_INPUT);
      Jpeg_HW_Decoder.indma_pause = 1;
    }
    else
    {    
      HAL_JPEG_ConfigInputBuffer(hjpeg,Jpeg_HW_Decoder.inbuf[Jpeg_HW_Decoder.inbuf_read_ptr].buf,Jpeg_HW_Decoder.inbuf[Jpeg_HW_Decoder.inbuf_read_ptr].size);    
    }
  }
  else
  {
    HAL_JPEG_ConfigInputBuffer(hjpeg,Jpeg_HW_Decoder.inbuf[Jpeg_HW_Decoder.inbuf_read_ptr].buf + NbDecodedData, Jpeg_HW_Decoder.inbuf[Jpeg_HW_Decoder.inbuf_read_ptr].size - NbDecodedData);      
  }
}

void HAL_JPEG_DataReadyCallback (JPEG_HandleTypeDef *hjpeg, uint8_t *pDataOut, uint32_t OutDataLength)
{
  Jpeg_HW_Decoder.outbuf[Jpeg_HW_Decoder.outbuf_write_ptr].sta  = JPEG_BUFFER_FULL;
  Jpeg_HW_Decoder.outbuf[Jpeg_HW_Decoder.outbuf_write_ptr].size = OutDataLength;
    
  Jpeg_HW_Decoder.outbuf_write_ptr++;
  if(Jpeg_HW_Decoder.outbuf_write_ptr >= NB_OUTPUT_DATA_BUFFERS)
  {
    Jpeg_HW_Decoder.outbuf_write_ptr = 0;        
  }

  if(Jpeg_HW_Decoder.outbuf[Jpeg_HW_Decoder.outbuf_write_ptr].sta != JPEG_BUFFER_EMPTY)
  {
    HAL_JPEG_Pause(hjpeg, JPEG_PAUSE_RESUME_OUTPUT);
    Jpeg_HW_Decoder.outdma_pause = 1;
  }
  HAL_JPEG_ConfigOutputBuffer(hjpeg, Jpeg_HW_Decoder.outbuf[Jpeg_HW_Decoder.outbuf_write_ptr].buf, JPEG_DMA_OUTBUF_LEN); 
}

/**
  * @brief  JPEG Error callback
  * @param hjpeg: JPEG handle pointer
  * @retval None
  */
void HAL_JPEG_ErrorCallback(JPEG_HandleTypeDef *hjpeg)
{
  LCD_printf("HAL_JPEG_ErrorCallback! \r\n");
}

/**
  * @brief  JPEG Decode complete callback
  * @param hjpeg: JPEG handle pointer
  * @retval None
  */
void HAL_JPEG_DecodeCpltCallback(JPEG_HandleTypeDef *hjpeg)
{    
  Jpeg_HW_Decoder.JpegProcessing_End = 1; 
}

