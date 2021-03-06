#include "string.h"
#include "file.h"	
#include "malloc.h"
#include "usart.h"
#include "lcd.h"

#define FILE_MAX_TYPE_NUM		7	//最多FILE_MAX_TYPE_NUM个大类
#define FILE_MAX_SUBT_NUM		4	//最多FILE_MAX_SUBT_NUM个小类

 //文件类型列表
u8* const FILE_TYPE_TBL[FILE_MAX_TYPE_NUM][FILE_MAX_SUBT_NUM] =
{
	{"BIN"},			             //BIN文件
	{"LRC"},			             //LRC文件
	{"NES","SMS"},		         //NES/SMS文件
	{"TXT","C","H"},	         //文本文件
	{"WAV","MP3","APE","FLAC"},//支持的音乐文件
	{"BMP","JPG","JPEG","GIF"},//图片文件
	{"AVI"},			             //视频文件
};

FATFS *fs[_VOLUMES];         //逻辑磁盘工作区.	 
FIL *file;	  		           //文件
FIL *ftemp;	  		           //临时文件
UINT br,bw;			             //读写变量
FILINFO fileinfo;	           //文件信息
DIR dir;  			             //目录

u8 Fatfs_init(void)
{
	u8 i;
	for(i=0;i<_VOLUMES;i++)
	{
		fs[i]=(FATFS*)mymalloc(SRAM0,sizeof(FATFS));	//为磁盘i工作区申请内存	
		if(!fs[i])break;
	}
	file=(FIL*)mymalloc(SRAM0,sizeof(FIL));		//为file申请内存
	ftemp=(FIL*)mymalloc(SRAM0,sizeof(FIL));	//为ftemp申请内存
	if(i==_VOLUMES && file && ftemp)return 0; //申请有一个失败,即失败.
	else return 1;	
}

//将小写字母转为大写字母,如果是数字,则保持不变.
u8 char_upper(u8 c)
{
	if(c<'A')return c;//数字,保持不变.
	if(c>='a')return c-0x20;//变为大写.
	else return c;//大写,保持不变
}	      
//报告文件的类型
//fname:文件名
//返回值:0XFF,表示无法识别的文件类型编号.
//		 其他,高四位表示所属大类,低四位表示所属小类.
u8 f_typetell(u8 *fname)
{
	u8 tbuf[5];
	u8 *attr='\0';//后缀名
	u8 i=0,j;
	while(i<250)
	{
		i++;
		if(*fname=='\0')break;//偏移到了最后了.
		fname++;
	}
	if(i==250)return 0XFF;//错误的字符串.
 	for(i=0;i<5;i++)//得到后缀名
	{
		fname--;
		if(*fname=='.')
		{
			fname++;
			attr=fname;
			break;
		}
  	}
	strcpy((char *)tbuf,(const char*)attr);//copy
 	for(i=0;i<4;i++)tbuf[i]=char_upper(tbuf[i]);//全部变为大写 
	for(i=0;i<FILE_MAX_TYPE_NUM;i++)	//大类对比
	{
		for(j=0;j<FILE_MAX_SUBT_NUM;j++)//子类对比
		{
			if(*FILE_TYPE_TBL[i][j]==0)break;//此组已经没有可对比的成员了.
			if(strcmp((const char *)FILE_TYPE_TBL[i][j],(const char *)tbuf)==0)//找到了
			{
				return (i<<4)|j;
			}
		}
	}
	return 0XFF;//没找到		 			   
}	 

//得到磁盘剩余容量
//drv:磁盘编号("0:"/"1:")
//total:总容量	 （单位KB）
//free:剩余容量	 （单位KB）
//返回值:0,正常.其他,错误代码
u8 Fatfs_getfree(u8 *drv,u32 *total,u32 *free)
{
	  FATFS *fs1;
	u8 res;
    u32 fre_clust=0, fre_sect=0, tot_sect=0;
    //得到磁盘信息及空闲簇数量
    res =(u32)f_getfree((const TCHAR*)drv, (DWORD*)&fre_clust, &fs1);
    if(res==0)
	{											   
	    tot_sect=(fs1->n_fatent-2)*fs1->csize;	//得到总扇区数
	    fre_sect=fre_clust*fs1->csize;					//得到空闲扇区数	   
#if _MAX_SS!=512				  										//扇区大小不是512字节,则转换为512字节
		tot_sect*=fs1->ssize/512;
		fre_sect*=fs1->ssize/512;
#endif	  
		*total=tot_sect>>1;	//单位为KB
		*free=fre_sect>>1;	//单位为KB 
 	}
	return res;
}		   

u8 Scan_files(u8 * path)
{
	FRESULT res;	 
  res = f_opendir(&dir,(const TCHAR*)path); //打开一个目录
  if (res == FR_OK) 
	{	
		LCD_printf("\r\n"); 
		while(1)
		{
	    res = f_readdir(&dir, &fileinfo);                   //读取目录下的一个文件
	    if (res != FR_OK || fileinfo.fname[0] == 0) break;  //错误了/到末尾了,退出
 			LCD_printf("%s/", path);//打印路径	
			LCD_printf("%s\r\n",fileinfo.fname);//打印文件名	  
		} 
  }	   
  return res;	  
}

u16 Get_Pic_Num(u8 *path)
{	  
	u8 res;
	u16 rval=0; 			     
  res=f_opendir(&dir,(const TCHAR*)path); 	//打开目录 
	if(res==FR_OK)
	{
		while(1)//查询总的有效文件数
		{
	    res = f_readdir(&dir,&fileinfo);       		//读取目录下的一个文件  	 
	    if(res != FR_OK || fileinfo.fname[0] == 0)break;//错误了/到末尾了,退出	 		 
			res = f_typetell((u8*)fileinfo.fname);
			if(res == T_JPG || res == T_JPEG)
			{
				rval++;//有效文件数增加1
			  LCD_printf("%s\r\n",fileinfo.fname);//打印文件名	  
			}	    
		}  
	}  
	return rval;
}

u16 Get_AVI_Num(u8 *path)
{	  
	u8 res;
	u16 rval=0; 			     
  res=f_opendir(&dir,(const TCHAR*)path); 	//打开目录 
	if(res==FR_OK)
	{
		while(1)//查询总的有效文件数
		{
	    res = f_readdir(&dir,&fileinfo);       		      //读取目录下的一个文件  	 
	    if(res != FR_OK || fileinfo.fname[0] == 0)break;//错误了/到末尾了,退出	 		 
			res = f_typetell((u8*)fileinfo.fname);
			if(res == T_AVI)
			{
				rval++;//有效文件数增加1
			  LCD_printf("%s\r\n",fileinfo.fname);//打印文件名	  
			}	    
		}  
	}  
	return rval;
}

u16 Get_WAV_Num(u8 *path)
{	  
	u8 res;
	u16 rval=0;	
  res=f_opendir(&dir,(const TCHAR*)path);   //打开目录 
	if(res==FR_OK)
	{
		while(1)//查询总的有效文件数
		{
	    res=f_readdir(&dir,&fileinfo);       			      //读取目录下的一个文件
	    if(res != FR_OK || fileinfo.fname[0] == 0)break;//错误了/到末尾了,退出	 		 
			res = f_typetell((u8*)fileinfo.fname);	
			if(res == T_WAV)
			{
				rval++;//有效文件数增加1
				LCD_printf("%s\r\n",fileinfo.fname);//打印文件名
			}	    
		}  
	}  
	return rval;
}
