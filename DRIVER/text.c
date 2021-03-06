#include "text.h"
#include "file.h"
#include "delay.h"
#include "malloc.h"
#include "lcd.h"
#include "string.h"

u8* const GBK_PATH[5]=
{
	(u8* const)"1:/FONT/UNIGBK.BIN",	//UNIGBK.BIN的存放位置
	(u8* const)"1:/FONT/GBK12.FON",		//GBK12的存放位置
	(u8* const)"1:/FONT/GBK16.FON",		//GBK16的存放位置
	(u8* const)"1:/FONT/GBK24.FON",		//GBK24的存放位置
	(u8* const)"1:/FONT/GBK32.FON",		//GBK32的存放位置
}; 

font_info ftinfo;

u8 font_check(void)
{		
	u8 i = 0;
  
	while(i < 5)			//连续读取5次
	{
		i++;
		MX25L512_Read((u8*)&ftinfo,FONT_START_ADDR,sizeof(ftinfo));//读出ftinfo结构体数据
		if(ftinfo.fontok == 0XAA)
			break;
		delay_ms(20);
	}
	if(ftinfo.fontok != 0XAA)
	{
		return 1;
	}
	return 0;		    
}

static u8 cal_rate(u32 total,u32 cur)
{
	float rate;
	
	rate  = (float)cur / total;
	rate *= 100.0f;
	
	return (u8)rate;					    
} 

static u8 updata_fontx(u8 *filepath,u8 font_num)
{
	u32 flashaddr=0;								    
	u8 *tempbuf;
	u32 offx=0;
     
	tempbuf = mymalloc(SRAM0,MX25L512_SECTOR_SIZE);								//申请MX25L512_SECTOR_SIZE字节内存 
	if(tempbuf == NULL)
	{
		return 1;
	}
	
 	if(f_open(file,(const TCHAR*)filepath,FA_READ))
	{
		myfree(SRAM0,tempbuf);																			//释放内存
		return 2;
	}		

	switch(font_num)
	{
		case 0:																										
			ftinfo.ugbkaddr  = FONT_START_ADDR + sizeof(ftinfo);			//信息头之后，紧跟UNIGBK转换码表
			ftinfo.ugbksize  = file->obj.objsize;										  //UNIGBK大小
			flashaddr        = ftinfo.ugbkaddr;                       //UNIGBK的起始地址
			break;
		case 1:
			ftinfo.f12addr   = ftinfo.ugbkaddr + ftinfo.ugbksize;			//UNIGBK之后，紧跟GBK12字库
			ftinfo.gbk12size = file->obj.objsize;											//GBK12字库大小
			flashaddr        = ftinfo.f12addr;												//GBK12的起始地址
			break;
		case 2:
			ftinfo.f16addr   = ftinfo.f12addr + ftinfo.gbk12size;			//GBK12之后，紧跟GBK16字库
			ftinfo.gbk16size = file->obj.objsize;											//GBK16字库大小
			flashaddr        = ftinfo.f16addr;												//GBK16的起始地址
			break;
		case 3:
			ftinfo.f24addr   = ftinfo.f16addr + ftinfo.gbk16size;			//GBK16之后，紧跟GBK24字库
			ftinfo.gbk24size = file->obj.objsize;											//GBK24字库大小
			flashaddr        = ftinfo.f24addr;												//GBK24的起始地址
			break;
		case 4:
			ftinfo.f32addr   = ftinfo.f24addr + ftinfo.gbk24size;			//GBK24之后，紧跟GBK32字库
			ftinfo.gbk32size = file->obj.objsize;											//GBK32字库大小
			flashaddr        = ftinfo.f32addr;												//GBK32的起始地址
			break;
	} 
			
	while(1)
	{
		if(f_read(file,tempbuf,MX25L512_SECTOR_SIZE,&br))						//读取数据
		{
			myfree(SRAM0,tempbuf);																		//释放内存
			return 3;
		}
		MX25L512_Write(tempbuf,offx + flashaddr,br);								//写入数据  
		LCD_printf("update font %s %d%%\r",filepath,cal_rate(file->obj.objsize,offx));		//进度显示					
		if(br != MX25L512_SECTOR_SIZE)															//读完了
		{
			break;								
		}
		offx += br;
	}
	
	f_close(file);
	
	LCD_printf("update font %s %d%%\r\n",filepath,100);
	
	myfree(SRAM0,tempbuf);	//释放内存
	
	return 0;
} 

u8 update_font(void)
{	
	u16 i,j;
	u8  *pname;
	u32 *buf;

	ftinfo.fontok = 0XFF;
	
	pname = mymalloc(SRAM0,100);	  													//申请100字节存放文件名
	buf   = mymalloc(SRAM0,MX25L512_SECTOR_SIZE);							//申请MX25L512_SECTOR_SIZE字节内存 
	
	if(buf == NULL || pname == NULL)
	{
		myfree(SRAM0,pname);
		myfree(SRAM0,buf);
		return FONT_MALLOC_ERROR;			
	}
	
	for(i = 0;i < 5;i++)	
	{ 
		strcpy((char*)pname,(char*)GBK_PATH[i]);		    				//拷贝文件名
		if(f_open(file,(const TCHAR*)pname,FA_READ))
		{
			myfree(SRAM0,pname);
			myfree(SRAM0,buf);
			return FONT_FILE_ERROR;
		}
		f_close(file);
	} 
	
	for(i = 0;i < FONT_SECTOR_NUM;i++)					
	{
		LCD_printf("Erasing sectors... %d%%\r",cal_rate(FONT_SECTOR_NUM,i));
		
		MX25L512_Read((u8*)buf,(FONT_START_ADDR + (i * MX25L512_SECTOR_SIZE)),MX25L512_SECTOR_SIZE);
		
		for(j = 0;j < MX25L512_SECTOR_SIZE/4;j++)
		{
			if(buf[j] != 0XFFFFFFFF)															//需要擦除  	  
			{
				break;
			}
		}
		if(j != MX25L512_SECTOR_SIZE/4)
		{
			MX25L512_Erase_Sector(FONT_START_ADDR + (i * MX25L512_SECTOR_SIZE));	//擦除扇区
		}
	}
	LCD_printf("Erasing sectors... %d%%\r\n",100);
	
	for(i = 0;i < 5;i++)																			//依次更新UNIGBK,GBK12,GBK16,GBK24,GBK32
	{		
		strcpy((char*)pname,(char*)GBK_PATH[i]); 								//拷贝文件名
		
		if(updata_fontx(pname,i))         											//更新字库
		{
			myfree(SRAM0,buf);
			myfree(SRAM0,pname);
			return 1+i;
		} 
	} 

	ftinfo.fontok=0XAA;                                       //更新字库信息头OK标志
	MX25L512_Write((u8*)&ftinfo,FONT_START_ADDR,sizeof(ftinfo));	//保存字库信息头

	myfree(SRAM0,pname);																			//释放内存 
	myfree(SRAM0,buf);
	
	return FONT_UPDATE_OK;
} 

