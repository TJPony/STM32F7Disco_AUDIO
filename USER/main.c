#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "timer.h"
#include "sdram.h"
#include "led.h"
#include "KEY.h"
#include "lcd.h"
#include "malloc.h"
#include "file.h"	
#include "text.h"
#include "audioplay.h"
#include "ts.h"

static PT_COMOptr   	COM6;
static PT_LedOptr   	LED;
static PT_KEYOptr   	KEY;
static PT_LCDOptr   	LCD;
static PT_MemOptr   	MEM;
static PT_AudioOptr   AUDIO;
PT_TsOptr TS;

static void WakeUpKey_press_audio_func(void)
{
	if(AUDIO->status & (1<<0))
	{
		AUDIO->status &= ~(1<<0);
		AUDIO->stop();
	}
	AUDIO->curindex++;
	if(AUDIO->curindex == AUDIO->wavnum)
	{
		AUDIO->curindex = 0;
	}
	AUDIO->start(AUDIO->curindex);
}

int main(void)
{
		u8 result;
		u8 *work_buf;
		float total,free;
	  u32 total_tmp,free_tmp;
	
    Cache_Enable();                 				//打开L1-Cache
    HAL_Init();				        							//初始化HAL库
    Stm32_Clock_Init(432,25,2,9);   				//设置时钟,216Mhz 
    delay_init(216);                				//延时初始化
		
		__HAL_RCC_GPIOA_CLK_ENABLE();           //开启GPIOA时钟
	  __HAL_RCC_GPIOB_CLK_ENABLE();           //开启GPIOB时钟
	  __HAL_RCC_GPIOC_CLK_ENABLE();           //开启GPIOC时钟
	  __HAL_RCC_GPIOD_CLK_ENABLE();           //开启GPIOD时钟
	  __HAL_RCC_GPIOE_CLK_ENABLE();           //开启GPIOE时钟
	  __HAL_RCC_GPIOF_CLK_ENABLE();           //开启GPIOF时钟
	  __HAL_RCC_GPIOG_CLK_ENABLE();           //开启GPIOG时钟
	  __HAL_RCC_GPIOH_CLK_ENABLE();           //开启GPIOH时钟
	  __HAL_RCC_GPIOI_CLK_ENABLE();           //开启GPIOI时钟
		__HAL_RCC_GPIOJ_CLK_ENABLE();           //开启GPIOJ时钟
	  __HAL_RCC_GPIOK_CLK_ENABLE();           //开启GPIOK时钟
	
		SDRAM_Init();
		
		MEM = Get_MemOptr();
	  MEM->init(SRAM0);
		MEM->init(SRAM1);
		MEM->init(SRAM2);
		
	  LCD  = Get_LCDOptr();
	  LCD->init();
		
		LCD_printf("system clock init Finish! \r\n");
	  LCD_printf("AHB clock: 216MHz \r\n");
	  LCD_printf("APB1 clock: 59MHz \r\n");
	  LCD_printf("APB2 clock: 59MHz \r\n");
	  LCD_printf("USB clock: 48MHz \r\n");
		LCD_printf("SDRAM Controller init Finish!\r\n");
		LCD_printf("LCD controller init Finish! \r\n");
		
		LED  = Get_LedOptr();
	  LED->init();
		LCD_printf("LED init Finish! \r\n");
		
		KEY  = Get_KEYOptr();
		KEY->init();
		LCD_printf("KEY init Finish! \r\n");
		
		COM6 = Get_COMOptr();
	  COM6->init(115200);
		LCD_printf("USART6: 115200,8,n,1 \r\n");
		
		if(Fatfs_init())
		{
			LCD_printf("Fatfs malloc Failed! \r\n");
		}
		else
		{
			LCD_printf("Fatfs malloc Finish! \r\n");
		}
		
		result = f_mount(fs[0],"0:",1); 					//挂载NorFlash
		if(result)
		{
			work_buf = mymalloc(SRAM0,1024);
			if(!work_buf)
			{
				LCD_printf("Fatfs NOR FLASH f_mkfs work_buf malloc Failed! \r\n");
				LCD_printf("mount Fat NOR FLASH fail!\r\n");
			}
			else
			{
				if(f_mkfs("0:",1,4096,work_buf,1024) == 0)
				{
					LCD_printf("Fatfs NOR FLASH f_mkfs Finish! \r\n");
					result = f_mount(fs[0],"0:",1); 	  //挂载NorFlash
					if(result)
					{
						LCD_printf("mount Fat NOR FLASH fail!\r\n");
					}
					else
					{
						Fatfs_getfree((u8 *)"0:",&total_tmp,&free_tmp);
						total = total_tmp;
						free  = free_tmp;
						LCD_printf("mount Fat NOR FLASH success! total:%.2fMB, free:%.2fMB\r\n",total/1024.0f,free/1024.0f);
						Scan_files((u8 *)"0:");
					}
				}
				else
				{
					LCD_printf("Fatfs NOR FLASH f_mkfs failed! \r\n");
				}
				myfree(SRAM0,work_buf);
			}
		}
		else
		{
			Fatfs_getfree((u8 *)"0:",&total_tmp,&free_tmp);
			total = total_tmp;
			free  = free_tmp;
			LCD_printf("mount Fat NOR FLASH success! total:%.2fMB, free:%.2fMB\r\n",total/1024.0f,free/1024.0f);
		}
 	  result = f_mount(fs[1],"1:",1); 					//挂载SDCard
		if(result)
		{
			LCD_printf("mount Fat SDCard fail!\r\n");
		}
		else
		{
			Fatfs_getfree((u8 *)"1:",&total_tmp,&free_tmp);
			total = total_tmp;
			free  = free_tmp;
			LCD_printf("mount Fat SDCard success! total:%.2fMB, free:%.2fMB\r\n",total/1024.0f,free/1024.0f);
		}
		
		MEM->peruse0 = MEM->perused(SRAM0);
		MEM->peruse1 = MEM->perused(SRAM1);
		MEM->peruse2 = MEM->perused(SRAM2);
		LCD_printf("SRAM0 peruse = %.1f%% \r\n",(float)(MEM->peruse0)/10.0f);
		LCD_printf("SRAM1 peruse = %.1f%% \r\n",(float)(MEM->peruse1)/10.0f);
		LCD_printf("SRAM2 peruse = %.1f%% \r\n",(float)(MEM->peruse2)/10.0f);
		
		if(font_check())
		{
			result = update_font();
			if(result)
			{
				LCD_printf("Font Update Failed! ErrorCode = %02d \r\n",result);
			}
			else
			{
				LCD_printf("font update finish!\r\n");
			}
		}
		else
		{
			LCD_printf("**********font ready!***********\r\n");
		}
		
		AUDIO = Get_AudioOptr();
		AUDIO->init();
		
		TS = Get_TsOptr();
		result = TS->init();
		if(result)
		{
			LCD_printf("Touchscreen init Failed! ErrorCode = %02d \r\n",result);
		}
		else
		{
			LCD_printf("Touchscreen init finish!\r\n");
		}
		
		register_key_func(KEY_WK_VALUE,KEY_PRESS,WakeUpKey_press_audio_func);
		
	  ProcessTimer_Init(10000-1,108-1); //定时器周期10ms
    while(1)
    {
			if(AUDIO->isDmaFinish)
			{
				AUDIO->isDmaFinish = FALSE;
				AUDIO->get_curtime(AUDIO->file,AUDIO->wavinfo);
				if(AUDIO->fillnum != WAV_SAI_TX_DMA_BUFSIZE)   //播放结束
				{
					AUDIO->status &= ~(1<<0);
					AUDIO->stop();
				}
				if(AUDIO->curwavbuf)
				{
					AUDIO->fillnum = wav_buffill(AUDIO->saibuf2,WAV_SAI_TX_DMA_BUFSIZE,AUDIO->wavinfo->bps);//填充buf1
				}
				else 
				{
					AUDIO->fillnum = wav_buffill(AUDIO->saibuf1,WAV_SAI_TX_DMA_BUFSIZE,AUDIO->wavinfo->bps);//填充buf0	
				}
			}
    }
}

