/*****************************************************************************
文件名:  sdram.c
描述:    该文件为MT48LC4M32B2B5-6A底层驱动
作者:    马小龙
版本:    V1.0
时间:    2019年8月1日
修改记录:

*****************************************************************************/

#include "sdram.h"
#include "delay.h"

static SDRAM_HandleTypeDef SDRAM_Handler;   //SDRAM句柄

/************************************************
名    称: SDRAM_Send_Cmd
功    能: SDRAM命令发送函数
输入参数: bankx   --无符号8位整型 0:BANK5  1:BANK6
          cmd     --无符号8位整型 命令类型，参见宏定义
					refresh --无符号8位整型 自刷新次数
					regval  --无符号16位整型 模式寄存器值
输出参数: 无
返回值:   无符号8位整型 0:正常;1:失败.
************************************************/
static u8 SDRAM_Send_Cmd(u8 bankx,u8 cmd,u8 refresh,u16 regval)
{
    u32 target_bank=0;
    FMC_SDRAM_CommandTypeDef Command;
    
    if(bankx==0) target_bank=FMC_SDRAM_CMD_TARGET_BANK1;       
    else if(bankx==1) target_bank=FMC_SDRAM_CMD_TARGET_BANK2;   
    Command.CommandMode=cmd;                
    Command.CommandTarget=target_bank;      
    Command.AutoRefreshNumber=refresh;      
    Command.ModeRegisterDefinition=regval;  
    if(HAL_SDRAM_SendCommand(&SDRAM_Handler,&Command,0X1000)==HAL_OK) 
    {
        return 0;  
    }
    else return 1;    
}

/************************************************
名    称: SDRAM_Initialization_Sequence
功    能: 发送SDRAM初始化序列
输入参数: hsdram--SDRAM_HandleTypeDef型指针 ,SDRAM操作句柄
输出参数: 无
返回值:   无
************************************************/
static void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *hsdram)
{
    u32 temp=0;
    //发送初始化序列 参照MT48LC4M32B2B5-6A手册Page37 Initialize and Load Mode Register时序图
    SDRAM_Send_Cmd(0,FMC_SDRAM_CMD_CLK_ENABLE,1,0);      //时钟配置使能
    delay_us(300);                                       //至少延时100us
    SDRAM_Send_Cmd(0,FMC_SDRAM_CMD_PALL,1,0);            //对所有存储区预充电
    SDRAM_Send_Cmd(0,FMC_SDRAM_CMD_AUTOREFRESH_MODE,8,0);//设置自刷新次数 
		temp=(u32)SDRAM_MODEREG_BURST_LENGTH_1          |	   //设置突发长度:1(可以是1/2/4/8)
              SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |	   //设置突发类型:连续(可以是连续/交错)
              SDRAM_MODEREG_CAS_LATENCY_3           |	   //设置CAS值:3(可以是2/3)
              SDRAM_MODEREG_OPERATING_MODE_STANDARD |    //设置操作模式:0,标准模式
              SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;      //设置突发写模式:1,单点访问
    SDRAM_Send_Cmd(0,FMC_SDRAM_CMD_LOAD_MODE,1,temp);    //设置SDRAM的模式寄存器
}

/************************************************
名    称: SDRAM_Init
功    能: SDRAM初始化函数
输入参数: 无
输出参数: 无
返回值:   无
************************************************/
void SDRAM_Init(void)
{

    FMC_SDRAM_TimingTypeDef SDRAM_Timing;
                                                     
    SDRAM_Handler.Instance=FMC_SDRAM_DEVICE;                             
    SDRAM_Handler.Init.SDBank=FMC_SDRAM_BANK1;                           //第一个SDRAM BANK
    SDRAM_Handler.Init.ColumnBitsNumber=FMC_SDRAM_COLUMN_BITS_NUM_8;     //列数量8
    SDRAM_Handler.Init.RowBitsNumber=FMC_SDRAM_ROW_BITS_NUM_12;          //行数量12
    SDRAM_Handler.Init.MemoryDataWidth=FMC_SDRAM_MEM_BUS_WIDTH_32;       //数据宽度为32位
    SDRAM_Handler.Init.InternalBankNumber=FMC_SDRAM_INTERN_BANKS_NUM_4;  //一共4个BANK
    SDRAM_Handler.Init.CASLatency=FMC_SDRAM_CAS_LATENCY_3;               //CAS为3
    SDRAM_Handler.Init.WriteProtection=FMC_SDRAM_WRITE_PROTECTION_DISABLE;//失能写保护
    SDRAM_Handler.Init.SDClockPeriod=FMC_SDRAM_CLOCK_PERIOD_2;           //SDRAM时钟为HCLK/2=216M/2=108M=9.26ns
    SDRAM_Handler.Init.ReadBurst=FMC_SDRAM_RBURST_ENABLE;                //使能突发
    SDRAM_Handler.Init.ReadPipeDelay=FMC_SDRAM_RPIPE_DELAY_0;            //读通道延时0个SDRAM时钟
    
	  /* 参照MT48LC4M32B2B5-6A手册Page21 
		 * tMRD = 2Cycle
	   * tXSR >= 67nS = 8Cycle
	   * tRAS >= 42nS = 5Cycle
	   * tRC  >= 60nS = 7Cycle
	   * tWR  >= 1CLK+7nS = 2Cycle
	   * tRP  >= 18nS = 2Cycle
		 * tRCD >= 18nS = 2Cycle
	   */
		
    SDRAM_Timing.LoadToActiveDelay=2;                                   //tMRD 
    SDRAM_Timing.ExitSelfRefreshDelay=8;                                //tXSR 
    SDRAM_Timing.SelfRefreshTime=5;                                     //tRAS                         
    SDRAM_Timing.RowCycleDelay=7;                                       //tRC  
    SDRAM_Timing.WriteRecoveryTime=2;                                   //tWR
    SDRAM_Timing.RPDelay=2;                                             //tRP  
    SDRAM_Timing.RCDDelay=2;                                            //tRCD 
    HAL_SDRAM_Init(&SDRAM_Handler,&SDRAM_Timing);
		
    SDRAM_Initialization_Sequence(&SDRAM_Handler);                      //发送SDRAM初始化序列
		
		
		/*COUNT = SDRAM刷新周期/行数-20
		 *MT48LC4M32B2B5-6A刷新周期为64ms,SDCLK=216/2=108Mhz,行数为4096.
		 *COUNT = 64*1000*108/4096-20=1667
		 */
		HAL_SDRAM_ProgramRefreshRate(&SDRAM_Handler,1667);
}

/************************************************
名    称: HAL_SDRAM_MspInit
功    能: SDRAM外围配置函数
输入参数: hsdram--SDRAM_HandleTypeDef型指针 ,SDRAM操作句柄
输出参数: 无
返回值:   无
************************************************/
void HAL_SDRAM_MspInit(SDRAM_HandleTypeDef *hsdram)
{
    GPIO_InitTypeDef GPIO_Initure;
    
    __HAL_RCC_FMC_CLK_ENABLE();                 //使能FMC时钟
    
    GPIO_Initure.Pin=GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8| GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15;  
    GPIO_Initure.Mode=GPIO_MODE_AF_PP;          //推挽复用
    GPIO_Initure.Pull=GPIO_PULLUP;              //上拉
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;         //高速
    GPIO_Initure.Alternate=GPIO_AF12_FMC;       //复用为FMC    
    HAL_GPIO_Init(GPIOD,&GPIO_Initure);         
    
    GPIO_Initure.Pin=GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 |\
                              GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 |\
                              GPIO_PIN_15;              
    HAL_GPIO_Init(GPIOE,&GPIO_Initure);     

    GPIO_Initure.Pin=GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 |\
                              GPIO_PIN_5 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 |\
                              GPIO_PIN_15;              
    HAL_GPIO_Init(GPIOF,&GPIO_Initure);     
    
    GPIO_Initure.Pin=GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5 |\
															GPIO_PIN_8 | GPIO_PIN_15;              
    HAL_GPIO_Init(GPIOG,&GPIO_Initure);  

		GPIO_Initure.Pin=GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_9 |\
                              GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 |\
                              GPIO_PIN_15;
		HAL_GPIO_Init(GPIOH,&GPIO_Initure); 
		
		GPIO_Initure.Pin=GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 |\
                              GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_9 | GPIO_PIN_10;
		HAL_GPIO_Init(GPIOI,&GPIO_Initure); 
}
