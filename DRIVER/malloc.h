#ifndef __MALLOC_H
#define __MALLOC_H
#include "sys.h" 
 
#ifndef NULL
#define NULL 0
#endif
 
#define SRAM0	 0		//内部内存(STM32F769 SRAM1+SRAM2 = 384K)
#define SRAM1  1		//外部内存池(MT48LC4M32B2B5-6A内存大小16M)
#define SRAM2  2		//DTCM内存池(STM32F769 DTCM大小为128K,此部分SRAM仅仅CPU可以访问)


#define SRAMBANK 	3	//定义支持的SRAM块数.	

#define MEM1_BLOCK_SIZE		64  	  						  //内存块大小为64字节
#define MEM1_MAX_SIZE			100*1024  						//最大管理内存 200K
#define MEM1_ALLOC_TABLE_SIZE	MEM1_MAX_SIZE/MEM1_BLOCK_SIZE 	//内存表大小以字为单位

#define MEM2_BLOCK_SIZE		64  	  							//内存块大小为64字节
#define MEM2_MAX_SIZE			13*1024*1024  				//最大管理内存13M
#define MEM2_ALLOC_TABLE_SIZE	MEM2_MAX_SIZE/MEM2_BLOCK_SIZE 	//内存表大小以字为单位
		 
//mem3内存参数设定.mem3处于CCM,用于管理CCM(特别注意,这部分SRAM,仅CPU可以访问!!)
#define MEM3_BLOCK_SIZE		64  	  							//内存块大小为64字节
#define MEM3_MAX_SIZE			120*1024  						//最大管理内存120K
#define MEM3_ALLOC_TABLE_SIZE	MEM3_MAX_SIZE/MEM3_BLOCK_SIZE 	//内存表大小以字为单位
		 

//内存管理控制器
typedef struct t_MemOptr
{
	u8 	*membase[SRAMBANK];				//内存池 管理SRAMBANK个区域的内存
	u32 *memmap[SRAMBANK]; 				//内存管理状态表
	u8  memrdy[SRAMBANK]; 				//内存管理是否就绪
	u16 peruse0;
	u16 peruse1;
	u16 peruse2;
	void (*init)(u8);					    //初始化
	u16 (*perused)(u8);		  	    //内存使用率
}T_MemOptr,*PT_MemOptr;

void mymemset(void *s,u8 c,u32 count);	
void mymemcpy(void *des,void *src,u32 n);			
void myfree(u8 memx,void *ptr);  			
void *mymalloc(u8 memx,u32 size);			
PT_MemOptr Get_MemOptr(void);

#endif













