#include "stm3210e_lcd.h"
#include "stm32f10x_rcc.h"
#include "fsmc_sram.h"
#include "appCommon.h"

#include "platform_config.h"

#define KEY_BOUNCE 2	// over systick count is valid key
#define TOUCH_IN_RANGE	30 // 30~220 is refer range

enum{
	kbKEY1,
	kbKEY2,
	kbINVALID
};
//----------------------------
sSIGNAL m_signal;
static u8 m_cKeyBounce[kbINVALID];
u32 m_nTaskTick[taskINVALID];
u16 m_busyFlag=0;
u16 m_KeyEvent=0;

extern const POSITION m_aPosMenu, m_aPosScaleMT, m_aPosScaleMV, m_aPosScalePT, m_aPosScalePV, m_aPosTrigEd, m_aPosTrigLv;
extern u16 m_anSideStartX;
//---------------------------
extern void ADC_InitDMA(void);
extern void lcdProcess(void);
extern void Touch_Initializtion(void);
extern u16 TPReadX(void);
extern u16 TPReadY(void);

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
  NVIC_SetPriority(SysTick_IRQn, 0x0);

  NVIC_InitStructure.NVIC_IRQChannel = EXTI_IRQ_KEY1;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  NVIC_InitStructure.NVIC_IRQChannel = EXTI_IRQ_KEY2;
  NVIC_Init(&NVIC_InitStructure);

  // Enable DMA channel3 IRQ Channel, LCD_DMA
  NVIC_InitStructure.NVIC_IRQChannel =  DMA1_Channel3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /*NVIC_InitStructure.NVIC_IRQChannel = SDIO_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure); */
}
//-------------------------------
//--- side bar, 120x200(max 7 chars, 8 lines) by font=16x24, pop menu, all side is touch region
//Trig Lv|	// allow touch to enable new setting
//   1.2V|	// 
//------
//Tg Edge|	// trig edge rising/falling, touch toggle
//Pos|Neg|
//-------
//V Scale|
//Pos|Neg|
//-------
//T_Scale|
//Pos|Neg|
//--------------------------------
static void checkValidTouch(u16 posX, u16 posY)
{
	// debug only
	// TODO: disp value can convert to str mode
	LCD_DispValue(0,0, 4, posX);
	LCD_DispValue(FONT_WIDTH*5, 0, 4, posY);

	// info bar only menu key...??
	if(posY >= m_aPosMenu.posY)
	{
		// y pos in range first
		if(posY <= (m_aPosMenu.posY + FONT_HEIGHT))
			if((posX >= m_aPosMenu.posX) && 
				(posX <= (m_aPosMenu.posX+ FONT_WIDTH*m_aPosMenu.width)))
				m_KeyEvent = keyTMENU;
	}
	// confirm in side bar range
	else if(m_signal.sideDisp && (posX >= m_anSideStartX))
	{
		u16 tempY;

		// middle of side bar, after + font_height
		tempY = m_aPosScalePV.posY;
		if(posY < tempY)
		{
			// possible be up 2 items
			if((posY < m_aPosTrigLv.posY + FONT_HEIGHT) && (posY >= m_aPosTrigLv.posY))
				m_KeyEvent = keyTTRIG_LEVEL;
			else if((posY <= m_aPosTrigEd.posY + FONT_HEIGHT) && (posY >= m_aPosTrigEd.posY))
				m_KeyEvent = keyTTRIG_EDGE;
		}
		else
		{
			// possible be down 2 items
			if(posY <= tempY *FONT_HEIGHT)
			{
				if(posX < m_aPosScaleMV.posX)
					m_KeyEvent = keyTSCALE_PLUSV;
				else
					m_KeyEvent = keyTSCALE_MINUSV;
			}
			else 
			{
				tempY = m_aPosScaleMT.posY;
				if((posY > tempY) && (posY < (tempY + FONT_HEIGHT)))
				{
					if(posX < m_aPosScaleMT.posX)
						m_KeyEvent = keyTSCALE_PLUST;
					else
						m_KeyEvent = keyTSCALE_MINUST;
				}
			}
		}
	}// end of sig.sideDisp
	// grid area should be don't care
}
//-----------------------------------------------
// polling 2 toggle key and touch key status
//--------------------------------------------
static void keyProcess(void)
{
	u16 tpX, tpY;
	
// every tick to chk it
	if(sys_msec == m_nTaskTick[taskKEY])
		return;
	m_nTaskTick[taskKEY] = sys_msec;
	
	if(GPIO_ReadInputDataBit(PORT_KEY1, GPIO_KEY1) == KEY_PRESS)
	{
		if(++m_cKeyBounce[kbKEY1] > KEY_BOUNCE)
		{
			m_KeyEvent = keyKEY_START;
			if(m_signal.startAdc)
				m_signal.startAdc =0;
			else
				m_signal.startAdc =1;
			lcdUpdateStart();
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

			lcdUpdateMode();
		}
	}
	else
		m_cKeyBounce[kbKEY2] =0;

	tpX = TPReadX();
	if(tpX < TOUCH_IN_RANGE)
		return;
	
	tpY = TPReadY();
	if(tpY < TOUCH_IN_RANGE)
		return;

	checkValidTouch(tpX, tpY);
}

//--------------------------------------
void main(void)
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
  ADC_Config();
  InterruptConfig();
  Touch_Initializtion();
  SysTick_Config(SystemFrequency /100);	// 10ms

//------------------- Drivers Initialization -------------------------------
	STM3210E_LCD_Init();
	FSMC_SRAM_Init();
	LCD_Clear(Blue);
	LCD_SetTextColor(White);
	LCD_SetBackColor(Black);
	LCD_DisplayStringLine(Line3, "redScope Initializing...");
	GPIO_SetBits(PORT_BKLIT, GPIO_BKLIT);	// back light on
	Delay10ms(100);	// initial time long enough can save this delay time for user.
	lcdInitScreen();

	LCD_WindowModeDisable();

	taskTick = sys_msec;
  while(1)
  {
  	// start combined lcd, adc update process
  	if(m_signal.startProc)
  		lcdProcess();

  	keyProcess();

 //---keep idle chk at last of the loop
  	if(!m_busyFlag && (taskTick == sys_msec))
  		__WFI();
  	taskTick = sys_msec;
  }
}
