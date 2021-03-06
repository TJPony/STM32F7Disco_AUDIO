#ifndef _SDCARD_H
#define _SDCARD_H
#include "sys.h"

#define   SD_TIMEOUT      								 ((uint32_t)100000000)       //��ʱʱ��

#define   BLOCK_SIZE                       (512)

#define   SDCARD_OK                        ((uint8_t)0x00)
#define   SDCARD_ERROR                     ((uint8_t)0x01)
#define   SDCARD_NO_DETECTED      				 ((uint8_t)0x02)

typedef struct 
{
	bool RD_finish;
	bool WR_finish;
	HAL_SD_CardInfoTypeDef info;
	 u8 (*init)(void);
	 u8 (*read)(u8 *pData, u32 sector, u32 cnt);
	 u8 (*write)(u8 *pData, u32 sector, u32 cnt);
}T_SDCardOptr,*PT_SDCardOptr;

PT_SDCardOptr Get_SDCardOptr(void);

#endif
