#ifndef _LED_H
#define _LED_H
#include "sys.h"

#define LD_USER1   0
#define LD_USER2   1

typedef struct 
{
	 void (*init)(void);
	 void (*on)(u8 num);
	 void (*off)(u8 num);
	 void (*toggle)(u8 num);
}T_LedOptr,*PT_LedOptr;

PT_LedOptr Get_LedOptr(void);

#endif
