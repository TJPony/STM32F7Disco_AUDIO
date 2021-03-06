#ifndef _JPEG_HW_DECODE_H
#define _JPEG_HW_DECODE_H
#include "sys.h"
#include "jpeg_utils.h"

#define JPEG_BUFFER_EMPTY     	0
#define JPEG_BUFFER_FULL      	1
	
#define JPEG_STATE_NOHEADER			0					//HEADER未读取,初始状态
#define JPEG_STATE_HEADEROK			1					//HEADER读取成功
#define JPEG_STATE_FINISHED			2					//解码完成

#define JPEG_DMA_INBUF_LEN			4096			//DMA IN  BUF的大小
#define JPEG_DMA_OUTBUF_LEN  		768 		  //DMA OUT BUF的大小

#define NB_INPUT_DATA_BUFFERS   2
#define NB_OUTPUT_DATA_BUFFERS  2


typedef enum
{
	PIC_NO_ERR      = 0,
	PIC_FORMAT_ERR	= 1,	 //格式错误
	PIC_SIZE_ERR		= 2,   //图片尺寸错误
	PIC_WINDOW_ERR  = 3,	 //窗口设定错误
	PIC_MEM_ERR			= 4, 	 //内存错误
	PIC_FILE_ERR    = 5    //图片文件错误
}PIC_StatusTypeDef;

typedef struct
{
		u8 sta;							 //缓冲区状态 0:空 1:满
    u8 *buf;						 //JPEG数据缓冲区
    u16 size; 					 //JPEG数据长度 
}jpeg_databuf_type; 

//jpeg编解码控制结构体
typedef struct
{ 
	JPEG_ConfTypeDef	info;             							//当前JPEG文件相关参数
	jpeg_databuf_type inbuf[NB_INPUT_DATA_BUFFERS];		//DMA IN buf
	jpeg_databuf_type outbuf[NB_OUTPUT_DATA_BUFFERS];	//DMA OUT buf
	
	u32 JpegProcessing_End;
	
	vu8 inbuf_read_ptr;																//DMA IN buf当前读取位置
	vu8 inbuf_write_ptr;															//DMA IN buf当前写入位置
	vu8 indma_pause;																	//输入DMA暂停状态标识
	vu8 outbuf_read_ptr;															//DMA OUT buf当前读取位置
	vu8 outbuf_write_ptr;															//DMA OUT buf当前写入位置
	vu8 outdma_pause;																	//输入DMA暂停状态标识
	vu8 state;																				//解码状态;0,未识别到Header;1,识别到了Header;2,解码完成;
	u32 blkindex;																			//当前block编号
	u32 total_blks;																		//jpeg文件总block数
	u32 (*yuv2rgb)(u8 *,u8 *,u32 ,u32 ,u32 *);				//颜色转换函数指针
}jpeg_codec_typedef;

extern JPEG_HandleTypeDef  JPEG_Handler;    			//JPEG句柄
PIC_StatusTypeDef Show_Picture(const u8 *filename,u16 x,u16 y,u16 width,u16 height);

#endif
