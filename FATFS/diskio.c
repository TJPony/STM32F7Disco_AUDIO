
#include "diskio.h"		/* FatFs lower layer API */
#include "MX25L512.h"
#include "sdcard.h"
#include "malloc.h"	
#include "lcd.h"	

#define NORFLASH	 	  0  		// Nor Flash,卷标为0
#define SD_CARD 	    1	    // SD Card,卷标为1

static PT_MX25L512 MX25L512;
static PT_SDCardOptr SDCard;

DSTATUS disk_status (BYTE pdrv)
{
	return RES_OK;
}

DSTATUS disk_initialize (BYTE pdrv)
{
	u8 res = 0;	    
	switch(pdrv)
	{
		case NORFLASH:		
		    MX25L512 = Get_MX25L512();
				res = MX25L512->init();
				if(res != QSPI_OK)
				{
					LCD_printf("NorFlash init Failed! ErrorCode = %02d \r\n",res);
				}
				else
				{
					LCD_printf("NorFlash init Finish! \r\n");
					LCD_printf("Flash Info:\r\n");
					LCD_printf("  ManufacturerID = %02x,DeviceID = %04x\r\n",MX25L512->ManufacturerID,MX25L512->DeviceID);
					LCD_printf("  Flash Size: %dMB\r\n",MX25L512_FLASH_SIZE/1024/1024);
				}
  			break;
		case SD_CARD:		
			  SDCard = Get_SDCardOptr();
				res = SDCard->init();
				if(res != SDCARD_OK)
				{
					LCD_printf("SDCard init Failed! ErrorCode = %02d \r\n",res);
				}
				else
				{
					switch(SDCard->info.CardType)
					{
						case CARD_SDSC:LCD_printf("Card Type:SDSC \r\n");break;
						case CARD_SDHC_SDXC:LCD_printf("Card Type:SDHC\r\n");break;
						default:break;
					}	
					LCD_printf("Card RCA:%d\r\n",SDCard->info.RelCardAdd);											//卡相对地址
					LCD_printf("Card BlockNbr:%d \r\n",SDCard->info.BlockNbr);	
					LCD_printf("Card BlockSize:%d\r\n\r\n",SDCard->info.BlockSize);		
					LCD_printf("Card LogBlockNbr:%d \r\n",SDCard->info.LogBlockNbr);	
					LCD_printf("Card LogBlockSize:%d\r\n\r\n",SDCard->info.LogBlockSize);		
				}
 			  break;
		default:
			res=1; 
	}		 
	if(res)return  STA_NOINIT;
	else return 0; //初始化成功 
}
//读扇区 pdrv:磁盘编号0~9，*buff:数据接收缓冲首地址，sector:扇区地址，count:需要读取的扇区数
DRESULT disk_read (BYTE pdrv,BYTE *buff,DWORD sector,UINT count)
{
	u8 res=0; 
  if (!count)return RES_PARERR;//count不能等于0，否则返回参数错误		 	 
	switch(pdrv)
	{
		case NORFLASH:
			for(;count>0;count--)
			{
				res = MX25L512->read(buff,sector*512,512);
				sector++;
				buff += 512;
			}
			break;
		case SD_CARD:
	    res = SDCard->read(buff,sector,count);
			break;
		default:
			res=1; 
	}
    if(res == 0x00)return RES_OK;	 
    else return RES_ERROR;	   
}
//写扇区 pdrv:磁盘编号0~9，*buff:发送数据首地址，sector:扇区地址，count:需要读取的扇区数
DRESULT disk_write (BYTE pdrv,const BYTE *buff,DWORD sector,UINT count)
{
	u8 res=0;  
  if (!count)return RES_PARERR;//count不能等于0，否则返回参数错误		 	 
	switch(pdrv)
	{
		case NORFLASH:
			for(;count>0;count--)
			{
				res = MX25L512->write((u8 *)buff,sector*512,512);
				sector++;
				buff += 512;
			}
			break;
		case SD_CARD:
	    res = SDCard->write((u8 *)buff,sector,count);
			break;
		default:
			res=1; 
	}
    if(res == 0x00)return RES_OK;	 
    else return RES_ERROR;	
} 
//其他表参数的获得 pdrv:磁盘编号0~9,ctrl:控制代码,*buff:发送/接收缓冲区指针 
DRESULT disk_ioctl (BYTE pdrv,BYTE cmd,void *buff)
{
  DRESULT res;
  if(pdrv == NORFLASH)
	{
	    switch(cmd)
	    {
		    case CTRL_SYNC:
				    res = RES_OK; 
		        break;	 
		    case GET_SECTOR_SIZE:
		        *(WORD*)buff = 512;													 //NOR FLASH扇区强制为512字节大小
		        res = RES_OK;
		        break;	 
		    case GET_BLOCK_SIZE:
		        *(WORD*)buff = MX25L512_SECTOR_SIZE / 512;   //block大小,8个扇区
		        res = RES_OK;
		        break;	 
		    case GET_SECTOR_COUNT:
		        *(DWORD*)buff = (MX25L512_FLASH_SIZE / 512) - (10*1024*1024/512);	//Nor FLASH的总扇区数量 后10M给字库
		        res = RES_OK;
		        break;
		    default:
		        res = RES_PARERR;
		        break;
	    }
	}
	else if(pdrv == SD_CARD)
	{
	    switch(cmd)
	    {
		    case CTRL_SYNC:
				    res = RES_OK; 
		        break;	 
		    case GET_SECTOR_SIZE:
		        *(WORD*)buff = SDCard->info.LogBlockSize;
		        res = RES_OK;
		        break;	 
		    case GET_BLOCK_SIZE:
		        *(WORD*)buff = SDCard->info.BlockSize;
		        res = RES_OK;
		        break;	 
		    case GET_SECTOR_COUNT:
		        *(DWORD*)buff = SDCard->info.LogBlockNbr;
		        res = RES_OK;
		        break;
		    default:
		        res = RES_PARERR;
		        break;
	    }
	}
  else 
	{
		res=RES_ERROR;//其他的不支持
	}
    return res;
} 
//获得时间
//31-25: Year(0-127 org.1980), 24-21: Month(1-12), 20-16: Day(1-31) */                                                                                                                                                                                                                                          
//15-11: Hour(0-23), 10-5: Minute(0-59), 4-0: Second(0-29 *2) */                                                                                                                                                                                                                                                
DWORD get_fattime (void)
{				 
	return 0;
}			 
//动态分配内存
void *ff_memalloc (UINT size)			
{
	return (void*)mymalloc(SRAM0,size);
}
//释放内存
void ff_memfree (void* mf)		 
{
	myfree(SRAM0,mf);
}
