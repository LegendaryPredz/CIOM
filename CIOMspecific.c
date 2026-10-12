//__________________________________________________________________________
// CIOMspecific.c
//__________________________________________________________________________
// Copyright 2019 PlayAGS, Inc.
// http://www.playags.com
//
// Program Description:
//
// Source file dedicated to managing the IOM specific functions.
//
// Release 6.0
//    -Initial Revision
//    -24 September 2019
//
//__________________________________________________________________________

//_____  I N C L U D E S ___________________________________________________

#include "Delay.h"
#include "USB_API/USB_Common/types.h"           // Basic Type declarations
#include "USB_API/USB_Common/usb.h"             // USB-specific functions
#include <msp430.h>
#include <intrinsics.h>
#include <string.h>
#include <stdlib.h>

#include "OneWire/OneWire.h"

#include "usbConstructs.h"
#include "platform_common_defs.h"
#include "Inputs.h"
#include "CIOMspecific.h"
#include "SwitchCounter.h"
#include "main.h"


//_____ M A C R O S ________________________________________________________

//_____ D E F I N I T I O N S ______________________________________________

#define SIZE_LOOKUP_TBL       8
const uint32_t LEDOUT_LOOKUP[] =
{
    SPARE_BIT,          // 0
    SPARE_BIT,          // 1
    SPARE_BIT,          // 2
    SPARE_BIT,          // 3
    SPARE_BIT,          // 4
    SPARE_BIT,          // 5
    SPARE_BIT,          // 6
    SPARE_BIT,          // 7

    SPARE_BIT,          // 8
    SPARE_BIT,          // 9
    SPARE_BIT,          // 10
    SPARE_BIT,          // 11
    SPARE_BIT,          // 12
    PLAY_LAMP_BIT,      // 13
    SPARE_BIT,          // 14
    SPARE_BIT,          // 15
};


//_____ G L O B A L S ______________________________________________________

extern USHORT	FanCnt[];
extern USHORT	FanRPM[];
extern USHORT	FanStat[];
extern SHORT  	PrevTemp[];

BYTE        CabinetType;
USHORT      LastStatusCode;    // Keep track of the last status reply (i.e., success or error code)
uint32_t    Outputs;
ONEWIRE 	OWCtl[ NUM_1_WIRE_BUSSES];
FAN_CONTROL	FanCtl[ NUM_FANS];
FLASH_CTRL  Flashers[ MAX_LAMPS];
ULONG       tempCounts[ NUM_COUNTER_SWITCHES];  // for reporting button press


//-----------------------------------------------------------------------------
// CIOM_Init
//-----------------------------------------------------------------------------
//
// Return Value : none
// Parameters   : none
//
// This routine initializes the one-wire devices as well as sets up the PWM and
// RPM capture registers.
//
//-----------------------------------------------------------------------------
void CIOM_Init( void)
{
    BYTE    i;
	BYTE 	fanNum;
	BYTE	id[8];

	// Initialize Port 7 for our 1-wire devices which happen to be fans
	for( i = 0; i < NUM_FANS; i++)
	{
		OWCtl[i].port_out = &P7OUT;
		OWCtl[i].port_in = &P7IN;
		OWCtl[i].port_ren = &P7REN;
		OWCtl[i].port_dir = &P7DIR;
		OWCtl[i].pin = 1 << i;

		// Initialize to zero
		FanStat[ i] = FAN_MISSING;
		PrevTemp[ i] = 0;

		// Search the 1-wire buses to find the 1-wire devices.
        if( OneWire_ReadROM( &OWCtl[ i], id, 0, 1) < 0)
        {
            // The first temp sensor is optional but the others are required
            // so if they're not found flag an error
            if( i != OPTIONAL_FAN)
            {
                ErrorNumber = ERROR_ONE_WIRE_PROBLEM;
                ErrorCulprit = i;
            }
		}
		else
		{
			// Kick-off readings to "prime" the temp sensor
			OneWire_ReadTemp_DS1822( &OWCtl[ i]);
			FanStat[ i] = FAN_FOUND;
		}
	}

	// Setup the PWM for driving the fans. The SMCLK is running at
	// 25kHz.
	TA0CCR0 = PWM_PERIOD;
	TA0CCTL2 = OUTMOD_7;                      	// CCR2 reset/set
	TA0CCTL3 = OUTMOD_7;                      	// CCR3 reset/set
	TA0CCTL4 = OUTMOD_7;                      	// CCR4 reset/set
	TA0CTL = TASSEL_2 + MC_1 + TACLR;     		// SMCLK, up mode, clear TAR

	// Point to the Capture/Compare Registers for Timer_A
	FanCtl[ FAN_0].CCR = &TA0CCR2;
	FanCtl[ FAN_1].CCR = &TA0CCR3;
	FanCtl[ FAN_2].CCR = &TA0CCR4;

	// Point to the Capture/Compare Control Registers for Timer_B
	FanCtl[ FAN_0].CCTL = &TB0CCTL2;
	FanCtl[ FAN_1].CCTL = &TB0CCTL3;
	FanCtl[ FAN_2].CCTL = &TB0CCTL4;

	// Set the fans to run full on weather they are plugged in or not,
	// the runtime code will adjust the speeds of those attached.
	for( fanNum = 0; fanNum < NUM_FANS; fanNum++)
	{
		FanCnt[ fanNum] = 0;							// Set the counters to zero
		*FanCtl[ fanNum].CCR = FAN_FULL_ON;
		*FanCtl[ fanNum].CCTL = CM_1 + CAP + CCIS_0 + SCS + CCIE;	// Rising edge, Capture, CCIxA, Synchronous & Enable intr
	}

	// The the Capture/compare control register for: SMCLK, /1 and continuous mode
	TB0CTL = TBSSEL_2 + ID_0 + MC_2;


	// Make some measurements to test the health of the fans. Clear
	// the counter then wait for 2 seconds.
	SpinTimer = 0;
	while( SpinTimer < 2000l);

	// Take the count and multiply it by 30 to give RPM but since
	// we get 2-ticks per revolution we need to divide by 2.

	// Calculate the RPM, (cnt * 30)/2 or *15
	// The 30-> 2sec measure*30 = 1min
	// The /2-> 2 ticks per revolution
	for( fanNum = 0; fanNum < NUM_FANS; fanNum++)
	{
		if( FanStat[ fanNum] >= FAN_FOUND)
		{
			FanRPM[ fanNum] = FanCnt[ fanNum] * 15;
			FanCnt[ fanNum] = 0;

			// With the freshly measured fan values, make sure the fans are
			// within our valid speed ranges.
			if( (FanRPM[ fanNum] < FAN_MAX_SPEED_MIN) || (FanRPM[ fanNum] > FAN_MAX_SPEED_MAX))
			{
				BoardStatEvents |= EVENT_FAN_OUTOFBOUNDS;
			}
		}
	}

    memset( &Flashers, 0x00, sizeof( Flashers));

	// When non-zero the host as requested to take control of the
	// external fans.
	ExternalFanLevel = 0;

    // What type of cabinet were we before?
    CabinetType = NVRAM_START.CabinetType;

}


//-----------------------------------------------------------------------------
// ProcessCmd
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// The host is sending requests/commands for this device to act on.
//
//-----------------------------------------------------------------------------
void ProcessCmd( void)
{
    uint8_t i;
    union   MIXED_TYPES uTemp;
    ULONG   iValue;
    double  fValue;

    P4OUT ^= LED_USB_ACTIVITY;

    // Get the packet header information, copy it into the returning packet so
    // the host knows what command & sequence number is replying so we know that
    // the host and this app are in sync!
    Header.Cmd = GetNextRxByte();
    Header.SeqNum = GetNextRxByte();

    // Process the commands.
    switch( Header.Cmd)
    {
        case READ_VERSION_CMD:
            RTN_STATUS( LastStatusCode = SUCCESS);
            EnTxQueue( FW_VERSION_MAJOR);
            EnTxQueue( FW_VERSION_MINOR);
            break;

        case READ_ID_CMD:
            RTN_STATUS( LastStatusCode = SUCCESS);
            uTemp.lVal = BoardID;                // Contains the Macola number of the board
            for( i = 0; i < NUM_BYTES; ++i)
            {
                EnTxQueue( uTemp.bVal[i]);
            }
            break;

        case READ_RAW_INPUTS_CMD:
            RTN_STATUS( LastStatusCode = SUCCESS);
            GetRawInputs();
            break;

        case READ_EVENTS_CMD:
        case REARM_EVENTS_CMD:
        case READ_REARM_EVENTS_CMD:
            RTN_STATUS( LastStatusCode = SUCCESS);
            LoadEvents( Header.Cmd);
            break;

        case READ_SYS_HEALTH_CMD:
            RTN_STATUS( LastStatusCode = SUCCESS);
            EnTxQueue( (AvgVoltage & 0xFF));            // Returning a structure and let
            EnTxQueue( (AvgVoltage >> 8) & 0xFF);       // the otherside contend w/swapping

            // With the fan and temp sensors co-located to me it
            // only makes sense to store them together and it makes
            // for loading for transport much simpler.
            for( i = 0; i < NUM_FANS; i++)
            {
                EnTxQueue( (FanRPM[ i] & 0xFF));
                EnTxQueue( (FanRPM[ i] >> 8) & 0xFF);
                EnTxQueue( (PrevTemp[ i] & 0xFF));      // Since previous temp was current
                EnTxQueue( (PrevTemp[ i] >> 8) & 0xFF); // we'll treat it as close enough.
            }

            // Removed N2 parameters when we went to the larger power supplies
            for( i = 0; i < FORMER_N2_BYTES; i++)
            {
                EnTxQueue( 0);
            }
            break;

        // For testing purposes we want to be able to override the automatic fan control
        // built into the f/w but allowing external control via the API
        case EXTERNAL_FAN_CONTROL_CMD:
            RTN_STATUS( LastStatusCode = SUCCESS);
            ExternalFanLevel = GetNextRxByte();
            SpinTimer = SEC_10_0;       // Force a h/w check
            break;

        // Report back the battery voltage and temperatures but using fixed point math
        // with a 10^2 scaler.
        case READ_SYS_STATUS_CMD:
            RTN_STATUS( LastStatusCode = SUCCESS);

            // Sent the fixed point values first
            fValue = (double)AvgVoltage * ADC_VOLT_MULTIPLIER;
            iValue = (ULONG)(fValue * FIX_PT_SCALER);
            EnTxQueue( (iValue & 0xFF)); EnTxQueue( (iValue >> 8) & 0xFF); EnTxQueue( (iValue >> 16) & 0xFF); EnTxQueue( (iValue >> 24) & 0xFF);

            for(i = 0; i < NUM_FANS; i++)
            {
                fValue = (double)PrevTemp[ i] * ADC_TEMP_MULTIPLIER;
                iValue = (ULONG)(fValue * FIX_PT_SCALER);
                EnTxQueue( (iValue & 0xFF)); EnTxQueue( (iValue >> 8) & 0xFF); EnTxQueue( (iValue >> 16) & 0xFF); EnTxQueue( (iValue >> 24) & 0xFF);
            }

            // Now send the exponential factor, in this case they are all 10^2
            EnTxQueue( FIX_PT_EXPO_FACTOR);
            EnTxQueue( FIX_PT_EXPO_FACTOR);
            EnTxQueue( FIX_PT_EXPO_FACTOR);
            EnTxQueue( FIX_PT_EXPO_FACTOR);

            break;

        // Prior to Ontario we always used negative logic for intrusion detection,
        // as a default we still use negative logic unless the host tells us otherwise.
        // The IOM will be released with positive logic only. No change for the IDM.
        // NOTE: 4-March 2015 - The decision was made to ship IPIOCBs to Ontario
        // and the default behavior is now positive logic.
        // 16-May-2016 - Adding the ability to change intrusion logic for the IOM.
        case SET_INTRUSION_POLARITY:
            RTN_STATUS( LastStatusCode = SUCCESS);
            IntrusionLogic = GetNextRxByte();
            break;

        case GET_INTRUSION_POLARITY:
            RTN_STATUS( LastStatusCode = SUCCESS);
            EnTxQueue( IntrusionLogic);
            break;

        case CONFIG_GPIO:
            RTN_STATUS( LastStatusCode = SUCCESS);

            // For each bit 0=input and 1=output
            GpioDir = GetNextRxByte() & GPIO_MASK;
            P6DIR = (P6DIR & ~GPIO_MASK) + GpioDir;
            break;

        case SET_GPIO:
            RTN_STATUS( LastStatusCode = SUCCESS);
            // Since the GpioDir has 1 as output we'll mask the value
            // with that
            P6OUT = (P6DIR & ~GPIO_MASK) + (GetNextRxByte() & GpioDir);
            break;

        case GET_GPIO:
            RTN_STATUS( LastStatusCode = SUCCESS);
            EnTxQueue( P6IN & GPIO_MASK);
            break;

        case CONFIG_PLAY_BUTTON:
            RTN_STATUS( LastStatusCode = SUCCESS);
            if( GetNextRxByte())
            {
                GpioDir &= ~PLAY_SWITCH_BIT;            // Input
                GpioDir |= PLAY_LAMP_BIT;               // Output
                P6DIR = GpioDir & GPIO_MASK;
                P6REN |= PLAY_SWITCH_BIT;               // Enable internal resistance
                P6OUT |= PLAY_SWITCH_BIT;               // Set as pull-up resistance
                P6OUT &= ~PLAY_LAMP_BIT;                // Start with lamp off

                // Flag that we're driving play button lamp
                PlayButtonInUse = TRUE;
            }
            else
            {
                GpioDir |= PLAY_SWITCH_BIT;             // Output
                P6OUT &= ~PLAY_SWITCH_BIT;              // Set low
                P6OUT &= ~PLAY_LAMP_BIT;                // Also low
                PlayButtonInUse = FALSE;
            }
            break;

        case STATUS_PLAY_BUTTON:
            RTN_STATUS( LastStatusCode = SUCCESS);
            EnTxQueue( PlayButtonInUse);
            break;

        case CONFIG_CHARGER:
            RTN_STATUS( LastStatusCode = SUCCESS);
            if( GetNextRxByte())
            {
                GpioDir |= WIRELESS_CHARGER_BIT;        // Output
                P6OUT   |= WIRELESS_CHARGER_BIT;        // Set hi
                WirelessChargeInUse = TRUE;
            }
            else
            {
                P6OUT  &= ~WIRELESS_CHARGER_BIT;        // Set low
                WirelessChargeInUse = FALSE;
            }
            break;

        case STATUS_CHARGER:
            RTN_STATUS( LastStatusCode = SUCCESS);
            EnTxQueue( WirelessChargeInUse);
            break;

        case BACKPLANE_DETECT:
            RTN_STATUS( LastStatusCode = SUCCESS);
            EnTxQueue( ((P4IN & BACKPLANE_DETECT_MASK) ? 1 : 0));
            break;

        case MECH_BELL_PLAY:
            RTN_STATUS( LastStatusCode = SUCCESS);

            // Clear the count-up counters
            MechBellOnCnt = 0;
            MechBellFreqCnt = 0;
            MechBellPeriodCnt = 0;
            MechBellTimesCnt = 0;
            MechBellDelayCnt = 0;

            // Need to make sure the GPIO is set for output and ring my bell
            P6DIR |= MECH_BELL;
            P6OUT |= MECH_BELL;

            // Loading the values starts the ISR driving the bell
            MechBellPeriod = (GetNextRxByte() << 8) | GetNextRxByte();
            MechBellFreq   = (GetNextRxByte() << 8) | GetNextRxByte();
            MechBellOn     = (GetNextRxByte() << 8) | GetNextRxByte();

            // Verify the parameters being passed in are kosher
            MechBellPeriod = MIN( MechBellPeriod, MAX_BELL_PERIOD);
            MechBellFreq   = MIN( MechBellFreq, (MAX_BELL_PERIOD >> 1));
            MechBellOn     = MIN( MechBellOn, MAX_BELL_ON);

            // Request from OS
            if( MechBellFreq ==  0)
            {
                MechBellFreq = MechBellParms[0][BELL_FREQ];
            }

            MechBellTimes = 1;
            MechBellDelay = 0;
            break;

        case MECH_BELL_PLAY_SEQ:
            // Grab the sequence number to play
            i = GetNextRxByte();
            if( i < BELL_MAX_SEQ_ENTRIES)
            {
                RTN_STATUS( LastStatusCode = SUCCESS);

                // Clear the count-up counters
                MechBellOnCnt = 0;
                MechBellFreqCnt = 0;
                MechBellPeriodCnt = 0;
                MechBellTimesCnt = 0;
                MechBellDelayCnt = 0;

                // Need to make sure the GPIO is set for output and ring my bell
                P6DIR |= MECH_BELL;
                P6OUT |= MECH_BELL;

                // Loading the values starts the ISR driving the bell
                MechBellPeriod = MechBellParms[i][BELL_PERIOD];
                MechBellFreq   = MechBellParms[i][BELL_FREQ];
                MechBellOn     = MechBellParms[i][BELL_ON];
                MechBellTimes  = MechBellParms[i][BELL_TIMES];
                MechBellDelay  = MechBellParms[i][BELL_DELAY];
            }
            else
            {
                RTN_STATUS( LastStatusCode = ERROR_INVALID_NUMBER);
            }
            break;

        case MECH_BELL_SET_SEQ:
            // Grab the sequence number to play
            i = GetNextRxByte();
            if( i < BELL_MAX_SEQ_ENTRIES)
            {
                RTN_STATUS( LastStatusCode = SUCCESS);

                MechBellParms[i][BELL_PERIOD] = (GetNextRxByte() << 8) | GetNextRxByte();
                MechBellParms[i][BELL_FREQ]   = (GetNextRxByte() << 8) | GetNextRxByte();
                MechBellParms[i][BELL_ON]     = (GetNextRxByte() << 8) | GetNextRxByte();
                MechBellParms[i][BELL_TIMES]  = (GetNextRxByte() << 8) | GetNextRxByte();
                MechBellParms[i][BELL_DELAY]  = (GetNextRxByte() << 8) | GetNextRxByte();
            }
            else
            {
                RTN_STATUS( LastStatusCode = ERROR_INVALID_NUMBER);
            }
            break;

        case UPDATE_FIRMWARE_CMD:
            RTN_STATUS( LastStatusCode = SUCCESS);
            PDSignature=0;                      // Trash the signature to force a full initialization

            // We must disconnect from the USB and disable general interrupts. This is
            // per Application Report SLAA452B, section 4.3
            USB_disconnect();
            USB_disable();

            __disable_interrupt();              // Ensure no application interrupts fire during BSL
            ((void (*)())0x1000)();             // This sends execution to the BSL. When execution
                                                // returns to the user app, it will be via the reset
                                                // vector, meaning execution will re-start.
            break;

        case RETRIEVE_RESPONSE_CMD:
            RTN_STATUS( SUCCESS);
            EnTxQueue( (uint8_t)(LastStatusCode >> 8) & 0x00FF);
            EnTxQueue( (uint8_t)(LastStatusCode & 0x00FF));

            // Need to track this too.
            LastStatusCode = SUCCESS;
            break;

        case SHUT_DOWN_CMD:
            RTN_STATUS( LastStatusCode = SUCCESS);
            Schedule = SHUT_DOWN_CMD;
            break;

        // Toggle GPIO_SPARE 4 pin
        case POWER_CYCLE_CMD:
            RTN_STATUS( LastStatusCode = SUCCESS);
            Schedule = POWER_CYCLE_CMD;
            break;

        case GET_SERIAL_NUMBER_CMD:
            RTN_STATUS( LastStatusCode = SUCCESS);
            EnTxQueue( DIE_ID_LENGTH);
            for( i = 0; i < DIE_ID_LENGTH; ++i)
            {
                EnTxQueue( SerialNumber[i]);
            }
            break;

        case READ_PRESS_COUNTS_CMD:
            RTN_STATUS( LastStatusCode = SUCCESS);
            Counter_GetAllCounts(tempCounts);
            for( i = 0; i < NUM_COUNTER_SWITCHES; i++)
            {
                EnTxQueue( (tempCounts[i] >> 0) & 0xFF);
                EnTxQueue( (tempCounts[i] >> 8) & 0xFF);
                EnTxQueue( (tempCounts[i] >> 16) & 0xFF);
                EnTxQueue( (tempCounts[i] >> 24) & 0xFF);
            }
            break;

        case RESET_PRESS_COUNTS_CMD:
            RTN_STATUS( LastStatusCode = SUCCESS);
            Counter_ResetAll();
            break;


		//
		// Lamp commands
		//
		case LAMP_CONTROL_CMD:
            RTN_STATUS( LastStatusCode = SUCCESS);
			LampControl();
			break;

		case SET_LAMP_STATE_CMD:
            RTN_STATUS( LastStatusCode = SUCCESS);
            SetupLampState();
            break;

		case FLASH_LAMP_CMD:
            RTN_STATUS( LastStatusCode = SUCCESS);
	        SetupFlashLamps();
	        break;

        case GET_CABINET_TYPE:
            RTN_STATUS( LastStatusCode = SUCCESS);
            EnTxQueue( CabinetType);
            break;

        case SET_CABINET_TYPE:
			i = GetNextRxByte();
            if( (i >= NEXTGEN_SLANT49) && (i < NUM_OF_CABINETS))
			{
				// Convert to local cabinet type number 0 being unknown and
				// starting at 1 the Slant 49 cabinet.
				i = (i - NEXTGEN_SLANT49) + 1;

				// Reset sequence table if different and update sequence header
				if( i != CabinetType)
				{
					CabinetType = i;

					// We want to save this in flash so we remember on next bootup
                    TempStorage.CabinetType = CabinetType;
					UpdateNvRam();
				}

                RTN_STATUS( LastStatusCode = SUCCESS);
            }
            else
            {
                RTN_STATUS( ERROR_INVALID_NUMBER);
            }
            break;

        default:
            RTN_STATUS( LastStatusCode = ERROR_INVALID_CMD);
            break;
    }

    // Send packet back to the host and reset the pointers for the next packet
    SendTxQueue();
    _InitRxQueue;
}


//-----------------------------------------------------------------------------
// CIOM_specific
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This routine handles the hardware specific routines required as part of the
// "main: while loop" related specifically to the CIOM board.
//
//-----------------------------------------------------------------------------
void CIOM_Specific( void)
{
    BYTE    fanNum;
    SHORT   CurrTemp;
    USHORT  ExtLevel;

    // Use the free-spinning timer counter to track the
    // once a minute stuff
    if( SpinTimer >= SEC_10_0)
    {
        SpinTimer = 0;
        BatteryMonitor();

        // Read the different temperature sensors and adjust the fans
        // accordingly - "FansFound" should be "TemperatureSensorsFound"
        // but since we're adjusting fans it was named as it is.
        TA0CCR0 = 0;                            // Stop timer to set new duty cycle
        for( fanNum = 0; fanNum < NUM_FANS; fanNum++)
        {
            if( FanStat[ fanNum] >= FAN_FOUND)
            {
                CurrTemp = ReadTemp( fanNum);

                // Error reading device
                if( CurrTemp == -1)
                {
                    ErrorCulprit = fanNum;
                    CurrTemp = 0;
                    *FanCtl[ fanNum].CCR = FAN_FULL_ON;
                    PrevTemp[ fanNum] = CurrTemp;
                }

                // We're too hot so flag an error and run the fan at full speed!
                else if( CurrTemp > TEMP_HI_THRESHOLD)
                {
                    // This is our initial reading and it's not valid so ignore
                    // whatever the temp sensor is passing out...
                    if( PrevTemp[ fanNum] != 0)
                    {
                        ErrorNumber = ERROR_OVER_TEMP_LIMIT;
                        ErrorCulprit = fanNum;
                        *FanCtl[ fanNum].CCR = FAN_FULL_ON;
                        BoardStatEvents |= EVENT_TEMP_OUTOFBOUNDS;
                    }
                }

                // When non-zero set the fan speed to what the host passed in
                else if( ExternalFanLevel != 0)
                {
                    // Scale so we go from 0 to 10; at 100% the result is 0xA0 so
                    // subtract 1 to give (0x9F - 0x9F) = 0
                    ExtLevel = ExternalFanLevel / 10;
                    ExtLevel = FAN_10_PERCENT_STEP * ExtLevel;
                    *FanCtl[ fanNum].CCR = ExtLevel;
                }

                // We're below the high threshold so let's adjust the fan to
                // maintain a constant temp
                else
                {
                    PrevTemp[ fanNum] = CurrTemp;

                    // Anything below 20C we'll leave at idle
                    if( CurrTemp < TEMP_20)
                    {
                        *FanCtl[ fanNum].CCR = FAN_30_PERCENT;
                    }

                    // Reuse the variable CurrTemp to calculate a new fan speed
                    else
                    {
                        CurrTemp = (CurrTemp - TEMP_20) / TEMP_2_5_DEGREE_STEPS;
                        *FanCtl[ fanNum].CCR = FAN_30_PERCENT + (FAN_2_5_PERCENT_STEP * CurrTemp);
                    }
                }
            }
        }

        // Restart the PWM
        TA0CCR0 = PWM_PERIOD;

        // Calculate the RPM, (cnt * 6)/2 or *3
        // The 6-> 10sec measure*6 = 1min
        // The /2-> 2 ticks per revolution
        for( fanNum = 0; fanNum < NUM_FANS; fanNum++)
        {
            if( FanStat[ fanNum] >= FAN_FOUND)
            {
                FanRPM[ fanNum] = FanCnt[ fanNum] * 3;
                FanCnt[ fanNum] = 0;

                // Check to make sure the fans are running within their
                // designed limits.

                // I'm trying to use math rather than a table to detect a fan
                // failure.  Using the PWM and scale factor to calculate lower
                // fan limits.
                if( *FanCtl[ fanNum].CCR >= FAN_30_PERCENT)         // looking at PWM
                {
                    if( FanRPM[ fanNum] < FAN_IDLE_SPEED_MIN)       // looking at RPM
                    {
                        ErrorCulprit = fanNum;
                        ErrorNumber = ERROR_FAN_PROBLEM;
                        BoardStatEvents |= EVENT_FAN_OUTOFBOUNDS;
                    }
                }
                else if( *FanCtl[ fanNum].CCR > FAN_50_PERCENT)
                {
                    // The "14" is from scale factor found in the spread sheet
                    // "Fan Speed Measurement IOM.xlsx" file
                    if( FanRPM[ fanNum] < (FAN_MAX_SPEED_MIN - (*FanCtl[ fanNum].CCR * 14)))
                    {
                        ErrorCulprit = fanNum;
                        ErrorNumber = ERROR_FAN_PROBLEM;
                        BoardStatEvents |= EVENT_FAN_OUTOFBOUNDS;
                    }
                }
                // The "25" is from scale factor found in the spread sheet
                // "Fan Speed Measurement IOM.xlsx" file
                else if( FanRPM[ fanNum] < (FAN_MAX_SPEED_MIN - (*FanCtl[ fanNum].CCR * 25)))
                {
                    ErrorCulprit = fanNum;
                    ErrorNumber = ERROR_FAN_PROBLEM;
                    BoardStatEvents |= EVENT_FAN_OUTOFBOUNDS;
                }
                else if( FanRPM[ fanNum] > FAN_MAX_SPEED_MAX)
                {
                    ErrorCulprit = fanNum;
                    ErrorNumber = ERROR_FAN_PROBLEM;
                    BoardStatEvents |= EVENT_FAN_OUTOFBOUNDS;
                }
            }
        }
    }

    // Used if the CIOM is driving the play button lamp 
    if( PlayButtonInUse)
    {
        UpdateLamps();
    }
}


//-----------------------------------------------------------------------------
// ReadTemp
//-----------------------------------------------------------------------------
//
// Return Value : temperature read back from sensor
// Parameters   : device - specific temp sensor to read
//
// This routine makes the calls necessary to read the Maxim DS1822 temperature
// sensor and returns that value to the calling routine.
//
//-----------------------------------------------------------------------------
SHORT ReadTemp( BYTE device)
{
    SHORT Temp;

    // Store current interrupt state, then disable all interrupts
    uint16_t globalInterruptState = __get_SR_register() & GIE;
    __disable_interrupt();

    Temp = OneWire_ReadTemp_DS1822( &OWCtl[ device]);
    if( Temp < 0)
    {
        // Try again
        Temp = OneWire_ReadTemp_DS1822( &OWCtl[ device]);
        if( Temp < 0)
        {
            ErrorNumber = ERROR_FAN_READ_PROBLEM;
            ErrorCulprit = device;
            Temp = -1;
        }
    }

    // Restore previous interrupt state
    __bis_SR_register(globalInterruptState);

    return( Temp);
}


//-----------------------------------------------------------------------------
// LampControl
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Turn off or on the lamps based on mask and state passed in.
//
//-----------------------------------------------------------------------------
void LampControl( void)
{
    uint8_t  State;
    uint16_t Idx;
    uint32_t tmpGame;
    union MIXED_TYPES Tmp;

    // Non-zero means the lamp gets illuminated
    State = GetNextRxByte();

    // Pull the enable mask off the USB queue
    for( Idx = 0; Idx < NUM_BYTES; ++Idx)
    {
        Tmp.bVal[ Idx] = GetNextRxByte();
    }

    tmpGame = Tmp.lVal & CIOM_OUTPUTS_MASK;

    // Loop through all the output related bits and set them accordingly
    for( Idx = 0; Idx < MAX_LAMPS; Idx++)
    {
        // 'tmpGame' is a mask used for setting or clearing based on
        // the contents of 'state'
        if( tmpGame & LSB_LONG)
        {
            if( State)
            {
                Outputs &= ~(1l << Idx);
                Flashers[ Idx].Flash = 0;
            }
            else
            {
                // Turn it off as well as the flasher
                Outputs |= (1l << Idx);
            }
        }

        // Shift the mask up
        tmpGame >>= 1;
    }
}


//-----------------------------------------------------------------------------
// SetupLampState
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Turn off or on the position represented by the lamp value sent down
// from the host.
//
//-----------------------------------------------------------------------------
void SetupLampState( void)
{
    uint16_t Idx;
    uint32_t tmpGame;
    union MIXED_TYPES Tmp;

    // Pull the enable mask off the USB queue
    for( Idx = 0; Idx < NUM_BYTES; ++Idx)
    {
        Tmp.bVal[ Idx] = GetNextRxByte();
    }

    // Loop through all the output related bits and set them accordingly
    tmpGame = Tmp.lVal;
    for( Idx = 0; Idx < MAX_LAMPS; Idx++)
    {
        // If lamp bit is enabled, turn it on
        // the contents of 'state'
        if( tmpGame & LSB_LONG)
        {
            Outputs &= ~(1l << Idx);
            Flashers[ Idx].Flash = 0;
        }
        else
        {
            // Turn it off as well as the flasher
            Outputs |= (1l << Idx);
        }

        // Shift the mask up
        tmpGame >>= 1;
    }
}


//-----------------------------------------------------------------------------
// SetupFlashLamps
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Setup the lamps for flashing.
//
//-----------------------------------------------------------------------------
void SetupFlashLamps( void)
{
    uint16_t Idx;
    uint16_t OnTime, OffTime;
    uint32_t tmpGame;
    union MIXED_TYPES Tmp;

    // Pull the data off the USB queue
    // Pull the enable mask off the USB queue
    for( Idx = 0; Idx < NUM_BYTES; ++Idx)
    {
        Tmp.bVal[ Idx] = GetNextRxByte();
    }

    OnTime  = GetNextRxByte() | (GetNextRxByte() << 8);
    OffTime = GetNextRxByte() | (GetNextRxByte() << 8);

    // Loop through all the output related bits and set them accordingly
    tmpGame = Tmp.lVal;
    for( Idx = 0; Idx < MAX_LAMPS; Idx++)
    {
        // Enable flashing where defined
        if( tmpGame & LSB_LONG)
        {
            Flashers[ Idx].Flash   = 1;
            Flashers[ Idx].OnTime  = OnTime;
            Flashers[ Idx].OffTime = OffTime;
            Flashers[ Idx].Timer   = 0;
        }

        // Shift the mask up
        tmpGame >>= 1;
    }
}


//-----------------------------------------------------------------------------
// UpdateLamps
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This routine drives the play button lamp
//
//-----------------------------------------------------------------------------
void UpdateLamps( void)
{
    uint16_t Idx;
    uint32_t TblVal;
    uint32_t tmpOutput;

    // "Outputs" is filled in SetupLampState and LampControl.
    tmpOutput = Outputs;
    for( Idx = 0; Idx < MAX_LAMPS; Idx++)
    {
        TblVal = LEDOUT_LOOKUP[ Idx];
        if( TblVal != SPARE_BIT)
        {
            // Toggle lamps as needed.
            if( Flashers[ Idx].Flash != 0)
            {
                if( Flashers[ Idx].Timer >= Flashers[ Idx].OffTime)
                {
                    // Turn on
                    P6OUT &= ~TblVal;
                }
                else
                {
                    // Turn off
                    P6OUT |= TblVal;
                }

                if( Flashers[ Idx].Timer >= (Flashers[ Idx].OffTime + Flashers[ Idx].OnTime))
                {
                    // Turn off
                    P6OUT |= TblVal;
                    Flashers[ Idx].Timer = 0;
                }
            }
            else
            {
                // 'tmpOutput' is a mask used for setting or clearing the LED
                if( tmpOutput & LSB_LONG)
                {
                    // Turn on
                    P6OUT &= ~TblVal;
                }
                else
                {
                    // Turn off
                    P6OUT |= TblVal;
                }
            }
        }

        // Shift the mask up
        tmpOutput >>= 1;
    }
}
