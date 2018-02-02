/* Includes ------------------------------------------------------------------*/
#include "platform_config.h"

/* defines -----------------------------------------------------------------*/
#define ADC1_DR_Address    ((u32)0x4001244C)

/* Global Variables ----------------------------------------------------------*/

//------------------------------------------------------------------//
// Enable or Disable DMA Channel
// Parameter: 
//          channel: channel number
//          cmd: ENABLE or DISABLE
// return value:
//          NONE
//------------------------------------------------------------------//
void DMAChannelCmd(u32 channel, u8 cmd)
{
	/* Enable DMA channels */
	DMA_Cmd((DMA_Channel_TypeDef *)channel, (FunctionalState)cmd);
}
//-----------------------------------------------
void DMASetBuffer(DMA_Channel_TypeDef *pDmaCh, u8 *cBuf, u32 nSize)
{
	pDmaCh->CMAR = (u32)cBuf;
	pDmaCh->CNDTR = nSize;
}
//--------------------------------------------
void WaitDMAChComplete(u32 pChannel, u32 nChFlag)
{
	DMA_Channel_TypeDef *pDmaCh;

	pDmaCh = (DMA_Channel_TypeDef *)pChannel;;
	
	// confirm data in length complete
	while(!DMA_GetFlagStatus(nChFlag) || (pDmaCh->CNDTR != 0));
}
//------------------------------------------------------------------//
// Configure DMA Channel
// Parameter: 
//          channel: channel number
//          *setting: config data structure
// return value:
//          NONE
//------------------------------------------------------------------//
void DMAChannelConfig(u32 channel, DMA_ChannelSetting* setting)
{
	DMA_InitTypeDef DMA_InitStructure;

	/* SPI_MASTER_Rx_DMA_Channel configuration ---------------------------------*/
	DMA_DeInit((DMA_Channel_TypeDef *)channel);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)setting->SrcAddr;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)setting->DstAddr;
	DMA_InitStructure.DMA_DIR = setting->Dir;
	DMA_InitStructure.DMA_BufferSize = setting->TansferSize;
	DMA_InitStructure.DMA_PeripheralInc = setting->SrcInc;
	DMA_InitStructure.DMA_MemoryInc = setting->DstInc;
	if(setting->DataSize==DMA_SIZE_8){
	    DMA_InitStructure.DMA_PeripheralDataSize=DMA_SRC_INC_8;  
	    DMA_InitStructure.DMA_MemoryDataSize=DMA_DST_INC_8;
    }else if(setting->DataSize==DMA_SIZE_16){
        DMA_InitStructure.DMA_PeripheralDataSize=DMA_SRC_INC_16;  
	    DMA_InitStructure.DMA_MemoryDataSize=DMA_DST_INC_16;
    }else{
        DMA_InitStructure.DMA_PeripheralDataSize=DMA_SRC_INC_32;  
	    DMA_InitStructure.DMA_MemoryDataSize=DMA_DST_INC_32;
    }        
	DMA_InitStructure.DMA_Mode = setting->Mode;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init((DMA_Channel_TypeDef *)channel, &DMA_InitStructure); 
	DMA_Cmd((DMA_Channel_TypeDef *)channel, ENABLE);
}
//------------------------------------------------------------------//
// Init Dma for ADC
// Parameter: 
//          NONE
// return value:
//          NONE
//------------------------------------------------------------------//
void ADC_InitDMA(void)
{
	DMA_ChannelSetting DmaSetting;

	DmaSetting.SrcAddr = (void *)ADC1_DR_Address;
	DmaSetting.DstAddr=0;
	DmaSetting.Dir = DMA_DIR_SRC;
	DmaSetting.DataSize =DMA_SIZE_32;
	DmaSetting.SrcInc = DMA_SRC_INC_NONE;
	DmaSetting.DstInc = DMA_DST_INC_ENABLE;
	DmaSetting.Mode = DMA_MODE_BASIC; 
    DmaSetting.TansferSize=0;
    DMAChannelConfig((u32)DMA_ADC_CHANNEL, &DmaSetting);
}
//------------------------------------------------------------------//
// Init Dma dst addr and transfer size
// Parameter: 
//          NONE
// return value:
//          NONE
//------------------------------------------------------------------//
void ADC_DMASetBuffer(u8 *buf, u32 size)
{
    DMA_Channel_TypeDef *pDmaCh;

	pDmaCh = DMA_ADC_CHANNEL;
	pDmaCh->CMAR = (u32)buf;
	pDmaCh->CNDTR = size;
}
