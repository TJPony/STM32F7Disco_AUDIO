#include "wm8994.h"
#include "IIC.h"

static uint32_t outputEnabled = 0;
static uint32_t inputEnabled = 0;
static uint8_t ColdStartup = 1;

static uint32_t wm8994_Init(uint16_t OutputInputDevice, uint8_t Volume, uint32_t AudioFreq);
static void wm8994_Play(void);
static void wm8994_Pause(void);
static void wm8994_Resume(void);
static void wm8994_Stop(uint32_t Cmd);
static void wm8994_SetFmt(u8 fmt,u8 len);
static void wm8994_SetVolume(uint8_t Volume);
static void wm8994_SetFrequency(uint32_t AudioFreq);

static T_CodecOptr g_tCodecOptr = {
	.readreg   = CODEC_IO_Read,
	.writereg  = CODEC_IO_Write,
	.init      = wm8994_Init,
	.play      = wm8994_Play,
	.pause     = wm8994_Pause,
	.resume    = wm8994_Resume,
	.stop      = wm8994_Stop,
	.setfmt    = wm8994_SetFmt,
	.setvolume = wm8994_SetVolume,
	.setfreq   = wm8994_SetFrequency,
};


/**
  * @brief  Get the WM8994 ID.
  * @param DeviceAddr: Device address on communication Bus.
  * @retval The WM8994 ID 
  */
static uint32_t wm8994_ReadID(void)
{
  return ((uint32_t)CODEC_IO_Read(WM8994_ADDR, WM8994_CHIPID_ADDR));
}

/**
  * @brief Resets wm8994 registers.
  * @param DeviceAddr: Device address on communication Bus. 
  * @retval 0 if correct communication, else wrong communication
  */
static void wm8994_Reset(void)
{
  /* Reset Codec by writing in 0x0000 address register */
  CODEC_IO_Write(WM8994_ADDR, 0x0000, 0x0000);
  outputEnabled = 0;
  inputEnabled=0;
}

/**
  * @brief Initializes the audio codec and the control interface.
  * @param DeviceAddr: Device address on communication Bus.   
  * @param OutputInputDevice: can be OUTPUT_DEVICE_SPEAKER, OUTPUT_DEVICE_HEADPHONE,
  *  OUTPUT_DEVICE_BOTH, OUTPUT_DEVICE_AUTO, INPUT_DEVICE_DIGITAL_MICROPHONE_1,
  *  INPUT_DEVICE_DIGITAL_MICROPHONE_2, INPUT_DEVICE_DIGITAL_MIC1_MIC2, 
  *  INPUT_DEVICE_INPUT_LINE_1 or INPUT_DEVICE_INPUT_LINE_2.
  * @param Volume: Initial volume level (from 0 (Mute) to 100 (Max))
  * @param AudioFreq: Audio Frequency 
  * @retval 0 if correct communication, else wrong communication
  */
static uint32_t wm8994_Init(uint16_t OutputInputDevice, uint8_t Volume, uint32_t AudioFreq)
{
  uint16_t output_device = OutputInputDevice & 0xFF;
  uint16_t input_device = OutputInputDevice & 0xFF00;
  uint16_t power_mgnt_reg_1 = 0;
  
	/* Initialize the Control interface of the Audio Codec */
  IIC_Init();

	wm8994_Reset();
	
	g_tCodecOptr.id = wm8994_ReadID();
	
  /* wm8994 Errata Work-Arounds */
  CODEC_IO_Write(WM8994_ADDR, 0x102, 0x0003);
  CODEC_IO_Write(WM8994_ADDR, 0x817, 0x0000);
  CODEC_IO_Write(WM8994_ADDR, 0x102, 0x0000);

  /* Enable VMID soft start (fast), Start-up Bias Current Enabled */
  CODEC_IO_Write(WM8994_ADDR, 0x39, 0x006C);
  
    /* Enable bias generator, Enable VMID */
  if (input_device > 0)
  {
    CODEC_IO_Write(WM8994_ADDR, 0x01, 0x0013);
  }
  else
  {
    CODEC_IO_Write(WM8994_ADDR, 0x01, 0x0003);
  }

  delay_ms(50);

  /* Path Configurations for output */
  if (output_device > 0)
  {
    outputEnabled = 1;

    switch (output_device)
    {
    case OUTPUT_DEVICE_SPEAKER:
      /* Enable DAC1 (Left), Enable DAC1 (Right),
      Disable DAC2 (Left), Disable DAC2 (Right)*/
      CODEC_IO_Write(WM8994_ADDR, 0x05, 0x0C0C);

      /* Enable the AIF1 Timeslot 0 (Left) to DAC 1 (Left) mixer path */
      CODEC_IO_Write(WM8994_ADDR, 0x601, 0x0000);

      /* Enable the AIF1 Timeslot 0 (Right) to DAC 1 (Right) mixer path */
      CODEC_IO_Write(WM8994_ADDR, 0x602, 0x0000);

      /* Disable the AIF1 Timeslot 1 (Left) to DAC 2 (Left) mixer path */
      CODEC_IO_Write(WM8994_ADDR, 0x604, 0x0002);

      /* Disable the AIF1 Timeslot 1 (Right) to DAC 2 (Right) mixer path */
      CODEC_IO_Write(WM8994_ADDR, 0x605, 0x0002);
      break;

    case OUTPUT_DEVICE_HEADPHONE:
      /* Disable DAC1 (Left), Disable DAC1 (Right),
      Enable DAC2 (Left), Enable DAC2 (Right)*/
      CODEC_IO_Write(WM8994_ADDR, 0x05, 0x0303);

      /* Enable the AIF1 Timeslot 0 (Left) to DAC 1 (Left) mixer path */
      CODEC_IO_Write(WM8994_ADDR, 0x601, 0x0001);

      /* Enable the AIF1 Timeslot 0 (Right) to DAC 1 (Right) mixer path */
      CODEC_IO_Write(WM8994_ADDR, 0x602, 0x0001);

      /* Disable the AIF1 Timeslot 1 (Left) to DAC 2 (Left) mixer path */
      CODEC_IO_Write(WM8994_ADDR, 0x604, 0x0000);

      /* Disable the AIF1 Timeslot 1 (Right) to DAC 2 (Right) mixer path */
      CODEC_IO_Write(WM8994_ADDR, 0x605, 0x0000);
      break;

    case OUTPUT_DEVICE_BOTH:
      if (input_device == INPUT_DEVICE_DIGITAL_MIC1_MIC2)
      {
        /* Enable DAC1 (Left), Enable DAC1 (Right),
        also Enable DAC2 (Left), Enable DAC2 (Right)*/
        CODEC_IO_Write(WM8994_ADDR, 0x05, 0x0303 | 0x0C0C);
        
        /* Enable the AIF1 Timeslot 0 (Left) to DAC 1 (Left) mixer path
        Enable the AIF1 Timeslot 1 (Left) to DAC 1 (Left) mixer path */
        CODEC_IO_Write(WM8994_ADDR, 0x601, 0x0003);
        
        /* Enable the AIF1 Timeslot 0 (Right) to DAC 1 (Right) mixer path
        Enable the AIF1 Timeslot 1 (Right) to DAC 1 (Right) mixer path */
        CODEC_IO_Write(WM8994_ADDR, 0x602, 0x0003);
        
        /* Enable the AIF1 Timeslot 0 (Left) to DAC 2 (Left) mixer path
        Enable the AIF1 Timeslot 1 (Left) to DAC 2 (Left) mixer path  */
        CODEC_IO_Write(WM8994_ADDR, 0x604, 0x0003);
        
        /* Enable the AIF1 Timeslot 0 (Right) to DAC 2 (Right) mixer path
        Enable the AIF1 Timeslot 1 (Right) to DAC 2 (Right) mixer path */
        CODEC_IO_Write(WM8994_ADDR, 0x605, 0x0003);
      }
      else
      {
        /* Enable DAC1 (Left), Enable DAC1 (Right),
        also Enable DAC2 (Left), Enable DAC2 (Right)*/
        CODEC_IO_Write(WM8994_ADDR, 0x05, 0x0303 | 0x0C0C);
        
        /* Enable the AIF1 Timeslot 0 (Left) to DAC 1 (Left) mixer path */
        CODEC_IO_Write(WM8994_ADDR, 0x601, 0x0001);
        
        /* Enable the AIF1 Timeslot 0 (Right) to DAC 1 (Right) mixer path */
        CODEC_IO_Write(WM8994_ADDR, 0x602, 0x0001);
        
        /* Enable the AIF1 Timeslot 1 (Left) to DAC 2 (Left) mixer path */
        CODEC_IO_Write(WM8994_ADDR, 0x604, 0x0002);
        
        /* Enable the AIF1 Timeslot 1 (Right) to DAC 2 (Right) mixer path */
        CODEC_IO_Write(WM8994_ADDR, 0x605, 0x0002);      
      }
      break;

    case OUTPUT_DEVICE_AUTO :
    default:
      /* Disable DAC1 (Left), Disable DAC1 (Right),
      Enable DAC2 (Left), Enable DAC2 (Right)*/
      CODEC_IO_Write(WM8994_ADDR, 0x05, 0x0303);

      /* Enable the AIF1 Timeslot 0 (Left) to DAC 1 (Left) mixer path */
      CODEC_IO_Write(WM8994_ADDR, 0x601, 0x0001);

      /* Enable the AIF1 Timeslot 0 (Right) to DAC 1 (Right) mixer path */
      CODEC_IO_Write(WM8994_ADDR, 0x602, 0x0001);

      /* Disable the AIF1 Timeslot 1 (Left) to DAC 2 (Left) mixer path */
      CODEC_IO_Write(WM8994_ADDR, 0x604, 0x0000);

      /* Disable the AIF1 Timeslot 1 (Right) to DAC 2 (Right) mixer path */
      CODEC_IO_Write(WM8994_ADDR, 0x605, 0x0000);
      break;
    }
  }
  else
  {
    outputEnabled = 0;
  }

  /* Path Configurations for input */
  if (input_device > 0)
  {
    inputEnabled = 1;
    switch (input_device)
    {
    case INPUT_DEVICE_DIGITAL_MICROPHONE_2 :
      /* Enable AIF1ADC2 (Left), Enable AIF1ADC2 (Right)
       * Enable DMICDAT2 (Left), Enable DMICDAT2 (Right)
       * Enable Left ADC, Enable Right ADC */
      CODEC_IO_Write(WM8994_ADDR, 0x04, 0x0C30);

      /* Enable AIF1 DRC2 Signal Detect & DRC in AIF1ADC2 Left/Right Timeslot 1 */
      CODEC_IO_Write(WM8994_ADDR, 0x450, 0x00DB);

      /* Disable IN1L, IN1R, IN2L, IN2R, Enable Thermal sensor & shutdown */
      CODEC_IO_Write(WM8994_ADDR, 0x02, 0x6000);

      /* Enable the DMIC2(Left) to AIF1 Timeslot 1 (Left) mixer path */
      CODEC_IO_Write(WM8994_ADDR, 0x608, 0x0002);

      /* Enable the DMIC2(Right) to AIF1 Timeslot 1 (Right) mixer path */
      CODEC_IO_Write(WM8994_ADDR, 0x609, 0x0002);

      /* GPIO1 pin configuration GP1_DIR = output, GP1_FN = AIF1 DRC2 signal detect */
      CODEC_IO_Write(WM8994_ADDR, 0x700, 0x000E);
      break;

    case INPUT_DEVICE_INPUT_LINE_1 :
      /* IN1LN_TO_IN1L, IN1LP_TO_VMID, IN1RN_TO_IN1R, IN1RP_TO_VMID */
      CODEC_IO_Write(WM8994_ADDR, 0x28, 0x0011);

      /* Disable mute on IN1L_TO_MIXINL and +30dB on IN1L PGA output */
      CODEC_IO_Write(WM8994_ADDR, 0x29, 0x0035);

      /* Disable mute on IN1R_TO_MIXINL, Gain = +30dB */
      CODEC_IO_Write(WM8994_ADDR, 0x2A, 0x0035);

      /* Enable AIF1ADC1 (Left), Enable AIF1ADC1 (Right)
       * Enable Left ADC, Enable Right ADC */
      CODEC_IO_Write(WM8994_ADDR, 0x04, 0x0303);

      /* Enable AIF1 DRC1 Signal Detect & DRC in AIF1ADC1 Left/Right Timeslot 0 */
      CODEC_IO_Write(WM8994_ADDR, 0x440, 0x00DB);

      /* Enable IN1L and IN1R, Disable IN2L and IN2R, Enable Thermal sensor & shutdown */
      CODEC_IO_Write(WM8994_ADDR, 0x02, 0x6350);

      /* Enable the ADCL(Left) to AIF1 Timeslot 0 (Left) mixer path */
      CODEC_IO_Write(WM8994_ADDR, 0x606, 0x0002);

      /* Enable the ADCR(Right) to AIF1 Timeslot 0 (Right) mixer path */
      CODEC_IO_Write(WM8994_ADDR, 0x607, 0x0002);

      /* GPIO1 pin configuration GP1_DIR = output, GP1_FN = AIF1 DRC1 signal detect */
      CODEC_IO_Write(WM8994_ADDR, 0x700, 0x000D);
      break;

    case INPUT_DEVICE_DIGITAL_MICROPHONE_1 :
      /* Enable AIF1ADC1 (Left), Enable AIF1ADC1 (Right)
       * Enable DMICDAT1 (Left), Enable DMICDAT1 (Right)
       * Enable Left ADC, Enable Right ADC */
      CODEC_IO_Write(WM8994_ADDR, 0x04, 0x030C);

      /* Enable AIF1 DRC2 Signal Detect & DRC in AIF1ADC1 Left/Right Timeslot 0 */
      CODEC_IO_Write(WM8994_ADDR, 0x440, 0x00DB);

      /* Disable IN1L, IN1R, IN2L, IN2R, Enable Thermal sensor & shutdown */
      CODEC_IO_Write(WM8994_ADDR, 0x02, 0x6350);

      /* Enable the DMIC2(Left) to AIF1 Timeslot 0 (Left) mixer path */
      CODEC_IO_Write(WM8994_ADDR, 0x606, 0x0002);

      /* Enable the DMIC2(Right) to AIF1 Timeslot 0 (Right) mixer path */
      CODEC_IO_Write(WM8994_ADDR, 0x607, 0x0002);

      /* GPIO1 pin configuration GP1_DIR = output, GP1_FN = AIF1 DRC1 signal detect */
      CODEC_IO_Write(WM8994_ADDR, 0x700, 0x000D);
      break; 
    case INPUT_DEVICE_DIGITAL_MIC1_MIC2 :
      /* Enable AIF1ADC1 (Left), Enable AIF1ADC1 (Right)
       * Enable DMICDAT1 (Left), Enable DMICDAT1 (Right)
       * Enable Left ADC, Enable Right ADC */
      CODEC_IO_Write(WM8994_ADDR, 0x04, 0x0F3C);

      /* Enable AIF1 DRC2 Signal Detect & DRC in AIF1ADC2 Left/Right Timeslot 1 */
      CODEC_IO_Write(WM8994_ADDR, 0x450, 0x00DB);
      
      /* Enable AIF1 DRC2 Signal Detect & DRC in AIF1ADC1 Left/Right Timeslot 0 */
      CODEC_IO_Write(WM8994_ADDR, 0x440, 0x00DB);

      /* Disable IN1L, IN1R, Enable IN2L, IN2R, Thermal sensor & shutdown */
      CODEC_IO_Write(WM8994_ADDR, 0x02, 0x63A0);

      /* Enable the DMIC2(Left) to AIF1 Timeslot 0 (Left) mixer path */
      CODEC_IO_Write(WM8994_ADDR, 0x606, 0x0002);

      /* Enable the DMIC2(Right) to AIF1 Timeslot 0 (Right) mixer path */
      CODEC_IO_Write(WM8994_ADDR, 0x607, 0x0002);

      /* Enable the DMIC2(Left) to AIF1 Timeslot 1 (Left) mixer path */
      CODEC_IO_Write(WM8994_ADDR, 0x608, 0x0002);

      /* Enable the DMIC2(Right) to AIF1 Timeslot 1 (Right) mixer path */
      CODEC_IO_Write(WM8994_ADDR, 0x609, 0x0002);
      
      /* GPIO1 pin configuration GP1_DIR = output, GP1_FN = AIF1 DRC1 signal detect */
      CODEC_IO_Write(WM8994_ADDR, 0x700, 0x000D);
      break;    
    case INPUT_DEVICE_INPUT_LINE_2 :
    default:
      /* Actually, no other input devices supported */
      break;
    }
  }
  else
  {
    inputEnabled = 0;
  }
  
  /*  Clock Configurations */
  wm8994_SetFrequency(AudioFreq);

  if(input_device == INPUT_DEVICE_DIGITAL_MIC1_MIC2)
  {
  /* AIF1 Word Length = 16-bits, AIF1 Format = DSP mode */
		CODEC_IO_Write(WM8994_ADDR, 0x300, 0x4018);    
  }
  else
  {
  /* AIF1 Word Length = 16-bits, AIF1 Format = I2S (Default Register Value) */
		CODEC_IO_Write(WM8994_ADDR, 0x300, 0x4010);
  }
  
  /* slave mode */
  CODEC_IO_Write(WM8994_ADDR, 0x302, 0x0000);
  
  /* Enable the DSP processing clock for AIF1, Enable the core clock */
  CODEC_IO_Write(WM8994_ADDR, 0x208, 0x000A);
  
  /* Enable AIF1 Clock, AIF1 Clock Source = MCLK1 pin */
  CODEC_IO_Write(WM8994_ADDR, 0x200, 0x0001);

  if (output_device > 0)  /* Audio output selected */
  {
    if (output_device == OUTPUT_DEVICE_HEADPHONE)
    {      
      /* Select DAC1 (Left) to Left Headphone Output PGA (HPOUT1LVOL) path */
      CODEC_IO_Write(WM8994_ADDR, 0x2D, 0x0100);
      
      /* Select DAC1 (Right) to Right Headphone Output PGA (HPOUT1RVOL) path */
      CODEC_IO_Write(WM8994_ADDR, 0x2E, 0x0100);    
            
      /* Startup sequence for Headphone */
      if(ColdStartup)
      {
        CODEC_IO_Write(WM8994_ADDR,0x110,0x8100);
        
        ColdStartup=0;
        /* Add Delay */
        delay_ms(300);
      }
      else /* Headphone Warm Start-Up */
      { 
        CODEC_IO_Write(WM8994_ADDR,0x110,0x8108);
        /* Add Delay */
        delay_ms(50);
      }

      /* Soft un-Mute the AIF1 Timeslot 0 DAC1 path L&R */
      CODEC_IO_Write(WM8994_ADDR, 0x420, 0x0000);
    }
    /* Analog Output Configuration */

    /* Enable SPKRVOL PGA, Enable SPKMIXR, Enable SPKLVOL PGA, Enable SPKMIXL */
    CODEC_IO_Write(WM8994_ADDR, 0x03, 0x0300);

    /* Left Speaker Mixer Volume = 0dB */
    CODEC_IO_Write(WM8994_ADDR, 0x22, 0x0000);

    /* Speaker output mode = Class D, Right Speaker Mixer Volume = 0dB ((0x23, 0x0100) = class AB)*/
    CODEC_IO_Write(WM8994_ADDR, 0x23, 0x0000);

    /* Unmute DAC2 (Left) to Left Speaker Mixer (SPKMIXL) path,
    Unmute DAC2 (Right) to Right Speaker Mixer (SPKMIXR) path */
    CODEC_IO_Write(WM8994_ADDR, 0x36, 0x0300);

    /* Enable bias generator, Enable VMID, Enable SPKOUTL, Enable SPKOUTR */
    CODEC_IO_Write(WM8994_ADDR, 0x01, 0x3003);

    /* Headphone/Speaker Enable */

    if (input_device == INPUT_DEVICE_DIGITAL_MIC1_MIC2)
    {
    /* Enable Class W, Class W Envelope Tracking = AIF1 Timeslots 0 and 1 */
    CODEC_IO_Write(WM8994_ADDR, 0x51, 0x0205);
    }
    else
    {
    /* Enable Class W, Class W Envelope Tracking = AIF1 Timeslot 0 */
    CODEC_IO_Write(WM8994_ADDR, 0x51, 0x0005);      
    }

    /* Enable bias generator, Enable VMID, Enable HPOUT1 (Left) and Enable HPOUT1 (Right) input stages */
    /* idem for Speaker */
    power_mgnt_reg_1 |= 0x0303 | 0x3003;
    CODEC_IO_Write(WM8994_ADDR, 0x01, power_mgnt_reg_1);

    /* Enable HPOUT1 (Left) and HPOUT1 (Right) intermediate stages */
    CODEC_IO_Write(WM8994_ADDR, 0x60, 0x0022);

    /* Enable Charge Pump */
    CODEC_IO_Write(WM8994_ADDR, 0x4C, 0x9F25);

    /* Add Delay */
    delay_ms(15);

    /* Select DAC1 (Left) to Left Headphone Output PGA (HPOUT1LVOL) path */
    CODEC_IO_Write(WM8994_ADDR, 0x2D, 0x0001);

    /* Select DAC1 (Right) to Right Headphone Output PGA (HPOUT1RVOL) path */
    CODEC_IO_Write(WM8994_ADDR, 0x2E, 0x0001);

    /* Enable Left Output Mixer (MIXOUTL), Enable Right Output Mixer (MIXOUTR) */
    /* idem for SPKOUTL and SPKOUTR */
    CODEC_IO_Write(WM8994_ADDR, 0x03, 0x0030 | 0x0300);

    /* Enable DC Servo and trigger start-up mode on left and right channels */
    CODEC_IO_Write(WM8994_ADDR, 0x54, 0x0033);

    /* Add Delay */
    delay_ms(257);
    /* Enable HPOUT1 (Left) and HPOUT1 (Right) intermediate and output stages. Remove clamps */
    CODEC_IO_Write(WM8994_ADDR, 0x60, 0x00EE);

    /* Unmutes */

    /* Unmute DAC 1 (Left) */
    CODEC_IO_Write(WM8994_ADDR, 0x610, 0x00C0);

    /* Unmute DAC 1 (Right) */
    CODEC_IO_Write(WM8994_ADDR, 0x611, 0x00C0);

    /* Unmute the AIF1 Timeslot 0 DAC path */
    CODEC_IO_Write(WM8994_ADDR, 0x420, 0x0010);

    /* Unmute DAC 2 (Left) */
    CODEC_IO_Write(WM8994_ADDR, 0x612, 0x00C0);

    /* Unmute DAC 2 (Right) */
    CODEC_IO_Write(WM8994_ADDR, 0x613, 0x00C0);

    /* Unmute the AIF1 Timeslot 1 DAC2 path */
    CODEC_IO_Write(WM8994_ADDR, 0x422, 0x0010);
    
    /* Volume Control */
    wm8994_SetVolume(Volume);
  }

  if (input_device > 0) /* Audio input selected */
  {
    if ((input_device == INPUT_DEVICE_DIGITAL_MICROPHONE_1) || (input_device == INPUT_DEVICE_DIGITAL_MICROPHONE_2))
    {
      /* Enable Microphone bias 1 generator, Enable VMID */
      power_mgnt_reg_1 |= 0x0013;
      CODEC_IO_Write(WM8994_ADDR, 0x01, power_mgnt_reg_1);

      /* ADC oversample enable */
      CODEC_IO_Write(WM8994_ADDR, 0x620, 0x0002);

      /* AIF ADC2 HPF enable, HPF cut = voice mode 1 fc=127Hz at fs=8kHz */
      CODEC_IO_Write(WM8994_ADDR, 0x411, 0x3800);
    }
    else if(input_device == INPUT_DEVICE_DIGITAL_MIC1_MIC2)
    {
      /* Enable Microphone bias 1 generator, Enable VMID */
      power_mgnt_reg_1 |= 0x0013;
      CODEC_IO_Write(WM8994_ADDR, 0x01, power_mgnt_reg_1);

      /* ADC oversample enable */
      CODEC_IO_Write(WM8994_ADDR, 0x620, 0x0002);
    
      /* AIF ADC1 HPF enable, HPF cut = voice mode 1 fc=127Hz at fs=8kHz */
      CODEC_IO_Write(WM8994_ADDR, 0x410, 0x1800);
      
      /* AIF ADC2 HPF enable, HPF cut = voice mode 1 fc=127Hz at fs=8kHz */
      CODEC_IO_Write(WM8994_ADDR, 0x411, 0x1800);      
    }    
    else if ((input_device == INPUT_DEVICE_INPUT_LINE_1) || (input_device == INPUT_DEVICE_INPUT_LINE_2))
    {

      /* Disable mute on IN1L, IN1L Volume = +0dB */
      CODEC_IO_Write(WM8994_ADDR, 0x18, 0x000B);

      /* Disable mute on IN1R, IN1R Volume = +0dB */
      CODEC_IO_Write(WM8994_ADDR, 0x1A, 0x000B);

      /* AIF ADC1 HPF enable, HPF cut = hifi mode fc=4Hz at fs=48kHz */
      CODEC_IO_Write(WM8994_ADDR, 0x410, 0x1800);
    }
    /* Volume Control */
    wm8994_SetVolume(Volume);
  }
	return 0;
}

/**
  * @brief Enables or disables the mute feature on the audio codec.
  * @param DeviceAddr: Device address on communication Bus.   
  * @param Cmd: AUDIO_MUTE_ON to enable the mute or AUDIO_MUTE_OFF to disable the
  *             mute mode.
  * @retval 0 if correct communication, else wrong communication
  */
static uint32_t wm8994_SetMute(uint32_t Cmd)
{
  uint32_t counter = 0;
  
  if (outputEnabled != 0)
  {
    /* Set the Mute mode */
    if(Cmd == AUDIO_MUTE_ON)
    {
      /* Soft Mute the AIF1 Timeslot 0 DAC1 path L&R */
      CODEC_IO_Write(WM8994_ADDR, 0x420, 0x0200);

      /* Soft Mute the AIF1 Timeslot 1 DAC2 path L&R */
      CODEC_IO_Write(WM8994_ADDR, 0x422, 0x0200);
    }
    else /* AUDIO_MUTE_OFF Disable the Mute */
    {
      /* Unmute the AIF1 Timeslot 0 DAC1 path L&R */
      CODEC_IO_Write(WM8994_ADDR, 0x420, 0x0010);
      /* Unmute the AIF1 Timeslot 1 DAC2 path L&R */
      CODEC_IO_Write(WM8994_ADDR, 0x422, 0x0010);
    }
  }
  return counter;
}

static void wm8994_SetFmt(u8 fmt,u8 len)
{
	u8 temp = 0;
	
	fmt &= 0X03;
	len &= 0X03;//�޶���Χ
	
	temp = (len<<5)|(fmt<<3);
	
	CODEC_IO_Write(WM8994_ADDR, 0x300, 0x4000|temp);
}	

/**
  * @brief Start the audio Codec play feature.
  * @note For this codec no Play options are required.
  * @param DeviceAddr: Device address on communication Bus.   
  * @retval 0 if correct communication, else wrong communication
  */
static void wm8994_Play(void)
{
  /* Resumes the audio file playing */  
  /* Unmute the output first */
  wm8994_SetMute(AUDIO_MUTE_OFF);
}

/**
  * @brief Pauses playing on the audio codec.
  * @param DeviceAddr: Device address on communication Bus. 
  * @retval 0 if correct communication, else wrong communication
  */
static void wm8994_Pause(void)
{  
  /* Pause the audio file playing */
  /* Mute the output first */
  wm8994_SetMute(AUDIO_MUTE_ON);
  
  /* Put the Codec in Power save mode */
  CODEC_IO_Write(WM8994_ADDR, 0x02, 0x01);
}

/**
  * @brief Resumes playing on the audio codec.
  * @param DeviceAddr: Device address on communication Bus. 
  * @retval 0 if correct communication, else wrong communication
  */
static void wm8994_Resume(void)
{
  /* Resumes the audio file playing */  
  /* Unmute the output first */
  wm8994_SetMute(AUDIO_MUTE_OFF);
}

/**
  * @brief Stops audio Codec playing. It powers down the codec.
  * @param DeviceAddr: Device address on communication Bus. 
  * @param CodecPdwnMode: selects the  power down mode.
  *          - CODEC_PDWN_SW: only mutes the audio codec. When resuming from this 
  *                           mode the codec keeps the previous initialization
  *                           (no need to re-Initialize the codec registers).
  *          - CODEC_PDWN_HW: Physically power down the codec. When resuming from this
  *                           mode, the codec is set to default configuration 
  *                           (user should re-Initialize the codec in order to 
  *                            play again the audio stream).
  * @retval 0 if correct communication, else wrong communication
  */
static void wm8994_Stop(uint32_t CodecPdwnMode)
{
  if (outputEnabled != 0)
  {
    /* Mute the output first */
    wm8994_SetMute(AUDIO_MUTE_ON);

    if (CodecPdwnMode == CODEC_PDWN_SW)
    {
      /* Only output mute required*/
    }
    else /* CODEC_PDWN_HW */
    {
      /* Mute the AIF1 Timeslot 0 DAC1 path */
      CODEC_IO_Write(WM8994_ADDR, 0x420, 0x0200);

      /* Mute the AIF1 Timeslot 1 DAC2 path */
      CODEC_IO_Write(WM8994_ADDR, 0x422, 0x0200);

      /* Disable DAC1L_TO_HPOUT1L */
      CODEC_IO_Write(WM8994_ADDR, 0x2D, 0x0000);

      /* Disable DAC1R_TO_HPOUT1R */
      CODEC_IO_Write(WM8994_ADDR, 0x2E, 0x0000);

      /* Disable DAC1 and DAC2 */
      CODEC_IO_Write(WM8994_ADDR, 0x05, 0x0000);

      /* Reset Codec by writing in 0x0000 address register */
      CODEC_IO_Write(WM8994_ADDR, 0x0000, 0x0000);

      outputEnabled = 0;
    }
  }
}

/**
  * @brief Sets higher or lower the codec volume level.
  * @param DeviceAddr: Device address on communication Bus.
  * @param Volume: a byte value from 0 to 255 (refer to codec registers 
  *         description for more details).
  * @retval 0 if correct communication, else wrong communication
  */
static void wm8994_SetVolume(uint8_t Volume)
{
  uint8_t convertedvol = VOLUME_CONVERT(Volume);

  /* Output volume */
  if (outputEnabled != 0)
  {
    if(convertedvol > 0x3E)
    {
      /* Unmute audio codec */
      wm8994_SetMute(AUDIO_MUTE_OFF);

      /* Left Headphone Volume */
      CODEC_IO_Write(WM8994_ADDR, 0x1C, 0x3F | 0x140);

      /* Right Headphone Volume */
      CODEC_IO_Write(WM8994_ADDR, 0x1D, 0x3F | 0x140);

      /* Left Speaker Volume */
      CODEC_IO_Write(WM8994_ADDR, 0x26, 0x3F | 0x140);

      /* Right Speaker Volume */
      CODEC_IO_Write(WM8994_ADDR, 0x27, 0x3F | 0x140);
    }
    else if (Volume == 0)
    {
      /* Mute audio codec */
      wm8994_SetMute(AUDIO_MUTE_ON);
    }
    else
    {
      /* Unmute audio codec */
      wm8994_SetMute(AUDIO_MUTE_OFF);

      /* Left Headphone Volume */
      CODEC_IO_Write(WM8994_ADDR, 0x1C, convertedvol | 0x140);

      /* Right Headphone Volume */
      CODEC_IO_Write(WM8994_ADDR, 0x1D, convertedvol | 0x140);

      /* Left Speaker Volume */
      CODEC_IO_Write(WM8994_ADDR, 0x26, convertedvol | 0x140);

      /* Right Speaker Volume */
      CODEC_IO_Write(WM8994_ADDR, 0x27, convertedvol | 0x140);
    }
  }

  /* Input volume */
  if (inputEnabled != 0)
  {
    convertedvol = VOLUME_IN_CONVERT(Volume);

    /* Left AIF1 ADC1 volume */
    CODEC_IO_Write(WM8994_ADDR, 0x400, convertedvol | 0x100);

    /* Right AIF1 ADC1 volume */
    CODEC_IO_Write(WM8994_ADDR, 0x401, convertedvol | 0x100);

    /* Left AIF1 ADC2 volume */
    CODEC_IO_Write(WM8994_ADDR, 0x404, convertedvol | 0x100);

    /* Right AIF1 ADC2 volume */
    CODEC_IO_Write(WM8994_ADDR, 0x405, convertedvol | 0x100);
  }
}

/**
  * @brief Switch dynamically (while audio file is played) the output target 
  *         (speaker or headphone).
  * @param DeviceAddr: Device address on communication Bus.
  * @param Output: specifies the audio output target: OUTPUT_DEVICE_SPEAKER,
  *         OUTPUT_DEVICE_HEADPHONE, OUTPUT_DEVICE_BOTH or OUTPUT_DEVICE_AUTO 
  * @retval 0 if correct communication, else wrong communication
  */
static void wm8994_SetOutputMode(uint8_t Output)
{
  switch (Output) 
  {
  case OUTPUT_DEVICE_SPEAKER:
    /* Enable DAC1 (Left), Enable DAC1 (Right), 
    Disable DAC2 (Left), Disable DAC2 (Right)*/
    CODEC_IO_Write(WM8994_ADDR, 0x05, 0x0C0C);
    
    /* Enable the AIF1 Timeslot 0 (Left) to DAC 1 (Left) mixer path */
    CODEC_IO_Write(WM8994_ADDR, 0x601, 0x0000);
    
    /* Enable the AIF1 Timeslot 0 (Right) to DAC 1 (Right) mixer path */
    CODEC_IO_Write(WM8994_ADDR, 0x602, 0x0000);
    
    /* Disable the AIF1 Timeslot 1 (Left) to DAC 2 (Left) mixer path */
    CODEC_IO_Write(WM8994_ADDR, 0x604, 0x0002);
    
    /* Disable the AIF1 Timeslot 1 (Right) to DAC 2 (Right) mixer path */
    CODEC_IO_Write(WM8994_ADDR, 0x605, 0x0002);
    break;
    
  case OUTPUT_DEVICE_HEADPHONE:
    /* Disable DAC1 (Left), Disable DAC1 (Right), 
    Enable DAC2 (Left), Enable DAC2 (Right)*/
    CODEC_IO_Write(WM8994_ADDR, 0x05, 0x0303);
    
    /* Enable the AIF1 Timeslot 0 (Left) to DAC 1 (Left) mixer path */
    CODEC_IO_Write(WM8994_ADDR, 0x601, 0x0001);
    
    /* Enable the AIF1 Timeslot 0 (Right) to DAC 1 (Right) mixer path */
    CODEC_IO_Write(WM8994_ADDR, 0x602, 0x0001);
    
    /* Disable the AIF1 Timeslot 1 (Left) to DAC 2 (Left) mixer path */
    CODEC_IO_Write(WM8994_ADDR, 0x604, 0x0000);
    
    /* Disable the AIF1 Timeslot 1 (Right) to DAC 2 (Right) mixer path */
    CODEC_IO_Write(WM8994_ADDR, 0x605, 0x0000);
    break;
    
  case OUTPUT_DEVICE_BOTH:
    /* Enable DAC1 (Left), Enable DAC1 (Right), 
    also Enable DAC2 (Left), Enable DAC2 (Right)*/
    CODEC_IO_Write(WM8994_ADDR, 0x05, 0x0303 | 0x0C0C);
    
    /* Enable the AIF1 Timeslot 0 (Left) to DAC 1 (Left) mixer path */
    CODEC_IO_Write(WM8994_ADDR, 0x601, 0x0001);
    
    /* Enable the AIF1 Timeslot 0 (Right) to DAC 1 (Right) mixer path */
    CODEC_IO_Write(WM8994_ADDR, 0x602, 0x0001);
    
    /* Enable the AIF1 Timeslot 1 (Left) to DAC 2 (Left) mixer path */
    CODEC_IO_Write(WM8994_ADDR, 0x604, 0x0002);
    
    /* Enable the AIF1 Timeslot 1 (Right) to DAC 2 (Right) mixer path */
    CODEC_IO_Write(WM8994_ADDR, 0x605, 0x0002);
    break;
    
  default:
    /* Disable DAC1 (Left), Disable DAC1 (Right), 
    Enable DAC2 (Left), Enable DAC2 (Right)*/
    CODEC_IO_Write(WM8994_ADDR, 0x05, 0x0303);
    
    /* Enable the AIF1 Timeslot 0 (Left) to DAC 1 (Left) mixer path */
    CODEC_IO_Write(WM8994_ADDR, 0x601, 0x0001);
    
    /* Enable the AIF1 Timeslot 0 (Right) to DAC 1 (Right) mixer path */
    CODEC_IO_Write(WM8994_ADDR, 0x602, 0x0001);
    
    /* Disable the AIF1 Timeslot 1 (Left) to DAC 2 (Left) mixer path */
    CODEC_IO_Write(WM8994_ADDR, 0x604, 0x0000);
    
    /* Disable the AIF1 Timeslot 1 (Right) to DAC 2 (Right) mixer path */
    CODEC_IO_Write(WM8994_ADDR, 0x605, 0x0000);
    break;    
  }  
}

/**
  * @brief Sets new frequency.
  * @param DeviceAddr: Device address on communication Bus.
  * @param AudioFreq: Audio frequency used to play the audio stream.
  * @retval 0 if correct communication, else wrong communication
  */
static void wm8994_SetFrequency(uint32_t AudioFreq)
{
  /*  Clock Configurations */
  switch (AudioFreq)
  {
  case  AUDIO_FREQUENCY_8K:
    /* AIF1 Sample Rate = 8 (KHz), ratio=256 */ 
    CODEC_IO_Write(WM8994_ADDR, 0x210, 0x0003);
    break;
    
  case  AUDIO_FREQUENCY_16K:
    /* AIF1 Sample Rate = 16 (KHz), ratio=256 */ 
    CODEC_IO_Write(WM8994_ADDR, 0x210, 0x0033);
    break;

  case  AUDIO_FREQUENCY_32K:
    /* AIF1 Sample Rate = 32 (KHz), ratio=256 */ 
    CODEC_IO_Write(WM8994_ADDR, 0x210, 0x0063);
    break;
    
  case  AUDIO_FREQUENCY_48K:
    /* AIF1 Sample Rate = 48 (KHz), ratio=256 */ 
    CODEC_IO_Write(WM8994_ADDR, 0x210, 0x0083);
    break;
    
  case  AUDIO_FREQUENCY_96K:
    /* AIF1 Sample Rate = 96 (KHz), ratio=256 */ 
    CODEC_IO_Write(WM8994_ADDR, 0x210, 0x00A3);
    break;
    
  case  AUDIO_FREQUENCY_11K:
    /* AIF1 Sample Rate = 11.025 (KHz), ratio=256 */ 
    CODEC_IO_Write(WM8994_ADDR, 0x210, 0x0013);
    break;
    
  case  AUDIO_FREQUENCY_22K:
    /* AIF1 Sample Rate = 22.050 (KHz), ratio=256 */ 
    CODEC_IO_Write(WM8994_ADDR, 0x210, 0x0043);
    break;
    
  case  AUDIO_FREQUENCY_44K:
    /* AIF1 Sample Rate = 44.1 (KHz), ratio=256 */ 
    CODEC_IO_Write(WM8994_ADDR, 0x210, 0x0073);
    break; 
  case  AUDIO_FREQUENCY_88K:
		/* AIF1 Sample Rate = 88.2 (KHz), ratio=256 */ 
    CODEC_IO_Write(WM8994_ADDR, 0x210, 0x0093);
    break; 
  default:
    /* AIF1 Sample Rate = 48 (KHz), ratio=256 */ 
    CODEC_IO_Write(WM8994_ADDR, 0x210, 0x0083);
    break; 
  }
}

PT_CodecOptr Get_CodecOptr(void)
{
	PT_CodecOptr pTtemp;
	pTtemp = &g_tCodecOptr;
	return pTtemp;
}
