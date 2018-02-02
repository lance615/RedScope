/* Includes ------------------------------------------------------------------*/
#include "platform_config.h"

#include "stm32f10x_adc.h"
#include "stm32f10x_tim.h"
#include "system_stm32f10x.h"

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;	

#define ADC1_DR_Address    ((u32)0x4001244C)

#define TIMER_ADC_TRIG		TIM3
#define MAX_ADC_FRAME		2	// double buffer for LCD and ADC capture

u16 m_adcBuf[MAX_ADC_FRAME][LCD_WIDTH];
u8 m_adcGoingIdx=0;

//---------------------------------
extern void DMAChannelConfig(u32 channel, DMA_ChannelSetting * setting);

//------------------------------------------------------------------//
// Config ADC module
// Parameter: 
//          adc: adc number
//              ADC_1
//              ADC_2
//		ADC_3 (some special pin support)
//          sequence: 0(highest)~15
//          prio: not using now...
//          channel: 0~15
//          trig: trigger method
//              ADC_TRIG_BY_CPU
//              ADC_TRIG_BY_TIMER
// return value:
//          NONE
//------------------------------------------------------------------//
void ConfigADC(U32 adc, U8 sequence, U8 prio, U8 channel)
{
	ADC_InitTypeDef ADC_InitStructure;
	ADC_TypeDef *AdcModule;

	AdcModule = (ADC_TypeDef *)adc;
	
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(AdcModule, &ADC_InitStructure);

	ADC_RegularChannelConfig(AdcModule, channel, sequence, ADC_SampleTime_1Cycles5);	// Tconv=1.5cycle+12.5cycle=14cycle, adc max=14Mhz, so fastest =1us
}
//------------------------------------------------------------------//
// Enable or Disable ADC
// Parameter: 
//          adc: adc number
//              ADC_0
//              ADC_1
//          sequence: in ST will start ADC1/2/3, not single pin enabled?
//          cmd: ENABLE or DISABLE
// return value:
//          NONE
//------------------------------------------------------------------//
void ADCCmd(U32 adc, U8 sequence, U8 cmd)
{
    ADC_TypeDef *AdcModule;

	AdcModule = (ADC_TypeDef *)adc;
	
	ADC_Cmd(AdcModule, (FunctionalState)cmd);

	if(cmd == DISABLE)
		return;
	
	ADC_ResetCalibration(AdcModule);

	while(ADC_GetResetCalibrationStatus(AdcModule));

	ADC_StartCalibration(AdcModule);
	while(ADC_GetCalibrationStatus(AdcModule));
}
//------------------------------------------------------------------//
// Get ADC data from fifo
// Parameter: 
//          adc: adc number
//              ADC_0
//              ADC_1
//          sequence: 0,1,2,3
//          *buffer: buffer to save data,which is assumed to be large enough to
//                   hold that many samples
// return value:
//          the number of samples copied to the buffer
//------------------------------------------------------------------//
U32 ReadADCData(U32 adc, U8 sequence, U32 *buffer)
{
	ADC_TypeDef *adcModule;

	adcModule = (ADC_TypeDef *)adc;

	/* Check the end of ADC1 calibration, true is on going */
	if(!ADC_GetFlagStatus(adcModule, ADC_FLAG_EOC))
		return 0;

	*buffer = (adcModule->DR) & 0xFFF;		// high 16 bit is ADC2 result in dual mode
	return 1;
}

//-------------------------------------
u32 ADCGetChannel(u8 channel)
{
	U32 value;

	// sampleing rate: 1MHz
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); // ADCCLK=72/6=12MHz, conversion time is 1.17us

	// Enable ADC0 Clock
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	
	// ADC configuration: use ADC_1, Channel 10, PC0, only 1 pin need conv, sequencer must be 1 for 1st convert
	ConfigADC((u32)TRIG_ADC, 1,0, channel);
	
	// Enable ADC
	ADCCmd((u32)TRIG_ADC, 3, ENABLE);
	if(channel == ADC_Channel_16)	// core temperature
		ADC_TempSensorVrefintCmd(ENABLE);
	ADC_SoftwareStartConvCmd(TRIG_ADC, ENABLE);
	while(!ReadADCData((u32)TRIG_ADC, 3, &value));

	if(channel == ADC_Channel_16)	// core temperature
		ADC_TempSensorVrefintCmd(DISABLE);
	
	// Disable ADC
	ADCCmd((u32)TRIG_ADC, 3, DISABLE);
	
	// Disable ADC0 Clock
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, DISABLE);

	return value;
}
//----------------------------------------
void ADCTrigger(FunctionalState enable)
{
	ADC_ExternalTrigConvCmd(ADC1,  enable);
	TIM_Cmd(TIMER_ADC_TRIG, enable);
}
//------------------------------------------------------------------//
// Config ADC module
// Parameter: 
//	channel: select input data
//	set trig freq to decide sameple rate
//
// return value:
//          NONE
//------------------------------------------------------------------//
void ConfigADCdma(U8 channel, U32 Freq)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
	ADC_InitTypeDef ADC_InitStructure;
	ADC_TypeDef *AdcModule;
	DMA_ChannelSetting DmaSetting;

//---timer
    TIM_TimeBaseInitStruct.TIM_Prescaler = 0;//clock:72MHz 
    TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStruct.TIM_Period = (SystemFrequency /Freq)-1;
    TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStruct.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIMER_ADC_TRIG ,&TIM_TimeBaseInitStruct);
	TIM_SelectOutputTrigger(TIMER_ADC_TRIG, TIM_TRGOSource_Update);
	// remain extTrig bit and timer_cmd() to enable adc input

//--- ADC
	AdcModule = ADC1;
	
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); // ADCCLK=72/6=12MHz, conversion time is 1.17us
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T3_TRGO;	// 110628 so many timer can trig it, decide by app in future
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(AdcModule, &ADC_InitStructure);
	ADC_RegularChannelConfig(AdcModule, channel, 0, ADC_SampleTime_1Cycles5);	// Tconv=1.5cycle+12.5cycle=14cycle, adc max=14Mhz, so fastest =1us
	ADC_Cmd(ADC1, ENABLE);
	
//--- DMA
	DmaSetting.SrcAddr = (void *)ADC1_DR_Address;
	DmaSetting.Dir = DMA_DIR_SRC;
	DmaSetting.DataSize =DMA_SIZE_16;
	DmaSetting.SrcInc = DMA_SRC_INC_NONE;
	DmaSetting.DstInc = DMA_DST_INC_ENABLE;
	DmaSetting.Mode = DMA_MODE_BASIC; 
	
    DmaSetting.TansferSize=LCD_WIDTH;
    DmaSetting.DstAddr = m_adcBuf[m_adcGoingIdx];
    DMAChannelConfig((U32)DMA1_Channel1, &DmaSetting);
    DMA_ITConfig(DMA1_Channel1 ,DMA_IT_TC, ENABLE); // transfer complete, HT=half transfer, TE=transfer error
}
#if 0
//------------------------------------------------
u8 GetHWVersion(void)
{
	U32 value;
	u32 nCurTime = sys_msec;

	// sampleing rate: 1MHz
	SetADCSpeed();

    ConfigGPIO(PORT_HW_VER, GPIO_HW_VER_IN, GPIO_MODE_ANALOGIN);
    	
	// Enable ADC0 Clock
	PeriphClockCmd(PERIPH_ADC2, PERIPH_APB2_GROUP, ENABLE);

	// ADC configuration: use ADC_1, Channel 10, PC0, only 1 pin need conv, sequencer must be 1 for 1st convert
	ConfigADC(HW_VER_ADC, 1,0, HW_VER_ADC_CHANNEL, ADC_TRIG_BY_CPU);

	// Enable ADC
	ADCCmd(HW_VER_ADC, 3, ENABLE);

	// here may call by standby mode, so don't use os delay else cause fetal error
	while((sys_msec - nCurTime) < 2)
		SystemSleep();

	ADCTrigger(HW_VER_ADC, 3);
	while(!ReadADCData(HW_VER_ADC, 3, &value));

	// Disable ADC
	ADCCmd(HW_VER_ADC, 3, DISABLE);
	
	// Disable ADC0 Clock
	PeriphClockCmd(PERIPH_ADC2, PERIPH_APB2_GROUP, DISABLE);

	if(value < 1000)
		return ENGINE_CCD;
	else if((value >= 1000) && (value < 2000))
		return ENGINE_LASER;
	else if((value > 2000) && (value < 3000))
		return ENGINE_2D;	// reserved function
	else
		return ENGINE_CCD;
}
//------------------------------------------------------------
// temperature sensor only ADC0 available 
//------------------------------------------------------------
u16 GetTemperature(void)
{
	U32 value;
	//u32 nCurTime = sys_msec;

	// sampleing rate: 1MHz
	SetADCSpeed();

	// Enable ADC0 Clock
	PeriphClockCmd(PERIPH_ADC1, PERIPH_APB2_GROUP, ENABLE);

	// ADC configuration: use ADC_1, Channel 10, PC0, only 1 pin need conv, sequencer must be 1 for 1st convert
	ConfigADC(ADC_0, 1,0, TEMPERATURE_CHANNEL, ADC_TRIG_BY_CPU);
	ADC_RegularChannelConfig((ADC_TypeDef *)ADC_0, TEMPERATURE_CHANNEL, 1, ADC_SampleTime_239Cycles5);

	// Enable ADC
	ADCCmd(ADC_0, 0, ENABLE);
  	ADC_TempSensorVrefintCmd(ENABLE);
  
	// wait value stable, unit is 10ms
	//while((sys_msec - nCurTime) < 1)
	//	SystemSleep();
	
	ADCTrigger(ADC_0, 0);
	while(!ReadADCData(ADC_0, 0, &value));

	// Disable ADC
	ADCCmd(ADC_0, 0, DISABLE);
	ADC_TempSensorVrefintCmd(DISABLE);
	
	// Disable ADC0 Clock
	PeriphClockCmd(PERIPH_ADC1, PERIPH_APB2_GROUP, DISABLE);

	// ( V25-Vsense)/AVG_slope + 25 = temper('C)
	// V25 = 1.34/1.43/1.52V, AVG_slope = 4.0/4.3/4.6mV/'C if all value use typical, simple method become
	// 6CFh = 1.43/3.36 * 4096 = V25
	// (6CFh - DVsense) * 19 /10 + 250 = temper('C), remain 1 float 
	return (((0x6CF - (S32)value) * 19 /10) + 250);
	//---use ADV value calculate alg
	//	4.3mV/0.82mV=5 dec, (DV25-DVsense)/5+25, mul_10 get (V)*2+250;
//	return (0x6CF - (S32)value)*2 + 250;
}
#endif
