#include "stm3210e_lcd.h"
#include "stm32f10x_rcc.h"
#include "fsmc_sram.h"
#include "appCommon.h"
#include "platform_config.h"

#define KEY_BOUNCE 2	// over systick count is valid key
#define TOUCH_IN_RANGE	 0xE00 // normal is 0xFFF touch would be down value
#define TOUCH_MIN		200
#define ADC_RES2	9880	// measured resistor by scope is required
#define ADC_RES1	9810

#define KEY_TASK_TICK	2	// systick

enum{
	kbKEY1,
	kbKEY2,
	kbINVALID
};

typedef struct{
	u8 keyEvent;
	void (*pFunc)(void);
}sKEY_FUNC;
//----------------------------
sSIGNAL m_signal;
static u8 m_cKeyBounce[kbINVALID];
volatile u32 m_nTaskTick[taskINVALID];
u16 m_busyFlag=0;	
u16 m_KeyEvent=0;

void nullFunc(void);
static const sKEY_FUNC m_keyFunc[]={
	// !! follow key event enum seq to build the table !!
		{keyKEY_START, 	lcdUpdateStart},
		{keyKEY_MODE, 	lcdUpdateMode},
		{keyTMENU, 		nullFunc},
		{keyTSCALE_PLUSV, nullFunc},
		{keyTSCALE_MINUSV, nullFunc },
		{keyTSCALE_PLUST, nullFunc },
		{keyTSCALE_MINUST, nullFunc },
		{keyTALIGN_T, 	nullFunc },
		{keyTTRIG_LEVEL, nullFunc },
		{keyTTRIG_EDGER, nullFunc },
		{keyTTRIG_EDGEF, nullFunc }
	};
extern const u16 m_thermCurve10k[], m_thermLen;
//---------------------------
extern void lcdProcess(void);
extern void Touch_Initializtion(void);
extern u16 TPReadX(void);
extern u16 TPReadY(void);
extern void checkButton(u16 posX, u16 posY);
//--------------------------------------------
static void nullFunc(void)
{

}
//--------------------------------------------
static u8 Hex2Str(u32 nValue, u8 *cBuf)
{
	u8 cDigits;
	u8 cData;
	u8 cTemp[8];		// max 8 digits for u32

	if(!nValue)
	{
		*cBuf = '0';
		return 1;
	}

	// for loop dec then chk stop condition.
	for(cDigits=7; nValue != 0; cDigits--)
	{
		cData = nValue & 0x0F;
		if(cData >= 10)
			cTemp[cDigits] = cData -10 + 'A';
		else
			cTemp[cDigits] = cData + '0';
		nValue = nValue >> 4;
	}
	cDigits++;	

	// ex. cTemp[5,6,7] -> cBuf[0,1,2],start and end by cDigits
	for(cData=0; cDigits < 8; cDigits++)
		cBuf[cData++] = cTemp[cDigits];

	return cData;
}
//-----------------------------------------------
u8 Dec2Str(u16 nValue, u8 *cBuf)
{
	u8 cIndex, cCnt=0;
	u16 nDivid = 10000;

	if(nValue < 10)
	{
		*cBuf = nValue +  '0';
		return 1;
	}
	
	for(cIndex =0; cIndex < 5; cIndex++)
	{
		if((nValue >= nDivid) || (cCnt != 0))
		{
			if(nDivid == 1)	// last digit
			{
				*(cBuf + cCnt) = nValue + '0';
				return ++cCnt;
			}
			else
				*(cBuf + cCnt) = (nValue / nDivid) + '0';
			nValue %= nDivid;
			cCnt++;
		}
		nDivid /= 10;
	}
	return 0;	// impossible be here...
}

// -----------------------------------------------------------------------------
static u16 STR2HEX(u8 *c)
  {
   u16 value,t;

   t=*c;
   if (t >='0' && t <='9') value=(t-'0')<<4;
   else if (t>='a' && t <='f') value=(t-'a'+10)<<4;
   else if (t>='A' && t <='F') value=(t-'A'+10)<<4;
   else return(0);
  
   t=*(c+1);
   if (t >='0' && t <='9') value+=(t-'0');
   else if (t>='a' && t <='f') value+=(t-'a'+10);
   else if (t>='A' && t <='F') value+=(t-'A'+10);
   else return(0);
  
   return(value);
}

/**------------------------------------
  * @brief  Configures the different GPIO ports pins.
  * @param  None
  * @retval : None
  */
void GPIO_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
	
  /* SEL Button */
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource8);

  /* KEY Button */
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOD, GPIO_PinSource3);
  
  GPIO_InitStructure.GPIO_Pin = GPIO_KEY1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(PORT_KEY1, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = GPIO_KEY2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(PORT_KEY2, &GPIO_InitStructure);

  /* Configure PC.03 (ADC Channel13) as analog input -------------------------*/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_AIN1 | GPIO_AIN2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(PORT_ADC, &GPIO_InitStructure);

	// LED1
  GPIO_InitStructure.GPIO_Pin = GPIO_LED1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(PORT_LED1, &GPIO_InitStructure);
  GPIO_SetBits(PORT_LED1, GPIO_LED1);

	// LCD
	GPIO_ResetBits(PORT_BKLIT, GPIO_BKLIT);
  GPIO_InitStructure.GPIO_Pin = GPIO_BKLIT;
  GPIO_Init(PORT_BKLIT, &GPIO_InitStructure);
}

/**--------------------------------------
  * @brief  Configures the used IRQ Channels and sets their priority.
  * @param  None
  * @retval : None
  --------------------------------------*/
void InterruptConfig(void)
{ 
  NVIC_InitTypeDef NVIC_InitStructure;

  /* Set the Vector Table base address at 0x08000000 */
  NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x00);

  /* Configure the Priority Group to 2 bits */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

  /* Configure the SysTick handler priority */
  NVIC_SetPriority(SysTick_IRQn, 0x7);

  NVIC_InitStructure.NVIC_IRQChannel = EXTI_IRQ_KEY1;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  NVIC_InitStructure.NVIC_IRQChannel = EXTI_IRQ_KEY2;
  NVIC_Init(&NVIC_InitStructure);

  /*/ Enable DMA channel3 IRQ Channel, LCD_DMA
  NVIC_InitStructure.NVIC_IRQChannel =  DMA1_Channel3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  NVIC_InitStructure.NVIC_IRQChannel = SDIO_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure); */
}

//------------------------------------------
void CheckKey(void)
{
	if(GPIO_ReadInputDataBit(PORT_KEY1, GPIO_KEY1) == KEY_PRESS)
	{
		if(++m_cKeyBounce[kbKEY1] > KEY_BOUNCE)
		{
			m_KeyEvent = keyKEY_START;
			if(m_signal.startAdc)
				m_signal.startAdc =0;
			else
				m_signal.startAdc =1;
		}
	}
	else
		m_cKeyBounce[kbKEY1] =0;

	if(GPIO_ReadInputDataBit(PORT_KEY2, GPIO_KEY2) == KEY_PRESS)
	{
		if(++m_cKeyBounce[kbKEY2] > KEY_BOUNCE)
		{
			m_KeyEvent = keyKEY_MODE;
			if(++m_signal.trigMode >= modeINVALID)
				m_signal.trigMode = modeAUTO;
		}
	}
	else
		m_cKeyBounce[kbKEY2] =0;
}
//----------------------------------------
static __inline u16 convertTPX(u16 value)
{
	// left corner: X=touch_in_range, Y=200
	// touch_in_range:200 == 0 : lcd_width, convert in percent of lcd_width
	// lcd_width * (n-200)/(touch_in_range-200)= posX
	return LCD_WIDTH - ((value - TOUCH_MIN) * LCD_WIDTH)/ (TOUCH_IN_RANGE - TOUCH_MIN);
}
//--------------------------------
static __inline u16 convertTPY(u16 value)
{
	return ((value - TOUCH_MIN) * LCD_HEIGHT)/ (TOUCH_IN_RANGE - TOUCH_MIN);
}
//--------------------------------------
static __inline void ExecuteKeyEvent(void)
{
	void (*pFunc)(void);

	pFunc = m_keyFunc[m_KeyEvent].pFunc;
	pFunc();
	m_KeyEvent = keyINVALID;
}
//-----------------------------------------------
// polling 2 toggle key and touch key status
//--------------------------------------------
static void keyProcess(void)
{
	u16 tpX, tpY;
	
	m_nTaskTick[taskKEY] = sys_msec + KEY_TASK_TICK;

//--- toggle key
	// m_signal.startAdc in capture mode will adcIn to LCD drawing
	if(GPIO_ReadInputDataBit(PORT_KEY1, GPIO_KEY1) == KEY_PRESS)
	{
		if(++m_cKeyBounce[kbKEY1] > KEY_BOUNCE)
		{
			m_KeyEvent = keyKEY_START;
			if(m_signal.startAdc)
				m_signal.startAdc =0;
			else
				m_signal.startAdc =1;
		}
	}
	else
		m_cKeyBounce[kbKEY1] =0;

	if(GPIO_ReadInputDataBit(PORT_KEY2, GPIO_KEY2) == KEY_PRESS)
	{
		if(++m_cKeyBounce[kbKEY2] > KEY_BOUNCE)
		{
			m_KeyEvent = keyKEY_MODE;
			if(++m_signal.trigMode >= modeINVALID)
				m_signal.trigMode = modeAUTO;
		}
	}
	else
		m_cKeyBounce[kbKEY2] =0;

//--- execute toggle key event
	if(m_KeyEvent < keyINVALID)
		ExecuteKeyEvent();

//--- touch key
	tpX = TPReadX();
	if(tpX > TOUCH_IN_RANGE)
		return;
	
	tpY = TPReadY();
	if(tpY > TOUCH_IN_RANGE)
		return;

	tpX = convertTPX(tpX);
	tpY = convertTPY(tpY);
	if(m_signal.debug)
	{
		u8 cTemp[7] = "X:";
		u8 cCnt;
		
		cCnt = 2+ Dec2Str(tpX, &cTemp[2]);
		cTemp[cCnt] =0;
		LCD_DisplayString(FONT_WIDTH*4, Line0, cTemp);

		cTemp[0] = 'Y';
		cCnt = 2+ Dec2Str(tpY, &cTemp[2]);
		cTemp[cCnt] =0;
		LCD_DisplayString(FONT_WIDTH*4, Line1, cTemp);

		utilSoftCountSet(ecoDELAY, 100);
		while(utilSoftCountGet(ecoDELAY))
			__WFI();
		LCD_ClearRect(FONT_WIDTH*4, Line0, FONT_WIDTH * 5, Line2);
	}
	checkButton(tpX, tpY);

	//--- execute toggle key event
	if(m_KeyEvent < keyINVALID)
		ExecuteKeyEvent();
}
//----------------------------------------
static u16 GetTemper(u8 channel)
{
	u16 nMax = m_thermLen;
	u16 nIndex = nMax /2;
	u16 nMin =0;
	u32 nRes, nTemp;

	// core temper used vref setting enable before get data
	nTemp = ADCGetChannel(channel);
	// Vt1= 3.3 * R1/(Rt+R1) = adc1*3.3/4096, R1=9880
	// Vt2= 3.3 * R2/(Rt+R2), R2=9810
	// so, R1(4096-adc)/adc = Rt
	if(channel == TRIG_CORE_TEMPER)
	{
		// ( V25-Vsense)/AVG_slope + 25 = temper('C)
		// V25 = 1.34/1.43/1.52V, AVG_slope = 4.0/4.3/4.6mV/'C if all value use typical, simple method become
		// 6CFh = 1.43/3.36 * 4096 = V25
		// (6CFh - DVsense) * 19 /10 + 250 = temper('C), remain 1 float 
		return (((0x6CF - nTemp) * 19 /10) + 250);
	}
	else if(channel == TRIG_CH1)
		nRes = ADC_RES1;
	else
		nRes = ADC_RES2;
	nTemp = ((4096 - nTemp) * nRes) / nTemp;
 
	while((nMax - nMin) > 1)
	{
		// connect resistor 20k, table used for 10k
		if(nTemp <= m_thermCurve10k[nIndex] *2)
			nMin = nIndex;
		else
			nMax = nIndex;
		nIndex = (nMax - nMin) /2 + nMin;
	}
	// +2 to adjust actual measure offset
	return (nIndex +5+2);	// thermal table start from 5'C, use nMax value close result, (manual offset+5)
}
//---------------------------------------
static void TemperProcess(void)
{
	u32 nValue;
	
	m_nTaskTick[taskTemper] = sys_msec + 100;
	
	LCD_DispValue(6*FONT_WIDTH, Line1, GetTemper(TRIG_CH1), 0);
	LCD_DispValue(6*FONT_WIDTH, Line2, GetTemper(TRIG_CH2), 0);
	// on board resistor
	nValue = ADCGetChannel(ADC_Channel_13);
	nValue = nValue *3300/4096;
	LCD_DispFloat(6*FONT_WIDTH, Line4, (float)nValue/1000, 0);
	// cpu temperature
	LCD_DispValue(6*FONT_WIDTH, Line5, GetTemper(TRIG_CORE_TEMPER), 3);
}
//--------------------------------------
int main(void)
{
	u32 taskTick;

  SystemInit();
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |RCC_APB2Periph_GPIOC 
         | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE | RCC_APB2Periph_GPIOF | RCC_APB2Periph_GPIOG 
         | RCC_APB2Periph_AFIO | RCC_APB2Periph_ADC1, ENABLE);
  // DMA1 for FSMC, DMA2 for SD, FSMC for LCD and SRAM
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1 | RCC_AHBPeriph_DMA2 | RCC_AHBPeriph_FSMC, ENABLE);
 
//------------------- Resources Initialization -----------------------------
  GPIO_Config();
  InterruptConfig();
  Touch_Initializtion();
  SysTick_Config(SystemFrequency /100);	// 10ms

//------------------- Drivers Initialization -------------------------------
	STM3210E_LCD_Init(); // NE4 used 6c00 0000h for lcd_bus
	FSMC_SRAM_Init();	// NE3 used 6800 0000h for sram 512_16 bits
	LCD_Clear(Red);
	GPIO_SetBits(PORT_BKLIT, GPIO_BKLIT);	// back light on
	LCD_SetTextColor(White);
	LCD_SetBackColor(Black);
	LCD_DrawRect(20, 200, 30, 30);

	LCD_SetFont(FONT_16X24);
	//LCD_WindowModeDisable();
	LCD_DisplayStringLine(Line1, "ADC1: ");
	LCD_DisplayStringLine(Line2, "ADC2: ");
	LCD_DisplayStringLine(Line3, "redScope Ready!");
	LCD_DisplayStringLine(Line4, "Res: ");

	// pause confirm LCD work
	utilSoftCountSet(ecoDELAY, 100);
	while(utilSoftCountGet(ecoDELAY))
		__WFI();

	lcdInitScreen();
	// default small font for most situation...
	LCD_SetFont(FONT_8x16);
	taskTick = sys_msec;

	// debug used
	m_signal.debug =1;
	while(1)
	{
	  	//lcdProcess();
	  	if(sys_msec >= m_nTaskTick[taskTemper])
	  		TemperProcess();

		if(sys_msec >= m_nTaskTick[taskKEY])
		  	keyProcess();
	
 //---keep idle chk at last of the loop
 // busyflag let main task no delay for some task working continues
	  	if(!m_busyFlag && (taskTick == sys_msec))
	  		__WFI();
	  	taskTick = sys_msec;
	}
}
