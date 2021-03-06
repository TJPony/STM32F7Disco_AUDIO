#ifndef _IIC_H
#define _IIC_H
#include "sys.h"

#define  I2C_HW_MODE    0

#if (I2C_HW_MODE == 1)
  #define DISCOVERY_I2Cx_TIMING                      ((uint32_t)0x40912732)
#else
	#define SDA_IN()  {GPIOB->MODER&=~(3<<(7*2));GPIOB->MODER|=0<<7*2;}	//PB7输入模式
	#define SDA_OUT() {GPIOB->MODER&=~(3<<(7*2));GPIOB->MODER|=1<<7*2;} //PB7输出模式

	#define IIC_SCL_H   (HAL_GPIO_WritePin(GPIOD,GPIO_PIN_12,GPIO_PIN_SET))
	#define IIC_SCL_L   (HAL_GPIO_WritePin(GPIOD,GPIO_PIN_12,GPIO_PIN_RESET))
	#define IIC_SDA_H   (HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_SET))
	#define IIC_SDA_L   (HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_RESET))
	#define READ_SDA    (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7))  
#endif

void IIC_Init(void);
void CODEC_IO_Write(uint8_t Addr, uint16_t Reg, uint16_t Value);
uint16_t CODEC_IO_Read(uint8_t Addr, uint16_t Reg);
void TS_IO_Write(uint8_t Addr, uint8_t Reg, uint8_t Value);
uint8_t TS_IO_Read(uint8_t Addr, uint8_t Reg);
uint16_t TS_IO_ReadMultiple(uint8_t Addr, uint8_t Reg, uint8_t *Buffer, uint16_t Length);

#endif
