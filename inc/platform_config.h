#include "stm32f10x.h"
#include "stm32f10x_gpio.h"

//---ADC ------------------
#define PORT_ADC	GPIOC
#define GPIO_AIN1	GPIO_Pin_0
#define GPIO_AIN2	GPIO_Pin_1

#define TRIG_CH1		ADC_Channel_10
#define TRIG_CH2		ADC_Channel_11
#define TRIG_CORE_TEMPER	ADC_Channel_16

#define TRIG_ADC	ADC1
#define TRIG_ADC2	ADC2

//--- DMA--------------
#define DMA_ADC_CHANNEL	DMA1_Channel1

typedef struct
{
    void* SrcAddr;   //source address
    void* DstAddr;   //destination address
    u32 Dir;		// device is src or dst
    				// DMA_DIR_SRC
    				// DMA_DIR_DST
    u32 BurstSize;   //burst size(item number,not bytes)
                        //DMA_BURST_1
                        //DMA_BURST_2
                        //DMA_BURST_4
                        //DMA_BURST_8
                        //DMA_BURST_16
                        //DMA_BURST_32
                        //DMA_BURST_64
                        //DMA_BURST_128
                        //DMA_BURST_256
                        //DMA_BURST_512
                        //DMA_BURST_1024
    u32 DataSize;    //each dma tranfer size
                        //DMA_SIZE_8
                        //DMA_SIZE_16
                        //DMA_SIZE_32
    u32 SrcInc;     //source address increase
                        //DMA_SRC_INC_NONE
                        //DMA_SRC_INC_ENABLE
    u32 DstInc;     //destination address increass
                        //DMA_DST_INC_NONE
                        //DMA_DST_INC_ENABLE
    u32 TansferSize;//total transfer size(item number,not bytes)(<=1024)
    u32 Mode;       //dma operation mode
                        //DMA_MODE_BASIC
                        //DMA_MODE_AUTO
}DMA_ChannelSetting;
#define DMA_DST_INC_8           DMA_MemoryDataSize_Byte
#define DMA_DST_INC_16          DMA_MemoryDataSize_HalfWord
#define DMA_DST_INC_32          DMA_MemoryDataSize_Word
#define DMA_DST_INC_NONE        DMA_MemoryInc_Disable
#define DMA_DST_INC_ENABLE      DMA_MemoryInc_Enable
#define DMA_SRC_INC_8           DMA_PeripheralDataSize_Byte
#define DMA_SRC_INC_16          DMA_PeripheralDataSize_HalfWord
#define DMA_SRC_INC_32          DMA_PeripheralDataSize_Word
#define DMA_SRC_INC_NONE        DMA_PeripheralInc_Disable
#define DMA_SRC_INC_ENABLE      DMA_PeripheralInc_Enable
#define DMA_DIR_SRC			    DMA_DIR_PeripheralSRC
#define DMA_DIR_DST		        DMA_DIR_PeripheralDST
#define DMA_MODE_BASIC          DMA_Mode_Normal
#define DMA_MODE_AUTO           DMA_Mode_Circular
#define DMA_SIZE_8              0x00000000
#define DMA_SIZE_16             0x11000000
#define DMA_SIZE_32             0x22000000

//--- GPIO -----------
#define PORT_KEY1	GPIOA
#define GPIO_KEY1	GPIO_Pin_8
#define PORT_KEY2	GPIOD
#define GPIO_KEY2	GPIO_Pin_3

#define PORT_LED1	GPIOF
#define GPIO_LED1	GPIO_Pin_6

//--- IRQ----------
#define EXTI_IRQ_KEY2	EXTI3_IRQn
#define EXTI_IRQ_KEY1	EXTI9_5_IRQn

//--- LCD---------
#define LCD_WIDTH	320
#define LCD_HEIGHT	240
#define LCD_FRAME_SIZE	(LCD_WIDTH *LCD_HEIGHT)
// refer stm3210e_lcd.c to define all LCD pins
#define PORT_BKLIT	GPIOA
#define GPIO_BKLIT	GPIO_Pin_1

//--------------------------------
u32 ADCGetChannel(u8 channel);
void ADCTrigger(FunctionalState enable);
