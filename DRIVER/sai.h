#ifndef __SAI_H
#define __SAI_H
#include "sys.h"

typedef struct 
{
	 void (*init)(u32 mode,u32 cpol,u32 datalen);
	 u8 (*set_samplerate)(u32 samplerate);
	 void (*dma_init)(u8* buf0,u8 *buf1,u16 num,u8 width);
	 void (*start)(void);
	 void (*stop)(void);
	 void (*dma_cplt_callback)(void);
}T_SaiOptr,*PT_SaiOptr;


PT_SaiOptr Get_SaiOptr(void);

#endif
