//__________________________________________________________________________
//	Inputs.h
//__________________________________________________________________________
// Copyright 2019 PlayAGS, Inc.
// http://www.playags.com
//
// Program Description:
//
// Inputs header file for the Integrated USB I/O Peripheral Board.
//
// Release 6.0
//    -Initial Revision
//    -24 September 2019

//
//__________________________________________________________________________

#ifndef INPUTS_H_
#define INPUTS_H_


//_____  I N C L U D E S ___________________________________________________


//_____ D E F I N I T I O N S ______________________________________________

#define SPARE_BIT				0xFFFFFFFF

// Input bit definitions
#define INPUT_00               	0x00000001l
#define INPUT_01               	0x00000002l
#define INPUT_02               	0x00000004l
#define INPUT_03                0x00000008l
#define INPUT_04                0x00000010l
#define INPUT_05                0x00000020l
#define INPUT_06                0x00000040l
#define INPUT_07                0x00000080l
#define INPUT_08                0x00000100l
#define INPUT_09                0x00000200l
#define INPUT_10                0x00000400l
#define INPUT_11                0x00000800l
#define INPUT_12                0x00001000l
#define INPUT_13                0x00002000l
#define INPUT_14                0x00004000l
#define INPUT_15                0x00008000l
#define INPUT_16                0x00010000l
#define INPUT_17                0x00020000l
#define INPUT_18                0x00040000l
#define INPUT_19                0x00080000l
#define INPUT_20                0x00100000l
#define INPUT_21                0x00200000l
#define INPUT_22                0x00400000l
#define INPUT_23                0x00800000l
#define INPUT_24                0x01000000l
#define INPUT_25                0x02000000l
#define INPUT_26                0x04000000l
#define INPUT_27                0x08000000l
#define INPUT_28                0x10000000l
#define INPUT_29                0x20000000l
#define INPUT_30                0x40000000l
#define INPUT_31                0x80000000l

// Gameside input bitmappings
#define BOTTOM_SWITCH5			INPUT_00
#define BOTTOM_SWITCH4			INPUT_01
#define BOTTOM_SWITCH3			INPUT_02
#define BOTTOM_SWITCH2			INPUT_03
#define BOTTOM_SWITCH1			INPUT_04
#define MENU_SWITCH				INPUT_05
#define CLEAR_ERROR_SWITCH		INPUT_06
#define ATTEN_SWITCH			INPUT_07
#define COLLECT_SWITCH			INPUT_08
#define MAX_BET_SWITCH			INPUT_09
#define PLAY_SWITCH				INPUT_10
#define TOP_SWITCH5				INPUT_11
#define TOP_SWITCH4				INPUT_12
#define TOP_SWITCH3				INPUT_13
#define TOP_SWITCH2				INPUT_14
#define TOP_SWITCH1				INPUT_15
#define SPARE_KEY_SWITCH 		INPUT_16
#define BET_MAX_SWITCH_L 		INPUT_17	// Place holder
#define PLAY_SWITCH_L 			INPUT_18	// Place holder
#define ANTE_BET_SWITCH_L 		INPUT_19	// Place holder
#define ANTE_BET_SWITCH			INPUT_20
#define HEADPHONE_SWITCH 		INPUT_21

// Gameside intrusion bitmappings
// If changes are made, make sure to update the lookup section in
// "Inputs.c".
#define MAIN_DOOR				INPUT_02
#define LOGIC_DOOR				INPUT_03
#define BELLY_DOOR				INPUT_00
#define CASH_DOOR				INPUT_01
#define SPARE_DOOR1				INPUT_04
#define CABLE_PRESENT			INPUT_05
#define SPARE_DOOR2				INPUT_06

// Boardside intrusion bitmappings
#define BOARD_MAIN_DOOR			0x01
#define BOARD_LOGIC_DOOR		0x02
#define BOARD_BELLY_DOOR		0x04
#define BOARD_CASH_DOOR			0x08
#define BOARD_SPARE1_DOOR		0x10
#define BOARD_SPARE2_DOOR		0x20
#define BOARD_CABLE_PRESENT     0x40

#define SIZE_HISTORY 			32
#define SIZE_INTRU_HISTORY		8

// Board status events
#define EVENT_BATTERY_LOW		0x01		// Battery level low
#define EVENT_METER_ROLLOVER	0x02		// (Doesn't appear to be used)
#define EVENT_USB_DETECTED		0x04		// USB detected
#define EVENT_FAN_OUTOFBOUNDS	0x08		// Fan is running outside normal envelop
#define EVENT_TEMP_OUTOFBOUNDS	0x10		// Temperature       "
#define EVENT_WATCH_DOG			0x20
#define EVENT_RAM_CLEAR			0x40		// CODE_SIGNATURE_MISMATCH - I/O board memory was cleared
#define EVENT_USER_INIT         0x80        // Was when dip switch was set on old USBIO

// Input bit definitions
#define RED_KEY_SW              (BIT2)
#define GREEN_KEY_SW            (BIT3)
#define MENU2_KEY_SW            (BIT4)
#define KEY_MASK                (RED_KEY_SW | GREEN_KEY_SW | MENU2_KEY_SW)
#define INPUTS_MASK             (MENU_SWITCH | SPARE_KEY_SWITCH| CLEAR_ERROR_SWITCH | PLAY_SWITCH)
#define PLAY_MASK               (PLAY_SWITCH_BIT)
#define INTRUSION_MASK          (0x5F)      // No longer looking for the main (deck) Mar2022; was (0x7F)
#define KEY_SHIFT               2           // Move the bits down for easier processing
#define PLAY_SHIFT              1           // Move the bit left of the key bits
#define PLAY_CTR_INDX           3           // Index to report the play/spin counter presses
#define NUM_SWITCHES			4


//_____ D E C L A R A T I O N S ____________________________________________

extern union MIXED_TYPES SwitchSet;
extern union MIXED_TYPES SwitchClr;

extern volatile BYTE PowerOffEvents;

extern BYTE		PowerEvents;
extern BYTE 	PowerOnEvents;
extern BYTE 	BoardStatEvents;
extern BYTE		Intrusions;
extern BYTE 	LoadIndex;
extern BYTE 	ReadIndex;
extern BYTE 	ReadIntruIndex;
extern BYTE 	LoadIntruIndex;
extern BYTE		PrevIntrusions;
extern BYTE     LastReportedIntrusion;
extern ULONG	LastReported;
extern ULONG	PrevValue;
extern ULONG	ChangeLoc[];
extern ULONG	ChangeValue[];

extern void  InitInputs( void);
extern void	 GetRawInputs( void);
extern ULONG FindChangedInputs( ULONG NewValue);
extern BYTE  FindChangedIntrusions( BYTE NewValue);
extern void  ReadInputs( void);
extern void  ProcessInputs( void);
extern void  ProcessIntrusion( void);
extern BYTE  TranslateIntrusion( BYTE Incoming);
extern void  CheckEvents( void);
extern void  LoadEvents( BYTE cmd);


//_____ M A C R O S ________________________________________________________

#define IncReadIndex() 		{ReadIndex++;if(ReadIndex>=SIZE_HISTORY)ReadIndex=0;}
#define IncLoadIndex() 		{LoadIndex++;if(LoadIndex>=SIZE_HISTORY)LoadIndex=0;}
#define IncReadIntruIndex() {ReadIntruIndex++;if(ReadIntruIndex>=SIZE_INTRU_HISTORY)ReadIntruIndex=0;}
#define IncLoadIntruIndex() {LoadIntruIndex++;if(LoadIntruIndex>=SIZE_INTRU_HISTORY)LoadIntruIndex=0;}



#endif /*INPUTS_H_*/
