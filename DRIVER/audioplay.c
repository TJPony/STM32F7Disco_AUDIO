#include "audioplay.h"
#include "malloc.h"
#include "file.h"	
#include "delay.h"
#include "lcd.h"
#include "string.h"

static void audio_init(void);
static void audio_play_song(u16 index);
static void audio_stop(void);
static void audio_pause(void);
static void audio_resume(void);
static void wav_get_curtime(FIL *fx,wavparam *wavx);
static void audio_setvol(u8 vol);

static wavparam WavInfo;		//WAV参数结构体

/* 音乐播放控制器 */
static T_AudioOptr g_tAudioOptr = {
	.wavinfo  = &WavInfo,
	.curindex = 0,
	.status   = 0,
	.Vol      = 50,
	.isDmaFinish = FALSE,
	.curwavbuf = 0,
	.init     = audio_init,
	.start    = audio_play_song,
	.stop     = audio_stop,
	.pause    = audio_pause,
	.resume   = audio_resume,
	.get_curtime = wav_get_curtime,
	.setvol   = audio_setvol,
};

static void wav_sai_dma_tx_callback(void) 
{   
	u16 i;
	if(DMA2_Stream6->CR&(1<<19))
	{
		g_tAudioOptr.curwavbuf = 0;
		if((g_tAudioOptr.status & 0X01)==0)//暂停
		{
			for(i=0;i<WAV_SAI_TX_DMA_BUFSIZE;i++)
			{
				g_tAudioOptr.saibuf1[i]=0;//填充0
			}
		}
	}
	else 
	{
		g_tAudioOptr.curwavbuf = 1;
		if((g_tAudioOptr.status & 0X01)==0)
		{
			for(i=0;i<WAV_SAI_TX_DMA_BUFSIZE;i++)//暂停
			{
				g_tAudioOptr.saibuf2[i]=0;//填充0
			}
		}
	}
	g_tAudioOptr.isDmaFinish = TRUE;
} 

static void audio_init(void)
{
	u8 res;
	u8 *pname;			    
 	u32 temp;
	
	g_tAudioOptr.sai   = Get_SaiOptr();                   //获取SAI接口
	g_tAudioOptr.codec = Get_CodecOptr();                 //获取编解码器
	
	/* 初始化SAI接口DMA传输完成回调函数 */
	g_tAudioOptr.sai->dma_cplt_callback = wav_sai_dma_tx_callback;
	/* 初始化SAI接口 主机发送模式 时钟下降沿选通数据 24位数据 */
	g_tAudioOptr.sai->init(SAI_MODEMASTER_TX,SAI_CLOCKSTROBING_FALLINGEDGE,SAI_DATASIZE_24);
	/* 设置默认采样率位44.1KHz */
	g_tAudioOptr.sai->set_samplerate(44100);
 	/* 初始化SAI接口DMA 默认32位数据传输格式*/
	g_tAudioOptr.sai->dma_init(g_tAudioOptr.saibuf1,g_tAudioOptr.saibuf2,WAV_SAI_TX_DMA_BUFSIZE/4,2);
	/* 停止传输 */
	g_tAudioOptr.sai->stop();
	/* 编解码器初始化 耳机输出模式 默认音量70 默认采样率22.05KHz */
	g_tAudioOptr.codec->init(OUTPUT_DEVICE_HEADPHONE,g_tAudioOptr.Vol,AUDIO_FREQUENCY_22K);
	/* 为MUSIC文件夹申请内存 */
	g_tAudioOptr.wavdir = (DIR*)mymalloc(SRAM0,sizeof(DIR));	
	
 	while(f_opendir(g_tAudioOptr.wavdir,"1:/MUSIC"))			//打开音乐文件夹
 	{	    
		LCD_printf("audio_init: open music dir failed!\r\n");
		delay_ms(1000);
	}
	/* 得到WAV文件数 */
	g_tAudioOptr.wavnum = Get_WAV_Num((u8 *)"1:/MUSIC");
	
  while(g_tAudioOptr.wavnum == 0)												//音乐文件总数为0		
 	{	    
		LCD_printf("audio_init: no WAV file!\r\n");
		delay_ms(1000);
	}	
	/* 为WAV文件信息结构体分配内存 */
	g_tAudioOptr.wavfileinfo = (FILINFO*)mymalloc(SRAM0,sizeof(FILINFO));	
	/* 为带路径的文件名分配内存 */
  pname = mymalloc(SRAM0,_MAX_LFN*2+1);
  /* 申请4*wavnum个字节的内存,用于存放音乐文件偏移索引 */
 	g_tAudioOptr.wavoffsettbl = mymalloc(SRAM0,4*g_tAudioOptr.wavnum);
  /* 为WAV文件结构体分配内存 */	
	g_tAudioOptr.file=(FIL*)mymalloc(SRAM0,sizeof(FIL));
	/* 为DMA缓冲区0分配内存 */
	g_tAudioOptr.saibuf1=mymalloc(SRAM0,WAV_SAI_TX_DMA_BUFSIZE);
	/* 为DMA缓冲区1分配内存 */
	g_tAudioOptr.saibuf2=mymalloc(SRAM0,WAV_SAI_TX_DMA_BUFSIZE);
	/* 为DMA临时缓冲区分配内存 */
	g_tAudioOptr.tbuf=mymalloc(SRAM0,WAV_SAI_TX_DMA_BUFSIZE);
	
 	while(!g_tAudioOptr.wavfileinfo||!pname||!g_tAudioOptr.wavoffsettbl||!g_tAudioOptr.file||!g_tAudioOptr.saibuf1||!g_tAudioOptr.saibuf2||!g_tAudioOptr.tbuf)   //内存分配出错
 	{	    
		LCD_printf("audio_init: malloc WAV memory failed!\r\n");
		delay_ms(1000);
	} 
	
  res = f_opendir(g_tAudioOptr.wavdir,"1:/MUSIC"); 											//打开目录
	
	if(res == FR_OK)
	{
		g_tAudioOptr.curindex = 0;																					//当前索引为0
		while(1)																														//遍历目录内所有文件
		{						
			temp = g_tAudioOptr.wavdir->dptr;								     							
	    res  = f_readdir(g_tAudioOptr.wavdir,g_tAudioOptr.wavfileinfo);   //读取目录当前文件
	    if(res != FR_OK || g_tAudioOptr.wavfileinfo->fname[0] == 0)       //错误了/到末尾了,退出
			{
				break; 
			}				
			res = f_typetell((u8*)g_tAudioOptr.wavfileinfo->fname);	          //判断文件类型
			if(res == T_WAV)                                                  //如果是WAV文件
			{
				g_tAudioOptr.wavoffsettbl[g_tAudioOptr.curindex]=temp;					//记录当前索引
				g_tAudioOptr.curindex++;
			}	    
		} 
	}  
  	
  g_tAudioOptr.curindex=0;									
	f_opendir(g_tAudioOptr.wavdir,"1:/MUSIC"); 				        						//打开目录
	
	myfree(SRAM0,pname);				    //释放内存			    
} 

static void wav_decode_init(u8* fname,PT_AudioOptr ptAudioOptr)
{
	FIL*ftemp;
	u8 *buf; 
	u32 br=0;
	u8 res=0;
	
	ChunkRIFF *riff;
	ChunkFMT *fmt;
	ChunkFACT *fact;
	ChunkDATA *data;
	
	ptAudioOptr->status &= ~(0xf<<1);// 清除状态
	
	ftemp=(FIL*)mymalloc(SRAM0,sizeof(FIL));
	buf=mymalloc(SRAM0,512);
	if(ftemp&&buf)	//内存申请成功
	{
		res=f_open(ftemp,(TCHAR*)fname,FA_READ);//打开文件
		if(res==FR_OK)
		{
			f_read(ftemp,buf,512,&br);	//读取512字节数据
			riff=(ChunkRIFF *)buf;		  //获取RIFF块
			if(riff->Format==0X45564157)//是WAV文件
			{
				fmt=(ChunkFMT *)(buf+12);	//获取FMT块 
				fact=(ChunkFACT *)(buf+12+8+fmt->ChunkSize);//读取FACT块
				if(fact->ChunkID==0X74636166||fact->ChunkID==0X5453494C)//具有fact/LIST块的时候(未测试)
				{
					ptAudioOptr->wavinfo->datastart=12+8+fmt->ChunkSize+8+fact->ChunkSize;
				}
				else 
				{
					ptAudioOptr->wavinfo->datastart=12+8+fmt->ChunkSize;  
				}
				data=(ChunkDATA *)(buf + ptAudioOptr->wavinfo->datastart);	//读取DATA块
				if(data->ChunkID==0X61746164)//解析成功!
				{
					ptAudioOptr->wavinfo->audioformat=fmt->AudioFormat;		  //音频格式
					ptAudioOptr->wavinfo->nchannels=fmt->NumOfChannels;		  //通道数
					ptAudioOptr->wavinfo->samplerate=fmt->SampleRate;		    //采样率
					ptAudioOptr->wavinfo->bitrate=fmt->ByteRate*8;			    //得到位速
					ptAudioOptr->wavinfo->blockalign=fmt->BlockAlign;		    //块对齐
					ptAudioOptr->wavinfo->bps=fmt->BitsPerSample;			      //位数,16/24/32位
					
					ptAudioOptr->wavinfo->datasize=data->ChunkSize;			    //数据块大小
					ptAudioOptr->wavinfo->datastart=ptAudioOptr->wavinfo->datastart+8;		  //数据流开始的地方. 
					 
					LCD_printf("audioformat:%d\r\n",ptAudioOptr->wavinfo->audioformat);
					LCD_printf("nchannels:%d\r\n",ptAudioOptr->wavinfo->nchannels);
					LCD_printf("samplerate:%d\r\n",ptAudioOptr->wavinfo->samplerate);
					LCD_printf("bitrate:%d\r\n",ptAudioOptr->wavinfo->bitrate);
					LCD_printf("blockalign:%d\r\n",ptAudioOptr->wavinfo->blockalign);
					LCD_printf("bps:%d\r\n",ptAudioOptr->wavinfo->bps);
					LCD_printf("datasize:%d\r\n",ptAudioOptr->wavinfo->datasize);
					LCD_printf("datastart:%d\r\n",ptAudioOptr->wavinfo->datastart);  
				}
				else
				{
					ptAudioOptr->status |= (1<<4);
					LCD_printf("wav_decode: Data zone not found!\r\n");
				}
			}
			else 
			{
				ptAudioOptr->status |= (1<<3);
				LCD_printf("wav_decode: It is not wav file!\r\n");
			}				
		}
		else 
		{
			ptAudioOptr->status |= (1<<2);
			LCD_printf("wav_decode: Can't open current song file!\r\n");
		}
	}
	else
	{
		ptAudioOptr->status |= (1<<1);
		LCD_printf("wav_decode: Memory malloc failed!\r\n");
	}
	f_close(ftemp);
	myfree(SRAM0,ftemp);//释放内存
	myfree(SRAM0,buf); 
}

u32 wav_buffill(u8 *buf,u16 size,u8 bits)
{
	u16 readlen=0;
	u32 bread;
	u16 i;
	u32 *p,*pbuf;
	if(bits==24)							//24bit音频,需要处理一下
	{
		readlen=(size/4)*3;			//此次要读取的字节数
		f_read(g_tAudioOptr.file,g_tAudioOptr.tbuf,readlen,(UINT*)&bread);//读取数据 
		pbuf=(u32*)buf;
		for(i=0;i<size/4;i++)
		{  
			p=(u32*)(g_tAudioOptr.tbuf+i*3);
			pbuf[i]=p[0];  
		} 
		bread=(bread*4)/3;		//填充后的大小.
	}
	else 
	{
		f_read(g_tAudioOptr.file,buf,size,(UINT*)&bread);//16bit音频,直接读取数据  
		if(bread<size)//不够数据了,补充0
		{
			for(i=bread;i<size-bread;i++)buf[i]=0; 
		}
	}
	return bread;
}  

static void audio_play_song(u16 index)
{
	u8 res;
	u8 *pname;			
	
	pname = mymalloc(SRAM0,_MAX_LFN*2+1);					      									//为带路径的文件名分配内存
  dir_sdi(g_tAudioOptr.wavdir,g_tAudioOptr.wavoffsettbl[index]);				//改变当前目录索引  
	res = f_readdir(g_tAudioOptr.wavdir,g_tAudioOptr.wavfileinfo);       	//读取目录当前文件
	
	if(res != FR_OK || g_tAudioOptr.wavfileinfo->fname[0] == 0)
	{
		LCD_printf("audio_play_song: f_readdir curindex failed!\r\n");
	}
	else
	{
		strcpy((char*)pname,"1:/MUSIC/");						              					//复制路径(目录)
		strcat((char*)pname,(const char*)g_tAudioOptr.wavfileinfo->fname);	//补全文件名
		LCD_printf("当前播放:%s\r\n",pname); 
		
		wav_decode_init(pname,&g_tAudioOptr);                     					//解码WAV文件，得到文件的信息
		
		if((g_tAudioOptr.status & 0x1E) == 0x00)
		{
			if(g_tAudioOptr.wavinfo->bps == 16)
			{
				g_tAudioOptr.codec->setfmt(2,0);															  //设置编解码器格式为飞利浦标准,16位数据长度
				g_tAudioOptr.codec->setfreq(g_tAudioOptr.wavinfo->samplerate);  //设置编解码器采样率
				g_tAudioOptr.sai->init(SAI_MODEMASTER_TX,SAI_CLOCKSTROBING_FALLINGEDGE,SAI_DATASIZE_16); //重新初始化SAI接口
				g_tAudioOptr.sai->set_samplerate(g_tAudioOptr.wavinfo->samplerate);//SAI接口设置采样率 				
				g_tAudioOptr.sai->dma_init(g_tAudioOptr.saibuf1,g_tAudioOptr.saibuf2,WAV_SAI_TX_DMA_BUFSIZE/2,1); //重新初始化SAI接口DMA
			}
			else if(g_tAudioOptr.wavinfo->bps == 24)
			{
				g_tAudioOptr.codec->setfmt(2,2);															  //设置编解码器格式为飞利浦标准,24位数据长度
				g_tAudioOptr.codec->setfreq(g_tAudioOptr.wavinfo->samplerate);  //设置编解码器采样率
				g_tAudioOptr.sai->init(SAI_MODEMASTER_TX,SAI_CLOCKSTROBING_FALLINGEDGE,SAI_DATASIZE_24); //重新初始化SAI接口
				g_tAudioOptr.sai->set_samplerate(g_tAudioOptr.wavinfo->samplerate);//SAI接口设置采样率 				
				g_tAudioOptr.sai->dma_init(g_tAudioOptr.saibuf1,g_tAudioOptr.saibuf2,WAV_SAI_TX_DMA_BUFSIZE/4,2); //重新初始化SAI接口DMA
			}
			g_tAudioOptr.status &= ~(1<<0);                                   //清除播放标志
	    g_tAudioOptr.sai->stop();                                         //停止SAI接口DMA传输
   		g_tAudioOptr.codec->stop(CODEC_PDWN_SW);                          //停止编解码器
			
			res = f_open(g_tAudioOptr.file,(TCHAR*)pname,FA_READ);	          //打开文件
			if(res==0)
			{
				f_lseek(g_tAudioOptr.file, g_tAudioOptr.wavinfo->datastart);		//跳过文件头
				g_tAudioOptr.fillnum = wav_buffill(g_tAudioOptr.saibuf1,WAV_SAI_TX_DMA_BUFSIZE,g_tAudioOptr.wavinfo->bps);//填充buf0
				g_tAudioOptr.fillnum = wav_buffill(g_tAudioOptr.saibuf2,WAV_SAI_TX_DMA_BUFSIZE,g_tAudioOptr.wavinfo->bps);//填充buf1
				g_tAudioOptr.status |= (1<<0);
				g_tAudioOptr.isDmaFinish = FALSE;
				g_tAudioOptr.codec->play();                                     //开启编解码器
	      g_tAudioOptr.sai->start();                                      //开始SAI接口DMA传输                 
			}
			else
			{
				g_tAudioOptr.status |= (1<<2);
			  LCD_printf("audio_play_song: Can't open current song file!\r\n");
			}
		}
	}		    
	myfree(SRAM0,pname);				    //释放内存			    
}

static void audio_stop(void)
{
	g_tAudioOptr.sai->stop();                                         //停止SAI接口DMA传输
  g_tAudioOptr.codec->stop(CODEC_PDWN_SW);                          //停止编解码器
}

static void audio_pause(void)
{
	g_tAudioOptr.sai->stop();                                         //停止SAI接口DMA传输
	g_tAudioOptr.codec->pause();                          						//停止编解码器
}

static void audio_resume(void)
{
	g_tAudioOptr.sai->start();
	g_tAudioOptr.codec->resume();
}

static void wav_get_curtime(FIL *fx,wavparam *wavx)
{
	long long fpos;  	
 	wavx->totsec=wavx->datasize/(wavx->bitrate/8);	 //歌曲总长度(单位:秒) 
	fpos=fx->fptr-wavx->datastart; 					         //得到当前文件播放到的地方 
	wavx->cursec=fpos*wavx->totsec/wavx->datasize;	 //当前播放到第多少秒了	
	
	LCD_printf("%02d:%02d/%02d:%02d\r",wavx->totsec/60,wavx->totsec%60,wavx->cursec/60,wavx->cursec%60);
}

static void audio_setvol(u8 vol)
{
	g_tAudioOptr.codec->setvolume(vol);
}

PT_AudioOptr Get_AudioOptr(void)
{
	PT_AudioOptr pTtemp;
	pTtemp = &g_tAudioOptr;
	return pTtemp;
}
