#ifndef _TEXT_H
#define _TEXT_H
#include "sys.h"
#include "MX25L512.h"

#define FONT_UPDATE_OK            0
#define FONT_UPDATE_UNIGBK_ERROR  1
#define FONT_MALLOC_GBK12_ERROR   2
#define FONT_MALLOC_GBK16_ERROR   3
#define FONT_MALLOC_GBK24_ERROR   4
#define FONT_MALLOC_GBK32_ERROR   5
#define FONT_MALLOC_ERROR   			6   
#define FONT_FILE_ERROR   			  7

#define FONT_SECTOR_NUM	 	  1539              //字库NorFlash区域的扇区数(4个字库+unigbk表+字库信息=6302984字节,约占1539个扇区,一个扇区4K字节)
#define FONT_START_ADDR 	  1024*1024*54 			//字库起始地址 前54M为FATFS管理区

__packed typedef struct _font_info
{
	u8 fontok;																	//字库存在标志，0XAA，字库正常；其他，字库不存在
	u32 ugbkaddr; 															//unigbk表地址
	u32 ugbksize;																//unigbk的大小	 
	u32 f12addr;																//gbk12地址	
	u32 gbk12size;															//gbk12的大小	 
	u32 f16addr;																//gbk16地址
	u32 gbk16size;															//gbk16的大小		 
	u32 f24addr;																//gbk24地址
	u32 gbk24size;															//gbk24的大小 	 
	u32 f32addr;																//gbk32地址
	u32 gbk32size;															//gbk32的大小 
}font_info; 

extern font_info ftinfo;

u8 font_check(void);
u8 update_font(void);

#endif
