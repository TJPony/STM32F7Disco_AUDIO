#include "ts.h"
#include "timer.h"
#include "IIC.h"

static TS_StatusTypeDef TS_Init(void);
static u8 ft6x06_DetectTouch(void);
static void ft6x06_GetXY(u8 index, u16 *X, u16 *Y);
static void ft6x06_GetTouchInfo(u8 index,u8* pWeight,u8* pArea,u8* pEvent);
static void ft6x06_GetGestureID(u8* pGestureId);
static TS_StatusTypeDef TS_GetAllState(void);
static PT_LCDOptr   	LCD; 

static T_TsOptr g_tTsOptr = {
	.init         = TS_Init,
	.DetectTouch  = ft6x06_DetectTouch,
	.GetXY 				= ft6x06_GetXY,
	.GetTouchInfo = ft6x06_GetTouchInfo,
	.GetGestureID = ft6x06_GetGestureID,
	.GetAllState  = TS_GetAllState,
};

PT_TsOptr Get_TsOptr(void)
{
	PT_TsOptr ptTemp;
	ptTemp = &g_tTsOptr;
	return ptTemp;
}

static void TS_ResetTouchData(PT_TsOptr pt_TsOptr)
{
    u8 index;
		
	  LCD = Get_LCDOptr();
	  pt_TsOptr->TsWidth   = LCD->width;
	  pt_TsOptr->TsHeight  = LCD->height;
	
		pt_TsOptr->SlaveAddr = FT6206_I2C_ADDRESS;
    pt_TsOptr->GestureId = GEST_ID_NO_GESTURE;
    pt_TsOptr->DetNum    = 0;

    for(index = 0; index < TS_MAX_NB_TOUCH; index++)
    {
      pt_TsOptr->Xpos[index]    = 0;
      pt_TsOptr->Ypos[index]    = 0;
      pt_TsOptr->Area[index]    = 0;
      pt_TsOptr->EventId[index] = TOUCH_EVENT_NO_EVT;
      pt_TsOptr->Weight[index]  = 0;
    }
}

static u8 ft6x06_ReadID(void)
{
  return (TS_IO_Read(g_tTsOptr.SlaveAddr, FT6206_CHIP_ID_REG));
}

static void ft6x06_Reset(void)
{
  /* 复位引脚和LCD共用，初始化时复位了LCD同时也就复位了FT6x06 */
}

static void ft6x06_IT_Disable(void)
{
  u8 regValue = 0;
	/* 设为中断查询模式 */
  regValue = (FT6206_G_MODE_INTERRUPT_POLLING & (FT6206_G_MODE_INTERRUPT_MASK >> FT6206_G_MODE_INTERRUPT_SHIFT)) << FT6206_G_MODE_INTERRUPT_SHIFT;

  TS_IO_Write(g_tTsOptr.SlaveAddr, FT6206_GMODE_REG, regValue);
}

static void ft6x06_IT_Enable(void)
{
  u8 regValue = 0;
	/* 设为中断触发模式 */
  regValue = (FT6206_G_MODE_INTERRUPT_TRIGGER & (FT6206_G_MODE_INTERRUPT_MASK >> FT6206_G_MODE_INTERRUPT_SHIFT)) << FT6206_G_MODE_INTERRUPT_SHIFT;
  
  TS_IO_Write(g_tTsOptr.SlaveAddr, FT6206_GMODE_REG, regValue);
}

static void TS_ITConfig(void)
{
  GPIO_InitTypeDef GPIO_Initure;
	GPIO_Initure.Pin   = GPIO_PIN_13;  
	GPIO_Initure.Mode  = GPIO_MODE_IT_FALLING;  	
	GPIO_Initure.Pull  = GPIO_PULLUP;         
	GPIO_Initure.Speed = GPIO_SPEED_FAST;       
	HAL_GPIO_Init(GPIOI,&GPIO_Initure);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 1);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
	
	ft6x06_IT_Enable();
}

static TS_StatusTypeDef TS_Init(void)
{
  TS_StatusTypeDef ts_status = TS_OK;
  
	TS_ResetTouchData(&g_tTsOptr);
	
	IIC_Init();
	
  g_tTsOptr.id = ft6x06_ReadID();
	
  if(g_tTsOptr.id != FT6206_ID_VALUE)
  {
		g_tTsOptr.SlaveAddr  = FT6336G_I2C_ADDRESS;    
    g_tTsOptr.id = ft6x06_ReadID();
  }
  else
  {
    g_tTsOptr.SlaveAddr  = FT6206_I2C_ADDRESS;    
  }
  
  if((g_tTsOptr.id == FT6206_ID_VALUE) || (g_tTsOptr.id == FT6x36_ID_VALUE))
  {
    if(g_tTsOptr.TsWidth < g_tTsOptr.TsHeight)
    {
      g_tTsOptr.Orientation = TS_SWAP_NONE;                
    }
    else
    {
      g_tTsOptr.Orientation = TS_SWAP_XY | TS_SWAP_Y;                 
    }

    ft6x06_Reset();

    TS_ITConfig();
  }
  else
  {
    ts_status = TS_DEVICE_NOT_FOUND;
  }
		
  return ts_status;
}

static u8 ft6x06_DetectTouch(void)
{
  volatile u8 nbTouch = 0;

  nbTouch = TS_IO_Read(g_tTsOptr.SlaveAddr, FT6206_TD_STAT_REG);
  nbTouch &= FT6206_TD_STAT_MASK;

  if(nbTouch > TS_MAX_NB_TOUCH)
  {
    /* If invalid number of touch detected, set it to zero */
    nbTouch = 0;
  }
	
  return nbTouch;
}

static void ft6x06_GetXY(u8 index, u16 *X, u16 *Y)
{
  u8 regAddress = 0;
  u8 dataxy[4];
  
	switch(index)
	{
    case 0:    
      regAddress = FT6206_P1_XH_REG; 
      break;
    case 1:
      regAddress = FT6206_P2_XH_REG; 
      break;
    default:
      break;
  }
    
    TS_IO_ReadMultiple(g_tTsOptr.SlaveAddr, regAddress, dataxy, sizeof(dataxy)); 

    *X = ((dataxy[0] & FT6206_MSB_MASK) << 8) | (dataxy[1] & FT6206_LSB_MASK);
    *Y = ((dataxy[2] & FT6206_MSB_MASK) << 8) | (dataxy[3] & FT6206_LSB_MASK);
}

static void ft6x06_GetTouchInfo(u8 index,u8* pWeight,u8* pArea,u8* pEvent)
{
  u8 regAddress = 0;
  u8 dataxy[3];
  
	switch(index)
	{
		case 0: 
			regAddress = FT6206_P1_WEIGHT_REG;
			break;
		case 1:
			regAddress = FT6206_P2_WEIGHT_REG;
			break;
		default:
			break;
	} 
    
  TS_IO_ReadMultiple(g_tTsOptr.SlaveAddr,regAddress, dataxy, sizeof(dataxy)); 
 
  *pWeight = (dataxy[0] & FT6206_TOUCH_WEIGHT_MASK) >> FT6206_TOUCH_WEIGHT_SHIFT;  
  *pArea   = (dataxy[1] & FT6206_TOUCH_AREA_MASK) >> FT6206_TOUCH_AREA_SHIFT; 
  *pEvent  = (dataxy[2] & FT6206_TOUCH_EVT_FLAG_MASK) >> FT6206_TOUCH_EVT_FLAG_SHIFT; 
}

static void ft6x06_GetGestureID(u8* pGestureId)
{
  volatile u8 ucReadData = 0;

  ucReadData = TS_IO_Read(g_tTsOptr.SlaveAddr, FT6206_GEST_ID_REG);

  *pGestureId = ucReadData;
}

static TS_StatusTypeDef TS_GetAllState(void)
{
  static u32 _x[TS_MAX_NB_TOUCH] = {0, 0};
  static u32 _y[TS_MAX_NB_TOUCH] = {0, 0};
  TS_StatusTypeDef ts_status = TS_OK;
  u16 tmp;
  u16 Raw_x[TS_MAX_NB_TOUCH];
  u16 Raw_y[TS_MAX_NB_TOUCH];
  u16 xDiff;
  u16 yDiff;
  u8 index;
	
  u8 weight = 0;
  u8 area = 0;
  u8 event = 0;
  u8 gestureId = 0;
	
  g_tTsOptr.DetNum = ft6x06_DetectTouch();
	
  if(g_tTsOptr.DetNum)
  {
    for(index = 0; index < g_tTsOptr.DetNum; index++)
    {
      ft6x06_GetXY(index, &(Raw_x[index]), &(Raw_y[index]));

      if(g_tTsOptr.Orientation & TS_SWAP_XY)
      {
        tmp = Raw_x[index];
        Raw_x[index] = Raw_y[index]; 
        Raw_y[index] = tmp;
      }
      
      if(g_tTsOptr.Orientation & TS_SWAP_X)
      {
        Raw_x[index] = g_tTsOptr.TsWidth - 1 - Raw_x[index];
      }

      if(g_tTsOptr.Orientation & TS_SWAP_Y)
      {
        Raw_y[index] = g_tTsOptr.TsHeight - 1 - Raw_y[index];
      }
            
      xDiff = Raw_x[index] > _x[index]? (Raw_x[index] - _x[index]): (_x[index] - Raw_x[index]);
      yDiff = Raw_y[index] > _y[index]? (Raw_y[index] - _y[index]): (_y[index] - Raw_y[index]);

      if ((xDiff + yDiff) > 5)
      {
        _x[index] = Raw_x[index];
        _y[index] = Raw_y[index];
      }


      g_tTsOptr.Xpos[index] = _x[index];
      g_tTsOptr.Ypos[index] = _y[index];

      ft6x06_GetTouchInfo(index, &weight, &area, &event);

      g_tTsOptr.Weight[index] = weight;
      g_tTsOptr.Area[index]   = area;

      switch(event)
      {
        case FT6206_TOUCH_EVT_FLAG_PRESS_DOWN:
          g_tTsOptr.EventId[index] = TOUCH_EVENT_PRESS_DOWN;
          break;
        case FT6206_TOUCH_EVT_FLAG_LIFT_UP:
          g_tTsOptr.EventId[index] = TOUCH_EVENT_LIFT_UP;
          break;
        case FT6206_TOUCH_EVT_FLAG_CONTACT:
          g_tTsOptr.EventId[index] = TOUCH_EVENT_CONTACT;
          break;
        case FT6206_TOUCH_EVT_FLAG_NO_EVENT:
          g_tTsOptr.EventId[index] = TOUCH_EVENT_NO_EVT;
          break;
        default:
          ts_status = TS_ERROR;
          break;
      } 
    } 
		
    ft6x06_GetGestureID(&gestureId);

  
		switch(gestureId)
		{
			case FT6206_GEST_ID_NO_GESTURE :
				g_tTsOptr.GestureId = GEST_ID_NO_GESTURE;
				break;
			case FT6206_GEST_ID_MOVE_UP :
				g_tTsOptr.GestureId = GEST_ID_MOVE_UP;
				break;
			case FT6206_GEST_ID_MOVE_RIGHT :
				g_tTsOptr.GestureId = GEST_ID_MOVE_RIGHT;
				break;
			case FT6206_GEST_ID_MOVE_DOWN :
				g_tTsOptr.GestureId = GEST_ID_MOVE_DOWN;
				break;
			case FT6206_GEST_ID_MOVE_LEFT :
				g_tTsOptr.GestureId = GEST_ID_MOVE_LEFT;
				break;
			case FT6206_GEST_ID_ZOOM_IN :
				g_tTsOptr.GestureId = GEST_ID_ZOOM_IN;
				break;
			case FT6206_GEST_ID_ZOOM_OUT :
				g_tTsOptr.GestureId = GEST_ID_ZOOM_OUT;
				break;
			default :
				ts_status = TS_ERROR;
				break;
		}
  } 

  return ts_status;
}

void EXTI15_10_IRQHandler(void)
{
  if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_13) != RESET)
  {
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_13);
		TS_GetAllState();
  }
}
