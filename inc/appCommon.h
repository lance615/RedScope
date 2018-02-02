#ifndef _APP_COMMON_
#define _APP_COMMON_

#define KEY_PRESS 0
#define KEY_RELEASE 1

#define FONT_16X24	0x24
#define FONT_8x16	0X16

// cpsid i/ cpsie i...in core_cm3.h

enum{
	ecoDELAY,
	ecoINVALID
};

//--- task compare systick for time counting
enum{
	taskKEY,
	taskLCD,
	taskTemper,
	taskINVALID
};

typedef enum{
	scaleFATEST,
	scale100US,
	scale500US,
	scale1MS,	// fatest period need to test
	scale5MS,
	scale10MS,
	scale20MS,
	scale50MS,
	scale100MS,
	scale200MS,
	scale500MS,
	scale1000MS,
	scaleT_INVALID
}eSCALE_T;

typedef enum{
	scale01V,	// 100mV
	scale05V,	// 500mV
	scale1V,
	scale2V,
	scale5V,
	scaleV_INVALID
}eSCALE_V;

extern volatile u32 m_nTaskTick[];
extern volatile u32 sys_msec;

//--- busy flag let main tak not idle. make real time task
#define BUSY_LCD_DMA	0x01

extern u16 m_busyFlag;

//--- key event for toggle key and touch key both
enum{
	keyKEY_START,	// start/ stop
	keyKEY_MODE,	// auto/ manual/ single
	keyTMENU,		// touch key event by 'T'
	keyTSCALE_PLUSV,	// zoom in V
	keyTSCALE_MINUSV, // zoom out V
	keyTSCALE_PLUST,
	keyTSCALE_MINUST,
	keyTALIGN_T,		// touch to select time align base, or input offset timing in future function
	keyTTRIG_LEVEL,		// touch to select valid volt, manaul mode use trig level to update scr.
	keyTTRIG_EDGER,		// touch rising edge to start trig
	keyTTRIG_EDGEF,		// touch falling edge
	keyINVALID
};

enum{
	modeAUTO,		// trig mode
	modeMANUAL,
	modeSINGLE,
	modeINVALID
};

typedef struct
{
	u16 posX;	// left top corner
	u16 posY;
	u16 width;	// valid range
	u16 height;
	const u8 *pStr;	// disp string
	u16 keyEvent; // reserved length
}BUTTON;
//-------------------
typedef struct{
	u16 startAdc:1;	// 0:stop, 1:start
	u16	trigMode:2;	// 0:auto, 1:manual, 2:single
	u16 sideDisp:1;	// side menu displayed on screen
	u16 reserve:11;
	u16 debug:1;	// show debug info on somewhere
}sSIGNAL;

extern sSIGNAL m_signal;
//------------------------
typedef struct{
	u8 trigLv;	// 0.1 V unit
	u8 trigEdge;	// 0:falling, 1:rising
	eSCALE_T scaleT;	// grid period unit for table
	eSCALE_V scaleV;
	u8 reserved[32 -6];
}SETTINGS;

extern SETTINGS m_setting;
extern const u32 m_scaleTtbl[scaleT_INVALID];
extern const u8 m_scaleVtbl[scaleV_INVALID];

typedef struct{
	u16 type;
	u16 width;
	u16 height;
	union{
		u8 const *cTable;
		u16 const *nTable;
	}pFont;
}sFONT_TYPE;

u16 utilSoftCountGet(u8 eTimer);
void utilSoftCountSet(u8 eTimer, u16 nValue);
void lcdInitScreen(void);
void lcdUpdateMode(void);
void lcdUpdateStart(void);
#endif
