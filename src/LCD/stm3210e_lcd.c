/**
  ******************************************************************************
  * @file FSMC_LCD/src/stm3210e_lcd.c 
  * @author   MCD Application Team
  * @version  V2.0.0
  * @date     04/27/2009
  * @brief    This file includes the LCD driver for AM-240320L8TNQW00H
  *           (LCD_ILI9320) and AM-240320LDTNQW00H (SPFD5408B) Liquid Crystal 
  *           Display Module of STM3210E-EVAL board.
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2009 STMicroelectronics</center></h2>
  */ 

/** @addtogroup FSMC_LCD
  * @{
  */ 
  
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "appCommon.h"

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  __IO uint16_t LCD_REG;
  __IO uint16_t LCD_RAM;
} LCD_TypeDef;

/* LCD is connected to the FSMC_Bank1_NOR/SRAM4 and NE4 is used as ship select signal */
#define LCD_BASE    ((uint32_t)(0x60000000 | 0x0C000000))
#define LCD         ((LCD_TypeDef *) LCD_BASE)
/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
//-----extern var -----------------
extern const u8 Font08X16[];
extern const u16 Font16x24[];

/* Private variables ---------------------------------------------------------*/
  /* Global variables to set the written text color */
static  __IO uint16_t TextColor = 0x0000, BackColor = 0xFFFF;

static sFONT_TYPE m_Font;

//---const var------------------------------
static const u16 m_DecDiv[]={1, 10, 100, 1000, 10000};
const u16 m_aBitTable[]={0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 
	0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000};


/* Private function prototypes -----------------------------------------------*/
void Delay10ms(u16 nTick);
/* Private functions ---------------------------------------------------------*/

//-------------------------------
// support font type
// FONT_16X24, FONT_8X16
//
//-----------------------------------
void LCD_SetFont(u16 type)
{
	switch(type)
	{
		case FONT_16X24:
			m_Font.type = type;
			m_Font.width = FONT24_WIDTH;
			m_Font.height = FONT24_HEIGHT;
			m_Font.pFont.nTable = Font16x24;
			break;
			
		case FONT_8x16:
			m_Font.type = type;
			m_Font.width = FONT_WIDTH;
			m_Font.height = FONT_HEIGHT;
			m_Font.pFont.cTable = Font08X16;
		default:
			break;
	}
}
//----------------------------------------------
u16 LCD_GetFont(void)
{
	return m_Font.type;
}
/**
  * @brief  Initializes the LCD.
  * @param  None
  * @retval : None
  */
void STM3210E_LCD_Init(void)
{ 
/* Configure the LCD Control pins --------------------------------------------*/
  LCD_CtrlLinesConfig();

/* Configure the FSMC Parallel interface -------------------------------------*/
  LCD_FSMCConfig();
// after power on delay 200ms
// release reset delay 20ms, reset pin connect to STM_reset pin too
  Delay10ms(20 +2); /* delay 20 ms */
	LCD_ReadReg(0);

// for id=0x4351, LGDP4531
    LCD_WriteReg(0x10,0x0628);	// power ctrl 1
    LCD_WriteReg(0x12,0x0006);	// power ctrl 3
    LCD_WriteReg(0x13,0x0A32);	// power ctrl 4
    LCD_WriteReg(0x11,0x0040);	// power ctrl 2
    LCD_WriteReg(0x15,0x0050);	// regulator ctrl
    LCD_WriteReg(0x12,0x0016);
    Delay10ms(2);
    LCD_WriteReg(0x10,0x5660);
    Delay10ms(2);
    LCD_WriteReg(0x13,0x2A4E);
    LCD_WriteReg(0x01,0x0100);	// driver output ctrl, bit8:seg scan direction
    LCD_WriteReg(0x02,0x0300);	// driving wave

    LCD_WriteReg(0x03,0x1018);	// entry mode, color565_16bit, bit4:5=scan direction

    LCD_WriteReg(0x08,0x0202);	// display ctrl 2
    LCD_WriteReg(0x0A,0x0000);	// display ctrl 4
    LCD_WriteReg(0x30,0x0000);	// gamma ctrl, R30~R39
    LCD_WriteReg(0x31,0x0402);
    LCD_WriteReg(0x32,0x0106);
    LCD_WriteReg(0x33,0x0700);
    LCD_WriteReg(0x34,0x0104);
    LCD_WriteReg(0x35,0x0301);
    LCD_WriteReg(0x36,0x0707);
    LCD_WriteReg(0x37,0x0305);
    LCD_WriteReg(0x38,0x0208);
    LCD_WriteReg(0x39,0x0F0B);
    Delay10ms(2);
    LCD_WriteReg(0x41,0x0002);	// eprom ctrl
    LCD_WriteReg(0x60,0x2700);	// driver output ctrl
    LCD_WriteReg(0x61,0x0001);	// base image display
    LCD_WriteReg(0x90,0x0119);	// panel interface
    LCD_WriteReg(0x92,0x010A);	// panel interface 2
    LCD_WriteReg(0x93,0x0004);	// panel interface 3
    LCD_WriteReg(0xA0,0x0100);	// test reg 1
    LCD_WriteReg(0x07,0x0001);	// display ctrl 1
    Delay10ms(2);
    LCD_WriteReg(0x07,0x0021); 
    Delay10ms(2);
    LCD_WriteReg(0x07,0x0023);
    Delay10ms(2);
    LCD_WriteReg(0x07,0x0033);
    Delay10ms(2);
    LCD_WriteReg(0x07,0x0133);
    Delay10ms(2);
    LCD_WriteReg(0xA0,0x0000);
    Delay10ms(2);
}
//----------------------------------------
u16 LCD_GetColor(u8 bTextColor)
{
	if(bTextColor)
		return TextColor;
	else
		return BackColor;
}
/**
  * @brief  Sets the Text color.
  * @param Color: specifies the Text color code RGB(5-6-5).
  *   and LCD_DrawPicture functions.
  * @retval : None
  */
void LCD_SetTextColor(__IO uint16_t Color)
{
	TextColor = Color;
}

/**
  * @brief  Sets the Background color.
  * @param Color: specifies the Background color code RGB(5-6-5).
  *   LCD_DrawChar and LCD_DrawPicture functions.
  * @retval : None
  */
void LCD_SetBackColor(__IO uint16_t Color)
{
	BackColor = Color;
}
//--------------------------------
void LCD_ClearRect(u16 Xpos, u16 Ypos, u16 width, u16 height)
{
	u16 orgWidth;

	if(height >= LCD_HEIGHT)
		height = LCD_HEIGHT;
	if(width >= LCD_WIDTH)
		width = LCD_WIDTH;
	orgWidth = width;
	
	while(height--)
	{
		LCD_SetCursor(Xpos, Ypos +height);
	    LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
	    while(width--)
	      LCD_WriteRAM(BackColor);
	    width = orgWidth;
	}
}
//--------------------------------------------
void LCD_ClearEndofLine(u16 Xpos, u16 Ypos)
{
	LCD_ClearRect(Xpos, Ypos, LCD_WIDTH - Xpos, FONT_HEIGHT);
}
/**
  * @brief  Clears the selected line.
  * @param Line: the Line to be cleared.
  *   This parameter can be one of the following values:
  * @arg Linex: where x can be 0..9
  * @retval : None
  */
void LCD_ClearLine(uint8_t Line)
{
  LCD_DisplayStringLine(Line, "                    ");
}

/**
  * @brief  Clears the hole LCD.
  * @param Color: the color of the background.
  * @retval : None
  */
void LCD_Clear(uint16_t Color)
{
  uint32_t index = 0;
  
  LCD_SetCursor(0x00, 0x013F); 

  LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */

  for(index = 0; index < 76800; index++)
  {
    LCD->LCD_RAM = Color;
  }  
}

/**
  * @brief  Sets the cursor position.
  * @param Xpos: specifies the X position.
  * @param Ypos: specifies the Y position. 
  * @retval : None
  */
void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos)
{
#if 0
  LCD_WriteReg(R32, Xpos);	// R20h max 239
  LCD_WriteReg(R33, Ypos);  // R21h max 319
#else
	// horz screen direction will swap register range
	// reg03 setting is right/top is original point, reverse width value
	LCD_WriteReg(R32, Ypos);
	LCD_WriteReg(R33, LCD_WIDTH- Xpos);
#endif
}

/**
  * @brief  Draws a character on LCD.
  * @param Xpos: the Line where to display the character shape.
  *   This parameter can be one of the following values:
  * @arg Linex: where x can be 0..9
  * @param Ypos: start column address.
  * @param c: pointer to the character data.
  * @retval : None
  */
__inline void LCD_DrawChar(uint16_t Xpos, uint16_t Ypos, sFONT_TYPE *pFont, uint16_t tableIdx)
{
  uint32_t index, i;

	if(pFont->width <= 8)
	{
		// font 8x16 use width and table in a byte
		for(index= 0; index < pFont->height; index++)
		{
			LCD_SetCursor(Xpos, Ypos++);
		    LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
			for(i=0; i < pFont->width; i++)
			{
				if((pFont->pFont.cTable[index + tableIdx] & m_aBitTable[i]) == 0x00)
					LCD_WriteRAM(BackColor);
				else
					LCD_WriteRAM(TextColor);
			}
		}
	}
	else
	  // font size fix 16x24 in ascii range
	  for(index = 0; index < pFont->height; index++)
	  {
	    LCD_SetCursor(Xpos, Ypos++);
	    LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
	    for(i = 0; i < pFont->width; i++)
	    {
	      if((pFont->pFont.nTable[index + tableIdx] & m_aBitTable[i]) == 0x00)
	      {
	        LCD_WriteRAM(BackColor);
	      }
	      else
	      {
	        LCD_WriteRAM(TextColor);
	      }
	    }
	  }
}

/**
  * @brief  Displays one character (16dots width, 24dots height).
  * @param Line: the Line where to display the character shape .
  *   This parameter can be one of the following values:
  * @arg Linex: where x can be 0..9
  * @param Column: start column address.
  * @param Ascii: character ascii code, must be between 0x20 and 0x7E.
  * @retval : None
  */
void LCD_DisplayChar(uint16_t Line, uint16_t Column, uint8_t Ascii)
{
	if(Ascii < 0x20)
		Ascii = 0x20;
	
	Ascii -= 0x20;
	LCD_DrawChar(Column, Line, &m_Font, Ascii * m_Font.height);
}

/**
  * @brief  Displays a maximum of 20 char on the LCD.
  * @param Line: the Line where to display the character shape .
  *   This parameter can be one of the following values:
  * @arg Linex: where x can be 0..9
  * @param *ptr: pointer to string to display on LCD.
  * @retval : None
  */
void LCD_DisplayStringLine(uint8_t Line, uint8_t *ptr)
{
  uint32_t i = 0;
  uint16_t refcolumn = 0;

  /* Send the string character by character on lCD */
  while ((*ptr != 0) & (i < (LCD_WIDTH / m_Font.width)))
  {
    /* Display one character on LCD */
    LCD_DisplayChar(Line, refcolumn, *ptr);
    /* Decrement the column position by 16 */
    refcolumn += m_Font.width;
    /* Point on the next character */
    ptr++;
    /* Increment the character counter */
    i++;
  }
}
//---------------------------------
void LCD_DisplayString(u16 posX, u16 posY, u8 *ptr)
{
	u16 i=0;

	// max string characters < 20 in a line
	while(*ptr && (i < 20))
	{
		LCD_DisplayChar(posY, posX+ (i* m_Font.width), *ptr++);
		i++;
	}
}
//-------------------------------
// show value integer mode
//
// input:
//	posX: pixel mode
//  posY: pixel mode
//  nvalue: input value
//  headZero: lead zero digits
//
// return:
//	how many digits used
//
// note:
//	value=0, headZero=0, it would be nothing output, until headZero=2
//------------------------------
u16 LCD_DispValue(u16 posX, u16 posY, u16 nValue, u16 headZero)
{
	u16 nTemp, nCount=0;
	u16 nDigit =4;	// max value under 65535, so max is digit 5
	u8 leadZ=0;

	while(nDigit && nDigit--) // 1st loop not used --
	{
		// chk lead zero
		if(headZero && !leadZ && (nDigit < headZero-1))
			leadZ =1;
		nTemp = nValue / m_DecDiv[nDigit];
		if(nTemp || nCount || leadZ)
		{
			LCD_DrawChar(posX + (nCount * m_Font.width), posY, &m_Font, (nTemp+ '0' -0x20) * m_Font.height);
			nCount++;	// ex. 350 have to show '0' after valid start
		}
		nValue %= m_DecDiv[nDigit];
	}
	return nCount;
}
//------------------------------------
// input value used unit to show total 1 or many digits under 1.
//
// input:
//	posX: start position X
//	posY: start position Y
//	value: input integer value but it need div unit to use
//  headZero: lead zero for integer value
//-------------------------------------------
void LCD_DispFloat(u16 posX, u16 posY, float fValue, u8 headZero)
{
	u16 nValue = (u16) fValue;
	u16 digits;

	digits = LCD_DispValue(posX, posY, nValue, headZero);
	LCD_DisplayChar(posY, posX + (digits++ *m_Font.width), '.');
	nValue = (fValue - nValue) * 1000;	// 0.xxx * 1000 = xxx for digits using
	LCD_DispValue(posX+ (digits * m_Font.width), posY, nValue, 3); // *1000 have to reserve 3 digis
}

/**
  * @brief  Sets a display window
  * @param Xpos: specifies the X buttom left position.
  * @param Ypos: specifies the Y buttom left position.
  * @param Height: display window height.
  * @param Width: display window width.
  * @retval : None
  */
void LCD_SetDisplayWindow(uint8_t Xpos, uint16_t Ypos, uint8_t Height, uint16_t Width)
{
  /* Horizontal GRAM Start Address */
  if(Xpos >= Height)
  {
    LCD_WriteReg(R80, (Xpos - Height + 1));
  }
  else
  {
    LCD_WriteReg(R80, 0);
  }
  /* Horizontal GRAM End Address */
  LCD_WriteReg(R81, Xpos);
  /* Vertical GRAM Start Address */
  if(Ypos >= Width)
  {
    LCD_WriteReg(R82, (Ypos - Width + 1));
  }  
  else
  {
    LCD_WriteReg(R82, 0);
  }
  /* Vertical GRAM End Address */
  LCD_WriteReg(R83, Ypos);

  LCD_SetCursor(Xpos, Ypos);
}

/**
  * @brief  Disables LCD Window mode.
  * @param  None
  * @retval : None
  */
void LCD_WindowModeDisable(void)
{
  LCD_SetDisplayWindow(239, 0x13F, 240, 320);
  LCD_WriteReg(R3, 0x1018);    
}

/**
  * @brief  Displays a line.
  * @param Xpos: specifies the X position.
  * @param Ypos: specifies the Y position.
  * @param Length: line length.
  * @param Direction: line direction.
  *   This parameter can be one of the following values: Vertical 
  *   or Horizontal.
  * @retval : None
  */
void LCD_DrawLine(uint16_t Xpos, uint16_t Ypos, uint16_t Length, uint8_t Direction)
{ 
  uint32_t i = 0;
  
  LCD_SetCursor(Xpos, Ypos);

  if(Direction == Horizontal)
  {
    LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
    for(i = 0; i < Length; i++)
    {
      LCD_WriteRAM(TextColor);
    }
  }
  else
  {
    for(i = 0; i < Length; i++)
    {
      LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
      LCD_WriteRAM(TextColor);
      Ypos++;
      LCD_SetCursor(Xpos, Ypos);
    }
  }
}

/**
  * @brief  Displays a rectangle.
  * @param Xpos: specifies the X position.
  * @param Ypos: specifies the Y position.
  * @param Height: display rectangle height.
  * @param Width: display rectangle width.
  * @retval : None
  */
void LCD_DrawRect(uint16_t Xpos, uint16_t Ypos, uint8_t Height, uint16_t Width)
{
  LCD_DrawLine(Xpos, Ypos, Width, Horizontal);
  LCD_DrawLine(Xpos, Ypos+Height, Width, Horizontal);
  
  LCD_DrawLine(Xpos, Ypos, Height, Vertical);
  LCD_DrawLine(Xpos +Width-1, Ypos, Height, Vertical);
}

/**
  * @brief  Displays a circle.
  * @param Xpos: specifies the X position.
  * @param Ypos: specifies the Y position.
  * @param Height: display rectangle height.
  * @param Width: display rectangle width.
  * @retval : None
  */
void LCD_DrawCircle(uint8_t Xpos, uint16_t Ypos, uint16_t Radius)
{
  int32_t  D;/* Decision Variable */ 
  uint32_t  CurX;/* Current X Value */
  uint32_t  CurY;/* Current Y Value */ 
  
  D = 3 - (Radius << 1);
  CurX = 0;
  CurY = Radius;
  
  while (CurX <= CurY)
  {
    LCD_SetCursor(Xpos + CurX, Ypos + CurY);
    LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
    LCD_WriteRAM(TextColor);

    LCD_SetCursor(Xpos + CurX, Ypos - CurY);
    LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
    LCD_WriteRAM(TextColor);

    LCD_SetCursor(Xpos - CurX, Ypos + CurY);
    LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
    LCD_WriteRAM(TextColor);

    LCD_SetCursor(Xpos - CurX, Ypos - CurY);
    LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
    LCD_WriteRAM(TextColor);

    LCD_SetCursor(Xpos + CurY, Ypos + CurX);
    LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
    LCD_WriteRAM(TextColor);

    LCD_SetCursor(Xpos + CurY, Ypos - CurX);
    LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
    LCD_WriteRAM(TextColor);

    LCD_SetCursor(Xpos - CurY, Ypos + CurX);
    LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
    LCD_WriteRAM(TextColor);

    LCD_SetCursor(Xpos - CurY, Ypos - CurX);
    LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
    LCD_WriteRAM(TextColor);

    if (D < 0)
    { 
      D += (CurX << 2) + 6;
    }
    else
    {
      D += ((CurX - CurY) << 2) + 10;
      CurY--;
    }
    CurX++;
  }
}

/**
  * @brief  Displays a monocolor picture.
  * @param Pict: pointer to the picture array.
  * @retval : None
  */
void LCD_DrawMonoPict(const uint32_t *Pict)
{
  uint32_t index = 0, i = 0;

  LCD_SetCursor(0, 319); 

  LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
  for(index = 0; index < 2400; index++)
  {
    for(i = 0; i < 32; i++)
    {
      if((Pict[index] & (1 << i)) == 0x00)
      {
        LCD_WriteRAM(BackColor);
      }
      else
      {
        LCD_WriteRAM(TextColor);
      }
    }
  }
}

/**
  * @brief  Displays a bitmap picture loaded in the internal Flash.
  * @param BmpAddress: Bmp picture address in the internal Flash.
  * @retval : None
  */
void LCD_WriteBMP(uint32_t BmpAddress)
{
  uint32_t index = 0, size = 0;

  /* Read bitmap size */
  size = *(__IO uint16_t *) (BmpAddress + 2);
  size |= (*(__IO uint16_t *) (BmpAddress + 4)) << 16;

  /* Get bitmap data address offset */
  index = *(__IO uint16_t *) (BmpAddress + 10);
  index |= (*(__IO uint16_t *) (BmpAddress + 12)) << 16;

  size = (size - index)/2;

  BmpAddress += index;

  /* Set GRAM write direction and BGR = 1 */
  /* I/D=00 (Horizontal : decrement, Vertical : decrement) */
  /* AM=1 (address is updated in vertical writing direction) */
  LCD_WriteReg(R3, 0x1008);
 
  LCD_WriteRAM_Prepare();
 
  for(index = 0; index < size; index++)
  {
    LCD_WriteRAM(*(__IO uint16_t *)BmpAddress);
    BmpAddress += 2;
  }
 
  /* Set GRAM write direction and BGR = 1 */
  /* I/D = 01 (Horizontal : increment, Vertical : decrement) */
  /* AM = 1 (address is updated in vertical writing direction) */
  LCD_WriteReg(R3, 0x1018);
}

/**
  * @brief  Writes to the selected LCD register.
  * @param LCD_Reg: address of the selected register.
  * @arg LCD_RegValue: value to write to the selected register.
  * @retval : None
  */
void LCD_WriteReg(uint8_t LCD_Reg, uint16_t LCD_RegValue)
{
  /* Write 16-bit Index, then Write Reg */
  LCD->LCD_REG = LCD_Reg;
  /* Write 16-bit Reg */
  LCD->LCD_RAM = LCD_RegValue;
}

/**
  * @brief  Reads the selected LCD Register.
  * @param  None
  * @retval : LCD Register Value.
  */
uint16_t LCD_ReadReg(uint8_t LCD_Reg)
{
  /* Write 16-bit Index (then Read Reg) */
  LCD->LCD_REG = LCD_Reg;
  /* Read 16-bit Reg */
  return (LCD->LCD_RAM);
}

/**
  * @brief  Prepare to write to the LCD RAM.
  * @param  None
  * @retval : None
  */
void LCD_WriteRAM_Prepare(void)
{
  LCD->LCD_REG = R34;
}

/**
  * @brief  Writes to the LCD RAM.
  * @param RGB_Code: the pixel color in RGB mode (5-6-5).
  * @retval : None
  */
void LCD_WriteRAM(uint16_t RGB_Code)
{
  /* Write 16-bit GRAM Reg */
  LCD->LCD_RAM = RGB_Code;
}

/**
  * @brief  Reads the LCD RAM.
  * @param  None
  * @retval : LCD RAM Value.
  */
uint16_t LCD_ReadRAM(void)
{
  /* Write 16-bit Index (then Read Reg) */
  LCD->LCD_REG = R34; /* Select GRAM Reg */
  /* Read 16-bit Reg */
  return LCD->LCD_RAM;
}

/**
  * @brief  Power on the LCD.
  * @param  None
  * @retval : None
  */
void LCD_PowerOn(void)
{
/* Power On sequence ---------------------------------------------------------*/
  LCD_WriteReg(R16, 0x0000); /* SAP, BT[3:0], AP, DSTB, SLP, STB */
  LCD_WriteReg(R17, 0x0000); /* DC1[2:0], DC0[2:0], VC[2:0] */
  LCD_WriteReg(R18, 0x0000); /* VREG1OUT voltage */
  LCD_WriteReg(R19, 0x0000); /* VDV[4:0] for VCOM amplitude*/
  Delay10ms(20);                 /* Dis-charge capacitor power voltage (200ms) */
  LCD_WriteReg(R16, 0x17B0); /* SAP, BT[3:0], AP, DSTB, SLP, STB */
  LCD_WriteReg(R17, 0x0137); /* DC1[2:0], DC0[2:0], VC[2:0] */
  Delay10ms(5);                 /* Delay 50 ms */
  LCD_WriteReg(R18, 0x0139); /* VREG1OUT voltage */
  Delay10ms(5);            /* Delay 50 ms */
  LCD_WriteReg(R19, 0x1d00); /* VDV[4:0] for VCOM amplitude */
  LCD_WriteReg(R41, 0x0013); /* VCM[4:0] for VCOMH */
  Delay10ms(5);             /* Delay 50 ms */
  LCD_WriteReg(R7, 0x0173);  /* 262K color and display ON */
}

/**
  * @brief  Enables the Display.
  * @param  None
  * @retval : None
  */
void LCD_DisplayOn(void)
{
  /* Display On */
  LCD_WriteReg(R7, 0x0173); /* 262K color and display ON */
}

/**
  * @brief  Disables the Display.
  * @param  None
  * @retval : None
  */
void LCD_DisplayOff(void)
{
  /* Display Off */
  LCD_WriteReg(R7, 0x0); 
}

/**
  * @brief  Configures LCD Control lines (FSMC Pins) in alternate function
  *   mode.
  * @param  None
  * @retval : None
  */
void LCD_CtrlLinesConfig(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable FSMC, GPIOD, GPIOE, GPIOF, GPIOG and AFIO clocks */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE |
                         RCC_APB2Periph_GPIOF | RCC_APB2Periph_GPIOG |
                         RCC_APB2Periph_AFIO, ENABLE);

  /* Set PD.00(D2), PD.01(D3), PD.04(NOE), PD.05(NWE), PD.08(D13), PD.09(D14),
     PD.10(D15), PD.14(D0), PD.15(D1) as alternate 
     function push pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_5 |
                                GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_14 | 
                                GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOD, &GPIO_InitStructure);

  /* Set PE.07(D4), PE.08(D5), PE.09(D6), PE.10(D7), PE.11(D8), PE.12(D9), PE.13(D10),
     PE.14(D11), PE.15(D12) as alternate function push pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | 
                                GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | 
                                GPIO_Pin_15;
  GPIO_Init(GPIOE, &GPIO_InitStructure);

  /* Set PF.00(A0 (RS)) as alternate function push pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_Init(GPIOF, &GPIO_InitStructure);

  /* Set PG.12(NE4 (LCD/CS)) as alternate function push pull - CE3(LCD /CS) */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
  GPIO_Init(GPIOG, &GPIO_InitStructure);
}

/**
  * @brief  Configures the Parallel interface (FSMC) for LCD(Parallel mode)
  * @param  None
  * @retval : None
  */
void LCD_FSMCConfig(void)
{
  FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure;
  FSMC_NORSRAMTimingInitTypeDef  p;

/*-- FSMC Configuration ------------------------------------------------------*/
  /* FSMC_Bank1_NORSRAM4 timing configuration */
  p.FSMC_AddressSetupTime = 1;
  p.FSMC_AddressHoldTime = 0;
  p.FSMC_DataSetupTime = 5;
  p.FSMC_BusTurnAroundDuration = 0;
  p.FSMC_CLKDivision = 0;
  p.FSMC_DataLatency = 0;
  p.FSMC_AccessMode = FSMC_AccessMode_B;

  /* FSMC_Bank1_NORSRAM4 configured as follows:
        - Data/Address MUX = Disable
        - Memory Type = SRAM
        - Data Width = 16bit
        - Write Operation = Enable
        - Extended Mode = Disable
        - Asynchronous Wait = Disable */
  FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM4;
  FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
  FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_SRAM;
  FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;
  FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
  FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
  FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
  FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
  FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
  FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
  FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Disable;
  FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
  FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &p;
  FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &p;

  FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);  

  /* Enable FSMC_Bank1_NORSRAM4 */
  FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM4, ENABLE);
}

/**
  * @}
  */ 

/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
