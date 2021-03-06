#include "IIC.h"
#include "delay.h"

#if (I2C_HW_MODE == 1)

static I2C_HandleTypeDef I2C4_Handler;
void IIC_Init(void)
{
		I2C4_Handler.Instance = I2C4;
    I2C4_Handler.Init.Timing           = DISCOVERY_I2Cx_TIMING;
    I2C4_Handler.Init.OwnAddress1      = 0;
    I2C4_Handler.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
    I2C4_Handler.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
    I2C4_Handler.Init.OwnAddress2      = 0;
    I2C4_Handler.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
    I2C4_Handler.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;

    HAL_I2C_Init(&I2C4_Handler);
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
		GPIO_InitTypeDef GPIO_Initure;
    __HAL_RCC_I2C4_CLK_ENABLE();
    
    GPIO_Initure.Pin=GPIO_PIN_12;  
    GPIO_Initure.Mode=GPIO_MODE_AF_OD;          //开漏复用
    GPIO_Initure.Pull=GPIO_NOPULL;              //无上拉
    GPIO_Initure.Speed=GPIO_SPEED_FREQ_HIGH;    //高速
    GPIO_Initure.Alternate=GPIO_AF4_I2C4;       //复用为I2C4  
    HAL_GPIO_Init(GPIOD,&GPIO_Initure);         //初始化
	
		GPIO_Initure.Pin=GPIO_PIN_7;
		GPIO_Initure.Alternate=GPIO_AF11_I2C4;      //复用为I2C4  
	  HAL_GPIO_Init(GPIOB,&GPIO_Initure);         //初始化
}

static void I2Cx_Error(I2C_HandleTypeDef *i2c_handler, uint8_t Addr)
{
  /* De-initialize the I2C communication bus */
  HAL_I2C_DeInit(i2c_handler);
  
  /* Re-Initialize the I2C communication bus */
  IIC_Init();
}

static HAL_StatusTypeDef I2Cx_WriteMultiple(I2C_HandleTypeDef *i2c_handler, uint8_t Addr, uint16_t Reg, uint16_t MemAddress, uint8_t *Buffer, uint16_t Length)
{
  HAL_StatusTypeDef status = HAL_OK;

  status = HAL_I2C_Mem_Write(i2c_handler, Addr, (uint16_t)Reg, MemAddress, Buffer, Length, 1000);

  /* Check the communication status */
  if(status != HAL_OK)
  {
    /* Re-Initiaize the I2C Bus */
    I2Cx_Error(i2c_handler, Addr);
  }
  return status;
}

static HAL_StatusTypeDef I2Cx_ReadMultiple(I2C_HandleTypeDef *i2c_handler, uint8_t Addr, uint16_t Reg, uint16_t MemAddress, uint8_t *Buffer, uint16_t Length)
{
  HAL_StatusTypeDef status = HAL_OK;

  status = HAL_I2C_Mem_Read(i2c_handler, Addr, (uint16_t)Reg, MemAddress, Buffer, Length, 1000);

  /* Check the communication status */
  if(status != HAL_OK)
  {
    /* I2C error occured */
    I2Cx_Error(i2c_handler, Addr);
  }
  return status;
}

void CODEC_IO_Write(uint8_t Addr, uint16_t Reg, uint16_t Value)
{
  uint16_t tmp = Value;
  
  Value = ((uint16_t)(tmp >> 8) & 0x00FF);
  
  Value |= ((uint16_t)(tmp << 8)& 0xFF00);
  
  I2Cx_WriteMultiple(&I2C4_Handler, Addr, Reg, I2C_MEMADD_SIZE_16BIT,(uint8_t*)&Value, 2);
}

uint16_t CODEC_IO_Read(uint8_t Addr, uint16_t Reg)
{
  uint16_t read_value = 0, tmp = 0;
  
  I2Cx_ReadMultiple(&I2C4_Handler, Addr, Reg, I2C_MEMADD_SIZE_16BIT, (uint8_t*)&read_value, 2);
  
  tmp = ((uint16_t)(read_value >> 8) & 0x00FF);
  
  tmp |= ((uint16_t)(read_value << 8)& 0xFF00);
  
  read_value = tmp;
  
  return read_value;
}

void TS_IO_Write(uint8_t Addr, uint8_t Reg, uint8_t Value)
{
  I2Cx_WriteMultiple(&hI2cAudioHandler, Addr, (uint16_t)Reg, I2C_MEMADD_SIZE_8BIT,(uint8_t*)&Value, 1);
}

uint8_t TS_IO_Read(uint8_t Addr, uint8_t Reg)
{
  uint8_t read_value = 0;

  I2Cx_ReadMultiple(&hI2cAudioHandler, Addr, Reg, I2C_MEMADD_SIZE_8BIT, (uint8_t*)&read_value, 1);

  return read_value;
}

uint16_t TS_IO_ReadMultiple(uint8_t Addr, uint8_t Reg, uint8_t *Buffer, uint16_t Length)
{
 return I2Cx_ReadMultiple(&hI2cAudioHandler, Addr, (uint16_t)Reg, I2C_MEMADD_SIZE_8BIT, Buffer, Length);
}

#else
void IIC_Init(void)
{
    GPIO_InitTypeDef GPIO_Initure;

    GPIO_Initure.Pin=GPIO_PIN_7;
    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_OD;  //推挽输出
    GPIO_Initure.Pull=GPIO_NOPULL;          //
    GPIO_Initure.Speed=GPIO_SPEED_FAST;     //快速
    HAL_GPIO_Init(GPIOB,&GPIO_Initure);
		
	  GPIO_Initure.Pin=GPIO_PIN_12;
	  HAL_GPIO_Init(GPIOD,&GPIO_Initure);
	
		IIC_SCL_H;
	  IIC_SDA_H;
}

//产生IIC起始信号
static void IIC_Start(void)
{
	SDA_OUT();    
	IIC_SDA_H;  
	IIC_SCL_H;
	delay_us(4);
 	IIC_SDA_L;
	delay_us(4);
	IIC_SCL_L;
}	  
//产生IIC停止信号
static void IIC_Stop(void)
{
	SDA_OUT();
	IIC_SCL_L;
	IIC_SDA_L;
 	delay_us(4);
	IIC_SCL_H; 
	delay_us(4);
  IIC_SDA_H; 	
}
//等待应答信号到来
//返回值：1，接收应答失败
//        0，接收应答成功
static u8 IIC_Wait_Ack(void)
{
	u8 ucErrTime=0;
	SDA_IN();   
	IIC_SDA_H;  
	delay_us(1);	   
	IIC_SCL_H;
	delay_us(1);	 
	while(READ_SDA)
	{
		ucErrTime++;
		if(ucErrTime>250)
		{
			IIC_Stop();
			return 1;
		}
	}
	IIC_SCL_L;
	return 0;  
} 
//产生ACK应答
static void IIC_Ack(void)
{
	IIC_SCL_L;
	SDA_OUT();
	IIC_SDA_L;
	delay_us(2);
	IIC_SCL_H;
	delay_us(2);
	IIC_SCL_L;
}
//不产生ACK应答		    
static void IIC_NAck(void)
{
	IIC_SCL_L;
	SDA_OUT();
	IIC_SDA_H;
	delay_us(2);
	IIC_SCL_H;
	delay_us(2);
	IIC_SCL_L;
}					 				     
//IIC发送一个字节	  
static void IIC_Send_Byte(u8 txd)
{                        
    u8 t;   
	  SDA_OUT(); 	    
    IIC_SCL_L;//拉低时钟开始数据传输
    for(t=0;t<8;t++)
    {      
       if(txd&0x80)
			 {
         IIC_SDA_H;
			 }
			 else
			 {
				 IIC_SDA_L;
			 }
        txd<<=1; 	  
				delay_us(2); 
				IIC_SCL_H;
				delay_us(2); 
				IIC_SCL_L;
				delay_us(2);
    }	
} 	    
//读1个字节，ack=1时，发送ACK，ack=0，发送nACK   
static u8 IIC_Read_Byte(unsigned char ack)
{
	  unsigned char i,receive=0;
	  SDA_IN();
    for(i=0;i<8;i++ )
	  {
        IIC_SCL_L;
        delay_us(2);
		    IIC_SCL_H;
        receive<<=1;
        if(READ_SDA)receive++;   
		    delay_us(1); 
    }					 
    if (!ack)
        IIC_NAck();//发送nACK
    else
        IIC_Ack(); //发送ACK   
    return receive;
}

void CODEC_IO_Write(uint8_t Addr, uint16_t Reg, uint16_t Value)
{
  IIC_Start(); 
	IIC_Send_Byte(Addr);							//发送器件地址+写命令	 
	if(IIC_Wait_Ack())return;	      //等待应答(成功?/失败?) 
  IIC_Send_Byte(Reg>>8);						//写寄存器地址高8位
	if(IIC_Wait_Ack())return;       //等待应答(成功?/失败?) 
	IIC_Send_Byte(Reg);								//写寄存器地址低8位
	if(IIC_Wait_Ack())return;	      //等待应答(成功?/失败?) 
	IIC_Send_Byte(Value>>8);	        //发送数据高8位
	if(IIC_Wait_Ack())return;	      //等待应答(成功?/失败?)
	IIC_Send_Byte(Value);	        		//发送数据低8位
	if(IIC_Wait_Ack())return;	      //等待应答(成功?/失败?)
  IIC_Stop();
}

uint16_t CODEC_IO_Read(uint8_t Addr, uint16_t Reg)
{
  uint16_t result = 0;
	uint8_t temp;
	
  IIC_Start();
  IIC_Send_Byte(Addr);							//发送器件地址+写命令
  if(IIC_Wait_Ack())return 1;	      //等待应答(成功?/失败?) 
  IIC_Send_Byte(Reg>>8);						//写寄存器地址高8位
	if(IIC_Wait_Ack())return 2;       //等待应答(成功?/失败?) 
	IIC_Send_Byte(Reg);								//写寄存器地址低8位
	if(IIC_Wait_Ack())return 3;	      //等待应答(成功?/失败?) 	
	IIC_Start();  	 	   
	IIC_Send_Byte(Addr+1);            //进入接收模式			   
	IIC_Wait_Ack();	 
  temp = IIC_Read_Byte(1);
	result = temp;
	temp = IIC_Read_Byte(0);
	result <<= 8;
	result += temp;
  IIC_Stop();												//产生一个停止条件	    
	return result;
}

void TS_IO_Write(uint8_t Addr, uint8_t Reg, uint8_t Value)
{
  IIC_Start(); 
	IIC_Send_Byte(Addr);							//发送器件地址+写命令	 
	if(IIC_Wait_Ack())return;	      	//等待应答(成功?/失败?) 
	IIC_Send_Byte(Reg);								//写寄存器地址低8位
	if(IIC_Wait_Ack())return;	      	//等待应答(成功?/失败?)
	IIC_Send_Byte(Value);	        		//发送数据低8位
	if(IIC_Wait_Ack())return;	      	//等待应答(成功?/失败?)
  IIC_Stop();
}

uint8_t TS_IO_Read(uint8_t Addr, uint8_t Reg)
{
  uint8_t result = 0;
	
  IIC_Start();
  IIC_Send_Byte(Addr);							//发送器件地址+写命令
  IIC_Wait_Ack();	      
	IIC_Send_Byte(Reg);								//写寄存器地址低8位
	IIC_Wait_Ack();	 
	IIC_Start();  	 	   
	IIC_Send_Byte(Addr+1);            //进入接收模式			   
	IIC_Wait_Ack();	 
	result = IIC_Read_Byte(0);
  IIC_Stop();												//产生一个停止条件	    
	return result;
}

uint16_t TS_IO_ReadMultiple(uint8_t Addr, uint8_t Reg, uint8_t *Buffer, uint16_t Length)
{
	uint8_t i;
	
  IIC_Start();
  IIC_Send_Byte(Addr);							//发送器件地址+写命令
  IIC_Wait_Ack();	      
	IIC_Send_Byte(Reg);								//写寄存器地址低8位
	IIC_Wait_Ack();	 
	IIC_Start();  	 	   
	IIC_Send_Byte(Addr+1);            //进入接收模式			   
	IIC_Wait_Ack();
  for(i = 0;i < Length;i++)
  {	
		if(i == (Length - 1))
		{
			Buffer[i] = IIC_Read_Byte(0);
		}
		else
		{
			Buffer[i] = IIC_Read_Byte(1);
		}
	}
  IIC_Stop();												//产生一个停止条件	    
	return i;
}

#endif
