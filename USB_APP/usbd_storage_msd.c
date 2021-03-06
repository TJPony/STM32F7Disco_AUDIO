#include "usbd_msc_mem.h"
#include "w25qxx.h"
#include "ftl.h"

#define STORAGE_LUN_NBR                  2 

const int8_t  STORAGE_Inquirydata[] = {
  
  /* LUN 0 W25QXX*/
  0x00,		
  0x80,		//RMB = 1 可移除设备
  0x02,		
  0x02,
  (USBD_STD_INQUIRY_LENGTH - 5),
  0x00,
  0x00,	
  0x00,
  'M', 'X', 'L', ' ', ' ', ' ', ' ', ' ', /* Manufacturer : 8 bytes */
  'W', '2', '5', 'Q', 'X', 'X', ' ', ' ', /* Product      : 16 Bytes */
  'F', 'l', 'a', 's', 'h', ' ', ' ', ' ',
  '1', '.', '0' ,' ',                     /* Version      : 4 Bytes */
	/* LUN 1 NandFlash*/
  0x00,		
  0x80,		//RMB = 1 可移除设备
  0x02,		
  0x02,
  (USBD_STD_INQUIRY_LENGTH - 5),
  0x00,
  0x00,	
  0x00,
  'S', 'T', 'M', ' ', ' ', ' ', ' ', ' ', /* Manufacturer : 8 bytes */
  'N', 'a', 'n', 'd', ' ', ' ', ' ', ' ', /* Product      : 16 Bytes */
  'F', 'l', 'a', 's', 'h', ' ', ' ', ' ',
  '1', '.', '0' ,' ',                     /* Version      : 4 Bytes */
}; 

int8_t STORAGE_Init (uint8_t lun);
int8_t STORAGE_GetCapacity (uint8_t lun,uint32_t *block_num,uint32_t *block_size);
int8_t STORAGE_IsReady (uint8_t lun);
int8_t STORAGE_IsWriteProtected (uint8_t lun);
int8_t STORAGE_Read (uint8_t lun,uint8_t *buf,uint32_t blk_addr,uint16_t blk_len);
int8_t STORAGE_Write (uint8_t lun,uint8_t *buf,uint32_t blk_addr,uint16_t blk_len);
int8_t STORAGE_GetMaxLun (void);

USBD_STORAGE_cb_TypeDef USBD_MSC_BSP_fops =
{
  STORAGE_Init,
  STORAGE_GetCapacity,
  STORAGE_IsReady,
  STORAGE_IsWriteProtected,
  STORAGE_Read,
  STORAGE_Write,
  STORAGE_GetMaxLun,
  (int8_t *)STORAGE_Inquirydata,
};

USBD_STORAGE_cb_TypeDef  *USBD_STORAGE_fops = &USBD_MSC_BSP_fops;

static PT_W25Qxx   g_ptW25Q256;
static PT_NandOptr g_ptNandOptr;

int8_t STORAGE_Init (uint8_t lun)
{
  switch(lun)
	{
		case 0:// NAND FLASH
			g_ptNandOptr = Get_NandOptr();
			if(g_ptNandOptr->init())
			{
				printf("NandFlash init failed! \r\n");
				return -1;
			}
			else
			{
				printf("NandFlash init Finish! \r\n");
				printf("total block num:%d\r\n",g_ptNandOptr->attr->block_totalnum);
				printf("good block num:%d\r\n",g_ptNandOptr->attr->good_blocknum);
				printf("valid block num:%d\r\n",g_ptNandOptr->attr->valid_blocknum);
				printf("block_pagenum:%d\r\n",g_ptNandOptr->attr->block_pagenum);
				printf("page_mainsize:%d\r\n",g_ptNandOptr->attr->page_mainsize);
				printf("page_sparesize:%d\r\n",g_ptNandOptr->attr->page_sparesize);
				printf("NandFlash totalsize:%dMB\r\n",(g_ptNandOptr->attr->block_totalnum/1024)*(g_ptNandOptr->attr->page_mainsize/1024)*g_ptNandOptr->attr->block_pagenum);
			}
			break;
		case 1:// W25QXX
			g_ptW25Q256 = Get_W25Qxx();
			if(g_ptW25Q256->init() == -1)
			{
				printf("W25Q256 init failed! \r\n");
				return -1;
			}
			else
			{
				printf("W25Q256 init Finish! Size : 32MB\r\n");
			}
			break;
		default:break;
	} 
  return 0;  
}

int8_t STORAGE_GetCapacity (uint8_t lun, uint32_t *block_num, uint32_t *block_size)
{
	switch(lun)
		{
			case 0:// NAND FLASH
				*block_size = NAND_ECC_SECTOR_SIZE;  
				*block_num  = g_ptNandOptr->attr->valid_blocknum * g_ptNandOptr->attr->block_pagenum * g_ptNandOptr->attr->page_mainsize / NAND_ECC_SECTOR_SIZE;
				break;
			case 1:// W25QXX
				*block_size = W25QXX_SECTOR_SIZE;  
				*block_num  = W25QXX_SECTOR_COUNT;
				break;
      default:break;
		}  	
	return 0; 
}

int8_t  STORAGE_IsReady (uint8_t lun)
{
  return 0;
}
int8_t  STORAGE_IsWriteProtected (uint8_t lun)
{
  return  0;
}
int8_t STORAGE_Read (uint8_t lun,uint8_t *buf,uint32_t blk_addr,uint16_t blk_len)
{
  int8_t res=0;
	switch(lun)
	{
		case 0:// NAND FLASH
			res = g_ptNandOptr->read(buf,blk_addr,NAND_ECC_SECTOR_SIZE,blk_len);
			break;
		case 1:// W25QXX
			g_ptW25Q256->read(buf,blk_addr*W25QXX_SECTOR_SIZE,blk_len*W25QXX_SECTOR_SIZE);
			break;
		default:break;
	} 
	if(res)
	{
		return -1;
	} 
	return 0;
}

int8_t STORAGE_Write (uint8_t lun,uint8_t *buf,uint32_t blk_addr,uint16_t blk_len)
{
	  int8_t res=0;
		switch(lun)
		{
			case 0:// NAND FLASH
				res = g_ptNandOptr->write(buf,blk_addr,NAND_ECC_SECTOR_SIZE,blk_len);
				break;
			case 1:// W25QXX
				g_ptW25Q256->write(buf,blk_addr*W25QXX_SECTOR_SIZE,blk_len*W25QXX_SECTOR_SIZE);
				break;
		  default:break;
		}  
		if(res)
		{
			return -1;
		}
		return 0; 
}

int8_t STORAGE_GetMaxLun (void)
{
  return (STORAGE_LUN_NBR - 1);
}
