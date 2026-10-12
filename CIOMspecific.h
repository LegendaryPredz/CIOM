//__________________________________________________________________________
//	CIOMspecific.h
//__________________________________________________________________________
// Copyright 2019 PlayAGS, Inc.
// http://www.playags.com
//
// Program Description:
//
// Header file used to define function calls and data structures used to
// drive the IOM board.
//
// Release 6.0
//    -Initial Revision
//    -24 September 2019
//
//__________________________________________________________________________


#ifndef CIOM_SPECIFIC_H_
#define CIOM_SPECIFIC_H_


//_____  I N C L U D E S ___________________________________________________

//_____ D E F I N I T I O N S ______________________________________________

typedef struct fancontrol_t
{
    volatile unsigned int *CCR;
    volatile unsigned int *CCTL;
} FAN_CONTROL;


#define TEMP_2_5_DEGREE_STEPS   0x0028                  // 2.5 degree steps
#define TEMP_5_DEGREE_STEPS		0x0050					// 5 degree steps
#define TEMP_LOW_THRESHOLD		0x0140					// 20 Degrees celsius
#define TEMP_20					(TEMP_LOW_THRESHOLD)
#define TEMP_25					(TEMP_LOW_THRESHOLD + TEMP_5_DEGREE_STEPS
#define TEMP_30					(TEMP_LOW_THRESHOLD + (TEMP_5_DEGREE_STEPS * 2))
#define TEMP_35					(TEMP_LOW_THRESHOLD + (TEMP_5_DEGREE_STEPS * 3))
#define TEMP_40					(TEMP_LOW_THRESHOLD + (TEMP_5_DEGREE_STEPS * 4))
#define TEMP_45					(TEMP_LOW_THRESHOLD + (TEMP_5_DEGREE_STEPS * 5))
#define TEMP_HI_THRESHOLD		TEMP_45

#define PWM_PERIOD				0x009F					// Fan wants a 25kHz period
#define FAN_FULL_ON				PWM_PERIOD
#define FAN_IDLE				0
#define FAN_2_5_PERCENT_STEP    0x04
#define FAN_5_PERCENT_STEP      0x08
#define FAN_10_PERCENT_STEP     0x10

#define FAN_10_PERCENT			(FAN_10_PERCENT_STEP)
#define FAN_20_PERCENT			(FAN_10_PERCENT_STEP * 2)
#define FAN_30_PERCENT			(FAN_10_PERCENT_STEP * 3)
#define FAN_40_PERCENT			(FAN_10_PERCENT_STEP * 4)
#define FAN_50_PERCENT			(FAN_10_PERCENT_STEP * 5)

#define FAN_ADJUSTMENT_DWN		8						// Adjust in bigger steps going
#define FAN_ADJUSTMENT_UP		8						// increase fan speed
#define SOME_HYSTERESIS			0x02			        // 1 degree of hysteresis so smooth things out

#define FAN_IDLE_SPEED			(600)					// Imperially measured is ~600
#define FAN_50_PERCENT_SPEED	(1100)					// Imperially measured is ~1100
#define FAN_MAX_SPEED			(3100)					// Imperially measured is ~3100
#define FAN_IDLE_SPEED_MIN		(FAN_IDLE_SPEED - 90)
#define FAN_IDLE_SPEED_MAX		(FAN_IDLE_SPEED + 90)   // 90 = FAN_IDLE_SPEED*0.15
#define FAN_MAX_SPEED_MIN		(FAN_MAX_SPEED - 465)
#define FAN_MAX_SPEED_MAX		(FAN_MAX_SPEED + 465)   // 465 = FAN_MAX_SPEED*0.15

#define CURRENT_120W			246						// N2Power 120watt value

#define FAN_0					0
#define FAN_1					1
#define FAN_2					2
#define NUM_FANS				3
#define NUM_1_WIRE_BUSSES		3
#define OPTIONAL_FAN            FAN_0                   // Not required to be present

#define FAN_MISSING				0
#define FAN_FOUND				1
#define FAN_ISSUE				2
#define FAN_ISSUE_FLASH_DELAY	6

#define MIN(a,b)                ((a) < (b) ? (a) : (b))

#define MAX_BELL_PERIOD         100                     // No more that 100 millsecs
#define MAX_BELL_ON             5000                    // No more that 5 seconds

// Gameside bitmappings
#define CIOM_OUTPUTS_MASK       (0x2000)                // Play/spin lamp
#define MAX_LAMPS               16                      // Really on 1 lamp

// Make sure this order matches platform_common_defs.h
#define CAB_TYPE_UNKNOWN        0
#define CAB_TYPE_SLANT_49		1
#define CAB_TYPE_UPRIGHT_43		2
#define CAB_TYPE_UPRIGHT_49		3
#define CAB_TYPE_SLANT_43		4
#define CAB_TYPE_SINGER			5
#define CAB_TYPE_BIG_RED		6
#define CAB_TYPE_SPECTRA_UR49C	7			// Duplicate of #3 above
#define CAB_TYPE_SL75			8
#define CAB_TYPE_SINGER_PREM	9
#define CAB_TYPE_NU27D          10
#define CAB_TYPE_NS65D          11
#define CAB_TYPE_ST28P          12
#define CAB_TYPE_ST43P          13
#define CAB_TYPE_SR28P_MRT      14
#define CAB_TYPE_GEN5_UR43		15
#define CAB_TYPE_GEN5_UR55		16
#define CAB_TYPE_GEN5_S55		17
#define CAB_TYPE_DEV_BRD		255
#define CAB_LAST_DEFINED		CAB_TYPE_GEN5_S55

typedef struct __attribute__((packed)) FLASH_CTRL_t
{
    BYTE   Flash;
    USHORT OnTime;
    USHORT OffTime;
    USHORT Timer;
} FLASH_CTRL;


//_____ D E C L A R A T I O N S ____________________________________________

extern void   CIOM_Init( void);
extern void   CIOM_Specific( void);
extern void   ProcessCmd( void);
extern SHORT  ReadTemp( BYTE device);
extern void   LampControl( void);
extern void   SetupLampState( void);
extern void   SetupFlashLamps( void);
extern void   UpdateLamps( void);


#endif /* CIOM_SPECIFIC_H_ */
