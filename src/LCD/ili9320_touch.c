#include "stm32f10x.h"

#define CH_X  0xd0//0x90
#define CH_Y  0x90//0xd0

#define TP_CS()  GPIO_ResetBits(GPIOB, GPIO_Pin_12)
#define TP_DCS() GPIO_SetBits(GPIOB, GPIO_Pin_12)

#define TOUCH_SPI	SPI2

unsigned char SPI_WriteByte(unsigned char data) 
{ 
 unsigned char Data = 0; 

   //Wait until the transmit buffer is empty 
  while(SPI_I2S_GetFlagStatus(TOUCH_SPI ,SPI_I2S_FLAG_TXE)==RESET); 
  // Send the byte  
  SPI_I2S_SendData(TOUCH_SPI,data); 

   //Wait until a data is received 
  while(SPI_I2S_GetFlagStatus(TOUCH_SPI,SPI_I2S_FLAG_RXNE)==RESET); 
  // Get the received data 
  Data = SPI_I2S_ReceiveData(TOUCH_SPI); 

  // Return the shifted data 
  return Data; 
}  
void SpiDelay(unsigned int DelayCnt)
{
 unsigned int i;
 for(i=0;i<DelayCnt;i++);
}

u16 TPReadX(void)
{ 
   u16 x=0;
   TP_CS();
   SpiDelay(10);
   SPI_WriteByte(0x90);
   SpiDelay(10);
   x=SPI_WriteByte(0x00);
   x<<=8;
   x+=SPI_WriteByte(0x00);
   SpiDelay(10);
   TP_DCS(); 
   x = x>>3;
   return (x);
}

u16 TPReadY(void)
{
 u16 y=0;
  TP_CS();
  SpiDelay(10);
  SPI_WriteByte(0xD0);
  SpiDelay(10);
  y=SPI_WriteByte(0x00);
  y<<=8;
  y+=SPI_WriteByte(0x00);
  SpiDelay(10);
  TP_DCS();
  y = y>>3; 
  return (y);
}

void Touch_Initializtion(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  SPI_InitTypeDef SpiInit;

  /*****************************
  **    硬件连接说明          **
  ** STM32         TSC2046    **
  ** PB12    <----> CS       ** o
  ** PB13    <----> SCK        ** o
  ** PB14    <----> MISO       ** i
  ** PB15    <----> MOSI        ** o
  ******************************/
  // MOSI, SCK
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 |GPIO_Pin_14 |GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  // CS
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

//---
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

	SpiInit.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SpiInit.SPI_Mode = SPI_Mode_Master;
	SpiInit.SPI_DataSize = SPI_DataSize_8b;
	SpiInit.SPI_CPOL = SPI_CPOL_Low;
	SpiInit.SPI_CPHA = SPI_CPHA_1Edge;
	SpiInit.SPI_NSS = SPI_NSS_Soft;
	SpiInit.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
	SpiInit.SPI_FirstBit = SPI_FirstBit_MSB;
	SpiInit.SPI_CRCPolynomial =7;
	SPI_Init(TOUCH_SPI, &SpiInit);

	SPI_Cmd(TOUCH_SPI, ENABLE);
}
