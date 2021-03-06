#include "sai.h"
#include "delay.h"

/* SAI接口采样率分频设置表
 * SAI Block A采样率设置 采样率计算公式:
 * MCKDIV!=0: Fs = SAI_CK_x/[512*MCKDIV]
 * MCKDIV==0: Fs = SAI_CK_x/256
 * SAI_CK_x = vco*PLLI2SN/PLLI2SQ/PLLI2SDIVQ
 * HSE = 25Mhz pllm为25分频,则vco=1Mhz
 * PLLI2SN:192~432 PLLI2SQ:2~15 PLLI2SDIVQ:0~31 MCKDIV:0~15 
 */
const u16 SAI_PSC_TBL[][5]=
{
	{800 ,344,7,1,12},	//8Khz采样率
	{1102,429,2,19,2},	//11.025Khz采样率 
	{1600,344,7, 1,6},	//16Khz采样率
	{2205,429,2,19,1},	//22.05Khz采样率
	{3200,344,7, 1,3},	//32Khz采样率
	{4410,429,2,19,0},	//44.1Khz采样率
	{4800,344,7, 1,2},	//48Khz采样率
	{8820,271,2, 3,1},	//88.2Khz采样率
	{9600,344,7, 1,1},	//96Khz采样率
	{17640,271,2,3,0},	//176.4Khz采样率 
	{19200,344,7,1,0},	//192Khz采样率
};

/* 函数声明 */
static void SAIA_Init(u32 mode,u32 cpol,u32 datalen);
static u8 SAIA_SampleRate_Set(u32 samplerate);
static void SAIA_TX_DMA_Init(u8* buf0,u8 *buf1,u16 num,u8 width);
static void SAIA_DMA_Enable(void);
static void SAI_Play_Start(void); 
static void SAI_Play_Stop(void); 

/* SAI接口操作结构体 */
static T_SaiOptr g_tSaiOptr = {
	 .init             = SAIA_Init,
	 .set_samplerate   = SAIA_SampleRate_Set,
	 .dma_init         = SAIA_TX_DMA_Init,
	 .start            = SAI_Play_Start,
	 .stop             = SAI_Play_Stop,
};

static SAI_HandleTypeDef SAI1A_Handler;        //SAI1 Block A句柄
static DMA_HandleTypeDef SAI1_TXDMA_Handler;   //DMA发送句柄

/************************************************
名    称: SAIA_Init
功    能: SAIA接口初始化
输入参数: mode--无符号32位整型 工作模式 SAI_MODEMASTER_TX、SAI_MODEMASTER_RX、SAI_MODESLAVE_TX、SAI_MODESLAVE_RX
          cpol--无符号32位整型 数据选通极性 SAI_CLOCKSTROBING_FALLINGEDGE、SAI_CLOCKSTROBING_RISINGEDGE
          datalen--无符号32位整型 数据大小 SAI_DATASIZE_8、SAI_DATASIZE_10、SAI_DATASIZE_16、SAI_DATASIZE_20、SAI_DATASIZE_24、SAI_DATASIZE_32
输出参数: 无
返回值:   无
************************************************/	 
static void SAIA_Init(u32 mode,u32 cpol,u32 datalen)
{
    HAL_SAI_DeInit(&SAI1A_Handler);                          								//清除以前的配置
		
		/* 基础设置 */
    SAI1A_Handler.Instance        						= SAI1_Block_A;               //SAI1 Bock A
    SAI1A_Handler.Init.AudioMode							= mode;                       //设置SAI1工作模式
    SAI1A_Handler.Init.Synchro								= SAI_ASYNCHRONOUS;           //音频模块异步
    SAI1A_Handler.Init.OutputDrive						= SAI_OUTPUTDRIVE_ENABLE;   	//立即驱动音频模块输出
	  SAI1A_Handler.Init.SynchroExt							= SAI_SYNCEXT_DISABLE;        //外部同步禁止
	  SAI1A_Handler.Init.CompandingMode 				= SAI_NOCOMPANDING;      			//不使用Companding模式
	  SAI1A_Handler.Init.TriState								= SAI_OUTPUT_NOTRELEASED;     //不使用Companding模式
    SAI1A_Handler.Init.NoDivider							= SAI_MASTERDIVIDER_ENABLE;   //使能主时钟分频器(MCKDIV)
    SAI1A_Handler.Init.FIFOThreshold  				= SAI_FIFOTHRESHOLD_1QF;  		//设置FIFO阈值,1/4 FIFO
    SAI1A_Handler.Init.MonoStereoMode 				= SAI_STEREOMODE;        			//立体声模式
    SAI1A_Handler.Init.Protocol								= SAI_FREE_PROTOCOL;          //设置SAI1协议为:自由协议(支持I2S/LSB/MSB/TDM/PCM/DSP等协议)
    SAI1A_Handler.Init.DataSize								= datalen;                    //设置数据大小
    SAI1A_Handler.Init.FirstBit								= SAI_FIRSTBIT_MSB;           //数据MSB位优先
    SAI1A_Handler.Init.ClockStrobing					= cpol;                   		//设置数据选通时钟沿极性
    
    /* 帧设置 */
    SAI1A_Handler.FrameInit.FrameLength				= 64;                  				//设置帧长度为64,左通道32个SCK,右通道32个SCK.
    SAI1A_Handler.FrameInit.ActiveFrameLength = 32;            							//设置帧同步有效电平长度,在I2S模式下=1/2帧长.
    SAI1A_Handler.FrameInit.FSDefinition			= SAI_FS_CHANNEL_IDENTIFICATION;//FS信号为SOF信号+通道识别信号
    SAI1A_Handler.FrameInit.FSPolarity				= SAI_FS_ACTIVE_LOW;    			//FS低电平有效(下降沿)
    SAI1A_Handler.FrameInit.FSOffset					= SAI_FS_BEFOREFIRSTBIT;      //在slot0的第一位的前一位使能FS,以匹配飞利浦标准	

    /* SLOT设置 */
    SAI1A_Handler.SlotInit.FirstBitOffset			= 0;                 					//slot偏移(FBOFF)为0
    SAI1A_Handler.SlotInit.SlotSize						= SAI_SLOTSIZE_32B;        		//slot大小为32位
    SAI1A_Handler.SlotInit.SlotNumber					= 2;                     			//slot数为2个    
    SAI1A_Handler.SlotInit.SlotActive					= SAI_SLOTACTIVE_0|SAI_SLOTACTIVE_1;//使能slot0和slot1
    
    HAL_SAI_Init(&SAI1A_Handler);                            								//初始化SAI
    __HAL_SAI_ENABLE(&SAI1A_Handler);                        								//使能SAI 
}

/************************************************
名    称: HAL_SAI_MspInit
功    能: SAI接口底层初始化
输入参数: hsai--SAI接口句柄
输出参数: 无
返回值:   无
************************************************/	 
void HAL_SAI_MspInit(SAI_HandleTypeDef *hsai)
{
    GPIO_InitTypeDef GPIO_Initure;
    __HAL_RCC_SAI1_CLK_ENABLE();                //使能SAI1时钟
    
    GPIO_Initure.Pin=GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6;  
    GPIO_Initure.Mode=GPIO_MODE_AF_PP;          //推挽复用
    GPIO_Initure.Pull=GPIO_PULLUP;              //上拉
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;         //高速
    GPIO_Initure.Alternate=GPIO_AF6_SAI1;       //复用为SAI   
    HAL_GPIO_Init(GPIOE,&GPIO_Initure);         //初始化
	
		GPIO_Initure.Pin=GPIO_PIN_7;
	  HAL_GPIO_Init(GPIOG,&GPIO_Initure);         //初始化
}
 
/************************************************
名    称: SAIA_DMA_Enable
功    能: SAIA接口DMA使能
输入参数: 无
输出参数: 无
返回值:   无
************************************************/	 
static void SAIA_DMA_Enable(void)
{
    u32 tempreg       = 0;
    tempreg  					= SAI1_Block_A->CR1;          //先读出以前的设置
	  tempreg 				 |= 1<<17;					            //使能DMA
	  SAI1_Block_A->CR1 = tempreg;		      					//写入CR1寄存器中
}

/************************************************
名    称: SAIA_SampleRate_Set
功    能: SAIA接口采样率设置
输入参数: samplerate--无符号32位整型 采样率 单位HZ
输出参数: 无
返回值:   无符号8位整型 0:设置成功 1:不支持的采样率
************************************************/	 
static u8 SAIA_SampleRate_Set(u32 samplerate)
{   
    u8 i=0; 
    
    RCC_PeriphCLKInitTypeDef RCCSAI1_Sture; 
	
    /* 查询是否支持数组中的采样率	*/
		for(i=0;i<(sizeof(SAI_PSC_TBL)/10);i++)
		{
			if((samplerate/10) == SAI_PSC_TBL[i][0])
			{
				break;                                                   		//匹配到了
			}
		}
    if(i == (sizeof(SAI_PSC_TBL)/10)) 
		{
			return 1;                                                  		//没匹配到
		}
			
    RCCSAI1_Sture.PeriphClockSelection = RCC_PERIPHCLK_SAI1;			  //外设时钟源选择SAI1时钟 
		RCCSAI1_Sture.Sai1ClockSelection   = RCC_SAI1CLKSOURCE_PLLSAI;  //选择PLLSAI作为SAI1时钟
    RCCSAI1_Sture.PLLSAI.PLLSAIN			 = (u32)SAI_PSC_TBL[i][1];    //设置PLLSAIN
    RCCSAI1_Sture.PLLSAI.PLLSAIQ       = (u32)SAI_PSC_TBL[i][2];    //设置PLLSAIQ
    RCCSAI1_Sture.PLLSAIDivQ					 = SAI_PSC_TBL[i][3];         //设置PLLSAIDIVQ
    HAL_RCCEx_PeriphCLKConfig(&RCCSAI1_Sture);                  		//设置时钟
    
    __HAL_SAI_DISABLE(&SAI1A_Handler);                          		//关闭SAI
    SAI1A_Handler.Init.AudioFrequency  = samplerate;                //设置播放频率
		SAI1A_Handler.Init.Mckdiv          = SAI_PSC_TBL[i][4];         //设置MCLK分频
    HAL_SAI_Init(&SAI1A_Handler);                               		//重新初始化SAI
    SAIA_DMA_Enable();                                          		//开启SAI的DMA功能
    __HAL_SAI_ENABLE(&SAI1A_Handler);                          			//使能SAI
		
    return 0;
}   

/************************************************
名    称: SAIA_TX_DMA_Init
功    能: SAIA接口发送DMA初始化
输入参数: buf0--无符号8位整型指针 DMA双缓冲区0
          buf1--无符号8位整型指针 DMA双缓冲区1
          num--无符号16位整型 DMA数据传输个数
					width--无符号8位整型 DMA单个数据字节数  0:1byte  1:2byte  2:4byte
输出参数: 无
返回值:   无
************************************************/	 
static void SAIA_TX_DMA_Init(u8* buf0,u8 *buf1,u16 num,u8 width)
{ 
    u32 memwidth=0,perwidth=0;      																			//外设和存储器位宽
    switch(width)
    {
        case 0:         
            memwidth=DMA_MDATAALIGN_BYTE;
            perwidth=DMA_PDATAALIGN_BYTE;
            break;
        case 1:         
            memwidth=DMA_MDATAALIGN_HALFWORD;
            perwidth=DMA_PDATAALIGN_HALFWORD;
            break;
        case 2:         
            memwidth=DMA_MDATAALIGN_WORD;
            perwidth=DMA_PDATAALIGN_WORD;
            break;
        default:    
					  memwidth=DMA_MDATAALIGN_WORD;
            perwidth=DMA_PDATAALIGN_WORD;
				    break;
    } 
		
    __HAL_RCC_DMA2_CLK_ENABLE();                                    			//使能DMA2时钟
    __HAL_LINKDMA(&SAI1A_Handler,hdmatx,SAI1_TXDMA_Handler);        			//将DMA与SAI联系起来
		
    SAI1_TXDMA_Handler.Instance									= DMA2_Stream6;     			//DMA2数据流1                    
    SAI1_TXDMA_Handler.Init.Channel							= DMA_CHANNEL_10;    			//通道0
    SAI1_TXDMA_Handler.Init.Direction						= DMA_MEMORY_TO_PERIPH;   //存储器到外设模式
    SAI1_TXDMA_Handler.Init.PeriphInc						= DMA_PINC_DISABLE;       //外设非增量模式
    SAI1_TXDMA_Handler.Init.MemInc							= DMA_MINC_ENABLE;        //存储器增量模式
    SAI1_TXDMA_Handler.Init.PeriphDataAlignment = perwidth;           		//外设数据长度
    SAI1_TXDMA_Handler.Init.MemDataAlignment    = memwidth;               //存储器数据长度
    SAI1_TXDMA_Handler.Init.Mode								= DMA_CIRCULAR;           //使用循环模式 
    SAI1_TXDMA_Handler.Init.Priority						= DMA_PRIORITY_HIGH;      //高优先级
    SAI1_TXDMA_Handler.Init.FIFOMode						= DMA_FIFOMODE_DISABLE;   //不使用FIFO
    SAI1_TXDMA_Handler.Init.MemBurst						= DMA_MBURST_SINGLE;      //存储器单次突发传输
    SAI1_TXDMA_Handler.Init.PeriphBurst					= DMA_PBURST_SINGLE;      //外设突发单次传输 
    HAL_DMA_DeInit(&SAI1_TXDMA_Handler);                            			//先清除以前的设置
    HAL_DMA_Init(&SAI1_TXDMA_Handler);	                            			//初始化DMA

    HAL_DMAEx_MultiBufferStart(&SAI1_TXDMA_Handler,(u32)buf0,(u32)&SAI1_Block_A->DR,(u32)buf1,num);	//开启双缓冲
    __HAL_DMA_DISABLE(&SAI1_TXDMA_Handler);                         			//关闭DMA 
    delay_us(10);                                                   			//10us延时，防止-O2优化出问题 	
    __HAL_DMA_ENABLE_IT(&SAI1_TXDMA_Handler,DMA_IT_TC);             			//开启传输完成中断
    __HAL_DMA_CLEAR_FLAG(&SAI1_TXDMA_Handler,DMA_FLAG_TCIF2_6);     			//清除DMA传输完成中断标志位
    HAL_NVIC_SetPriority(DMA2_Stream6_IRQn,0,0);                    			//DMA中断优先级设为最高
    HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);                                //使能中断
}
   
/************************************************
名    称: SAI_Play_Start
功    能: 开启SAI接口DMA传输
输入参数: 无
输出参数: 无
返回值:   无
************************************************/	
static void SAI_Play_Start(void)
{   	
    __HAL_DMA_ENABLE(&SAI1_TXDMA_Handler);//开启DMA TX传输  			
}

/************************************************
名    称: SAI_Play_Stop
功    能: 关闭SAI接口DMA传输
输入参数: 无
输出参数: 无
返回值:   无
************************************************/	
static void SAI_Play_Stop(void)
{   
    __HAL_DMA_DISABLE(&SAI1_TXDMA_Handler);  //结束播放  	 	 
} 


/************************************************
名    称: DMA2_Stream3_IRQHandler
功    能: DMA2_Stream3中断服务函数
输入参数: 无
输出参数: 无
返回值:   无
************************************************/	
void DMA2_Stream6_IRQHandler(void)
{   
    if(__HAL_DMA_GET_FLAG(&SAI1_TXDMA_Handler,DMA_FLAG_TCIF2_6)!=RESET) //DMA传输完成
    {
        __HAL_DMA_CLEAR_FLAG(&SAI1_TXDMA_Handler,DMA_FLAG_TCIF2_6);     //清除DMA传输完成中断标志位
        g_tSaiOptr.dma_cplt_callback();                                 //调用传输完成回调函数
    }  											 
} 

PT_SaiOptr Get_SaiOptr(void)
{
	PT_SaiOptr pTtemp;
	pTtemp = &g_tSaiOptr;
	return pTtemp;
}
