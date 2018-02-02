#include <stdio.h>
#include <string.h>

#include "Platform_config.h"
#include "appCommon.h"
#include "stm3210e_lcd.h"
//-----------------------
//|				|side	|	// side bar need enough width, so it's pop menu
//|  main plot	|bar 	|
//----------------------
//| bot info			|
//----------------------
//
#define INFO_HEIGHT_Y	(FONT_HEIGHT *2)	// Bottom info bar just need start Y pos
#define SIDE_WIDTH_X	120	// R side info bar just need start X pos

#define GRID_SIZE_X		(LCD_WIDTH-8)		// -8 for 12 grids remain pix, side is pop menu
#define GRID_SIZE_Y		(LCD_HEIGHT - INFO_HEIGHT_Y)	// 192
#define GRID_WIDTH		(GRID_SIZE_X / 12)	// 26pix, remain 8pix at last
#define GRID_HEIGHT		(GRID_SIZE_Y / 8)	// 25 pix= 24+1 for grid line
#define GRID_COLOR		Grey	
#define BUTTON_COLOR	0x3FE7	// light green

//--- info bar, 312x40 (include boarder line 2 pixels)
//		align
//-----------------------
//|start  ^ TBase	menu|
//|Mode VBase 			|
//-----------------------
#define INFO_START_POSX		0
#define INFO_START_POSY		(GRID_SIZE_Y +2)
#define STR_START_POSX	INFO_START_POSX
#define STR_START_POSY	INFO_START_POSY
#define STR_MODE_POSX	INFO_START_POSX
#define STR_MODE_POSY	(INFO_START_POSY + FONT_HEIGHT)
//#define STR_ALIGN_POSX	// TODO: fix align arrow on middle, not neccessory adjust now.
//#define STR_ALIGN_POSY
#define STR_VBASE_POSX	(INFO_START_POSX + FONT_WIDTH*6)	// mode keep 4 chars + 1 space, align Tbase
#define STR_VBASE_POSY	(INFO_START_POSY + FONT_HEIGHT)
#define STR_TBASE_POSX	(INFO_START_POSX + FONT_WIDTH*6)	// 6 == start(5 chars) + 1 space
#define STR_TBASE_POSY	INFO_START_POSY

#define SIDE_START_POSX			GRID_SIZE_X
#define SIDE_START_POSY			0

#define SCALE_V_MIN		1	// would be 100mV start unit
#define SCALE_T_MIN		1	// would be 100us start
//-----------------------------------
// large data in sram in noinit section, start on 0x6800 0000, size 0x10 0000
// TODO: setting have to store in flash and find last valid table
SETTINGS m_setting;

//---const string-----------------------
static const u8 m_strStart[]="START";
static const u8 m_strStop[]="STOP";

static const u8 m_strAuto[]="Auto";
static const u8 m_strManual[]="Manual";
static const u8 m_strSingle[]="Single";
static const u8 m_strPos[]="+";	// positive, increase
static const u8 m_strNeg[]="-";	// negtive, minus
static const u8 m_strTrigLv[]="Trig Lv";
static const u8 m_strTrigEd[]="Tg Edge";
static const u8 m_strVscale[]="V_Scale";
static const u8 m_strTscale[]="T_Scale";
static const u8 m_strMenu[]= "Menu";
const u8 m_scaleVtbl[scaleV_INVALID]={
		SCALE_V_MIN, SCALE_V_MIN*5,
		SCALE_V_MIN*10, SCALE_V_MIN *20,
		SCALE_V_MIN *50};
const u32 m_scaleTtbl[scaleT_INVALID] ={
		SCALE_T_MIN, SCALE_T_MIN,
		SCALE_T_MIN* 5, SCALE_T_MIN *10, 
		SCALE_T_MIN* 50, SCALE_T_MIN* 100,
		SCALE_T_MIN* 200, SCALE_T_MIN* 500,
		SCALE_T_MIN* 1000, SCALE_T_MIN *2000,
		SCALE_T_MIN* 5000, SCALE_T_MIN* 10000};
extern u16 m_KeyEvent;

// allow touch to call pop side bar
const BUTTON m_aBtnMenu ={GRID_SIZE_X- (FONT24_WIDTH*4), GRID_SIZE_Y+2, 
							FONT24_WIDTH*4, FONT24_HEIGHT, m_strMenu, keyTMENU};
// side start posx/Y is add by function call, struct define offset only
//--- side bar, 120x200(max 7 chars, 8 lines) by font=16x24, pop menu, all side is touch region
//Trig Lv|	// allow touch to enable new setting
//   1.2V|	// 
//------
//Tg Edge|	// trig edge rising/falling, touch toggle
//Rise Fall|
//-------
//V Scale|
//Pos|Neg|
//-------
//T_Scale|
//Pos|Neg|
const BUTTON m_aBtnTrigLv ={0, 0,
							FONT_WIDTH *7, FONT24_HEIGHT, m_strTrigLv, keyTTRIG_LEVEL};
const BUTTON m_aBtnTrigEdP={0, 			FONT24_HEIGHT *3,
							FONT24_WIDTH *3, FONT24_HEIGHT, m_strPos, keyTTRIG_EDGER};
const BUTTON m_aBtnTrigEdM={FONT24_WIDTH*3 , FONT24_HEIGHT *3,
							FONT24_WIDTH *3, FONT24_HEIGHT, m_strNeg, keyTTRIG_EDGEF};
const BUTTON m_aBtnScalePV={0,			 FONT24_HEIGHT *5,
							FONT_WIDTH *3, FONT_HEIGHT, m_strPos, keyTSCALE_PLUSV};
const BUTTON m_aBtnScaleMV={FONT_WIDTH *4, FONT24_HEIGHT *5,
							FONT_WIDTH *3, FONT24_HEIGHT, m_strNeg, keyTSCALE_MINUSV};
const BUTTON m_aBtnScalePT={0, 			FONT24_HEIGHT *7,
							FONT_WIDTH *3, FONT24_HEIGHT, m_strPos, keyTSCALE_PLUST};
const BUTTON m_aBtnScaleMT={FONT_WIDTH *4, FONT24_HEIGHT *7,
							FONT_WIDTH *3, FONT24_HEIGHT, m_strNeg, keyTSCALE_MINUST};
//-------------------------------------
static void drawSideBar(void)
{
	u16 offsetX = SIDE_START_POSX;	// let define long string be short
	u16 txtColor = LCD_GetColor(1);
	
	LCD_ClearRect(SIDE_START_POSX, SIDE_START_POSY, SIDE_WIDTH_X, GRID_SIZE_Y +2);

	LCD_SetTextColor(BUTTON_COLOR);
	
	LCD_DisplayString(offsetX + m_aBtnTrigLv.posX, m_aBtnTrigLv.posY, (u8*)m_aBtnTrigLv.pStr);

	LCD_DisplayString(offsetX, m_aBtnTrigEdP.posY - FONT24_HEIGHT, (u8*)m_strTrigEd);
	LCD_DisplayString(offsetX + m_aBtnTrigEdP.posX, m_aBtnTrigEdP.posY, (u8*)m_aBtnTrigEdP.pStr);
	LCD_DisplayString(offsetX + m_aBtnTrigEdM.posX, m_aBtnTrigEdM.posY, (u8*)m_aBtnTrigEdM.pStr);

	LCD_DisplayString(offsetX, m_aBtnScaleMV.posY - FONT24_HEIGHT, (u8*)m_strVscale);
	LCD_DisplayString(offsetX + m_aBtnScalePV.posX, m_aBtnScalePV.posY, (u8*)m_aBtnScalePV.pStr);
	LCD_DisplayString(offsetX + m_aBtnScaleMV.posX, m_aBtnScaleMV.posY, (u8*)m_aBtnScaleMV.pStr);

	LCD_DisplayString(offsetX, m_aBtnScaleMT.posY - FONT24_HEIGHT, (u8*)m_strTscale);
	LCD_DisplayString(offsetX + m_aBtnScalePT.posX, m_aBtnScalePT.posY, (u8*)m_aBtnScalePT.pStr);
	LCD_DisplayString(offsetX + m_aBtnScaleMT.posX, m_aBtnScaleMT.posY, (u8*)m_aBtnScaleMT.pStr);

	// region split line and static string
	LCD_SetTextColor(txtColor);
	LCD_DispFloat(offsetX + m_aBtnTrigLv.posX, m_aBtnTrigLv.posY+ FONT_HEIGHT, (float)m_setting.trigLv/10, 0);
	LCD_DrawLine(offsetX, FONT24_HEIGHT *2, SIDE_WIDTH_X, Horizontal);
	LCD_DrawLine(offsetX, FONT24_HEIGHT *4, SIDE_WIDTH_X, Horizontal);
	LCD_DrawLine(offsetX, FONT24_HEIGHT *6, SIDE_WIDTH_X, Horizontal);
}

//-------------------------------
void lcdUpdateStart(void)
{
	u8 const *pStr;
	
	if(m_signal.startAdc)
		pStr = m_strStart;
	else
		pStr = m_strStop;

	LCD_DisplayString(STR_START_POSX, STR_START_POSY, (u8*)pStr);
	
//---ADC
	ADCTrigger(m_signal.startAdc ? ENABLE : DISABLE);
}
//-------------------------------
void lcdUpdateMode(void)
{
	u16 fcolor = LCD_GetColor(1);
	u8 const *pStr;
	
	switch(m_signal.trigMode)
	{
		case modeAUTO:
		default:
			LCD_SetTextColor(Yellow);
			pStr = m_strAuto;
			break;
			
		case modeMANUAL:
			pStr = m_strManual;
			break;

		case modeSINGLE:
			pStr = m_strSingle;
			break;
	}
	LCD_DisplayString(STR_MODE_POSX, STR_MODE_POSY, (u8 *)pStr);
	LCD_SetTextColor(fcolor);
}
//-----------------------------------
//-----------------------
//|				|side	|	// side bar need enough width, so it's pop menu
//|  main plot	|bar 	|
//----------------------
//| bot info			|
//----------------------
void lcdInitScreen(void)
{
	u16 tmpColor =LCD_GetColor(0);
	u16 nposX=0, nposY;

// grid area
	LCD_Clear(LCD_GetColor(0));

	// draw grid in dot line mode
	for(nposY =0; nposY <= GRID_SIZE_Y; nposY++)
	{
		LCD_SetCursor(0, nposY);
		LCD_WriteRAM_Prepare();
		if((nposY % GRID_HEIGHT) == 0)
		{
			// horizon lines
			for(nposX=0 ; nposX< GRID_SIZE_X; nposX++)
			{
				u16 color;

				color = (nposX & 1)? tmpColor : GRID_COLOR;
				LCD_WriteRAM(color);
			} 
		}
		else
		{
			// vertical lines by horz mode to draw
			for(nposX=0 ; nposX<= GRID_SIZE_X; nposX+= GRID_WIDTH)
			{
				LCD_SetCursor(nposX, nposY);
				LCD_WriteRAM_Prepare();
				LCD_WriteRAM(GRID_COLOR);
			}
		}
	}

// menu button
	LCD_SetBackColor(BUTTON_COLOR);
	LCD_ClearRect(m_aBtnMenu.posX, m_aBtnMenu.posY, m_aBtnMenu.width, m_aBtnMenu.height);
	LCD_DisplayString(m_aBtnMenu.posX, m_aBtnMenu.posY, (u8*)m_aBtnMenu.pStr);
	LCD_SetBackColor(tmpColor);
	// bottom info?
}

//--------------------------------
void keyMenuPress(void)
{
	if(m_signal.sideDisp)
	{
		u16 tmpColor = LCD_GetColor(1);
		u16 pos;
		
		m_signal.sideDisp =0;
		LCD_ClearRect(SIDE_START_POSX, SIDE_START_POSY, SIDE_WIDTH_X, GRID_SIZE_Y);
		LCD_SetTextColor(GRID_COLOR);
		for(pos= 0; pos < SIDE_WIDTH_X ; pos +=GRID_WIDTH)
			LCD_DrawLine(SIDE_START_POSX+ pos, SIDE_START_POSY, GRID_SIZE_Y, Vertical);
			
		for(pos = SIDE_START_POSY; pos < GRID_SIZE_Y; pos += GRID_WIDTH)
			LCD_DrawLine(SIDE_START_POSX, pos, SIDE_WIDTH_X, Horizontal);

		LCD_SetTextColor(tmpColor);
	}
	else
	{
		m_signal.sideDisp =1;
		m_signal.startAdc =0;
		lcdUpdateStart();
		drawSideBar();
	}
}
//-----------------------------------
static __inline u16 chkBtnRange(u16 posX, u16 posY, BUTTON const **pBtn, u16 elements)
{
	u16 tempY;
	
	for(tempY=0; tempY < elements; tempY++)
		if((posY >= pBtn[tempY]->posY) && (posY < (pBtn[tempY]->posY+ FONT_HEIGHT)))
			if((posX >= pBtn[tempY]->posX) && (posX <= (pBtn[tempY]->posX + pBtn[tempY]->width)))
				return pBtn[tempY]->keyEvent;

	return keyINVALID;
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
static void chkBtnSideBar(u16 posX, u16 posY)
{
	BUTTON const *pBtn[4];

	// middle of side bar, after + font_height
	if(posY < m_aBtnScalePV.posY)
	{
		// possible be upper items
		pBtn[0] = &m_aBtnTrigLv;
		pBtn[1] = &m_aBtnTrigEdM;
		pBtn[2] = &m_aBtnTrigEdP;

		m_KeyEvent = chkBtnRange(posX, posY, pBtn, 3);
	}
	else
	{
		// possible be down items
		pBtn[0] = &m_aBtnScaleMV;
		pBtn[1] = &m_aBtnScalePV;
		pBtn[2] = &m_aBtnScaleMT;
		pBtn[3] = &m_aBtnScaleMV;

		m_KeyEvent = chkBtnRange(posX, posY, pBtn, 4);
	}
}
//--------------------------------
void checkButton(u16 posX, u16 posY)
{
	// confirm in side bar range
	if(m_signal.sideDisp && (posX >= SIDE_START_POSX))
		chkBtnSideBar(posX, posY);
	else
	{
		BUTTON const *pBtn[1];

		pBtn[0] = &m_aBtnMenu;
		// except side bar just one key would be search now...
		m_KeyEvent = chkBtnRange(posX, posY, pBtn, 1);
	}
	// grid area should be don't care
}
//----------------------------------
void lcdProcess(void)
{
	//drawSideBar();
}
