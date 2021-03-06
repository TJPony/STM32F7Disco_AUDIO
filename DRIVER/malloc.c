#include "malloc.h"	    

//内存池(32字节对齐)
__align(32) u8 mem1base[MEM1_MAX_SIZE];																					//内部SRAM内存池
__align(32) u8 mem2base[MEM2_MAX_SIZE] __attribute__((at(0XC0177000)));					//外部SDRAM内存池 定义在LCD显存之后
__align(32) u8 mem3base[MEM3_MAX_SIZE] __attribute__((at(0X20000000)));					//内部CCM内存池
//内存管理表
u32 mem1mapbase[MEM1_ALLOC_TABLE_SIZE];													//内部SRAM内存池MAP
u32 mem2mapbase[MEM2_ALLOC_TABLE_SIZE] __attribute__((at(0XC0177000+MEM2_MAX_SIZE)));	//外部SRAM内存池MAP
u32 mem3mapbase[MEM3_ALLOC_TABLE_SIZE] __attribute__((at(0X20000000+MEM3_MAX_SIZE)));	//内部CCM内存池MAP
//内存管理参数	   
const u32 memtblsize[SRAMBANK]={MEM1_ALLOC_TABLE_SIZE,MEM2_ALLOC_TABLE_SIZE,MEM3_ALLOC_TABLE_SIZE};	//内存表大小
const u32 memblksize[SRAMBANK]={MEM1_BLOCK_SIZE,MEM2_BLOCK_SIZE,MEM3_BLOCK_SIZE};					//内存分块大小
const u32 memsize[SRAMBANK]={MEM1_MAX_SIZE,MEM2_MAX_SIZE,MEM3_MAX_SIZE};							//内存总大小

static void my_mem_init(u8 memx);				
static u32 my_mem_malloc(u8 memx,u32 size);	
static u8 my_mem_free(u8 memx,u32 offset);		
static u16 my_mem_perused(u8 memx) ;

static T_MemOptr g_tMemOptr = {
	.peruse0 = 0,
	.peruse1 = 0,
	.peruse2 = 0,
	.membase = {
		mem1base,
		mem2base,
		mem3base		
	},
  .memmap  = {
		mem1mapbase,
		mem2mapbase,
		mem3mapbase
	},
	.memrdy  = {
		0,
		0,
		0
	},
	.init    = my_mem_init,						
	.perused = my_mem_perused,					
};

void mymemcpy(void *des,void *src,u32 n)  
{  
    u8 *xdes=des;
	  u8 *xsrc=src; 
    while(n--)*xdes++=*xsrc++;  
}  

void mymemset(void *s,u8 c,u32 count)  
{  
    u8 *xs = s;  
    while(count--)*xs++=c;  
}	

static void my_mem_init(u8 memx)  
{  
    mymemset(g_tMemOptr.memmap[memx],0,memtblsize[memx]*4);	//内存状态表数据清零  
 	  g_tMemOptr.memrdy[memx]=1;															//内存管理初始化OK  
}  

static u16 my_mem_perused(u8 memx)  
{  
    u32 used=0;  
    u32 i;  
    for(i=0;i<memtblsize[memx];i++)  
    {  
      if(g_tMemOptr.memmap[memx][i])used++; 
    } 
    return (used*1000)/(memtblsize[memx]);  
}  

static u32 my_mem_malloc(u8 memx,u32 size)  
{  
    signed long offset=0;  
    u32 nmemb;																				//需要的内存块数  
		u32 cmemb=0;																			//连续空内存块数
    u32 i;  
	
    if(!g_tMemOptr.memrdy[memx])
		{
			g_tMemOptr.init(memx);													//未初始化,先执行初始化
		}
    if(size==0)
		{
			return 0XFFFFFFFF;
		}
    nmemb = size/memblksize[memx];  									//获取需要分配的连续内存块数
    if(size % memblksize[memx])
		{
			nmemb++;                                        //向上取整
		}
    for(offset = memtblsize[memx]-1;offset >= 0;offset--)//搜索整个内存控制区
    {     
			if(!g_tMemOptr.memmap[memx][offset])
			{
				cmemb++;																				//连续空内存块数增加
			}
			else 
			{
				cmemb=0;																				//连续内存块清零
			}
			if(cmemb == nmemb)																//找到了连续nmemb个空内存块
			{
				for(i=0;i<nmemb;i++)  					
				{  
						g_tMemOptr.memmap[memx][offset+i]=nmemb;    //标注内存块非空 
				}  
				return (offset * memblksize[memx]);//返回偏移地址  
			}
    }  
    return 0XFFFFFFFF;//未找到符合分配条件的内存块  
}  
  
static u8 my_mem_free(u8 memx,u32 offset)  
{  
    int i; 
	
    if(!g_tMemOptr.memrdy[memx])
		{
			g_tMemOptr.init(memx);    
      return 1;
    }  
    if(offset < memsize[memx])											//偏移在内存池内. 
    {  
        int index = offset / memblksize[memx];			//偏移所在内存块号码  
        int nmemb = g_tMemOptr.memmap[memx][index];	//内存块数量
        for(i = 0;i < nmemb;i++)  						
        {  
            g_tMemOptr.memmap[memx][index+i] = 0;   //内存块清零
        }  
        return 0;  
    }
		else 
		{
			return 2;
		}			
}  

void myfree(u8 memx,void *ptr)  
{  
	u32 offset;
	
	if(ptr == NULL)
	{
		return;
	}		
	
 	offset = (u32)ptr - (u32)g_tMemOptr.membase[memx];     
  my_mem_free(memx,offset);	    //释放内存      
}  

void *mymalloc(u8 memx,u32 size)  
{  
  u32 offset;
	
	offset = my_mem_malloc(memx,size);
	
  if(offset == 0XFFFFFFFF)
	{
		return NULL;
	}		
  else 
	{
		return (void*)((u32)g_tMemOptr.membase[memx]+offset);
	}		
}

PT_MemOptr Get_MemOptr(void)
{
	PT_MemOptr pTtemp;
	pTtemp = &g_tMemOptr;
	return pTtemp;
}

