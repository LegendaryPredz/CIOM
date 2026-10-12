//__________________________________________________________________________
// Input.c
//__________________________________________________________________________
// Copyright 2019 PlayAGS, Inc.
// http://www.playags.com
//
// Program Description:
//
// Source file dedicated to managing inputs to the IOB 
//
// Release 6.0
//    -Initial Revision
//    -24 September 2019
//
//__________________________________________________________________________ 

//_____  I N C L U D E S ___________________________________________________

#include "USB_API/USB_Common/types.h"          // Basic Type declarations
#include <msp430.h>
#include <intrinsics.h>
#include <string.h>

#include "usbConstructs.h"
#include "Inputs.h"
#include "SwitchCounter.h"
#include "main.h"
#include "usb_api/USB_Common/usb.h"					///tedo

//_____ D E F I N I T I O N S ______________________________________________

const ULONG INPUT_LOOKUP[] =
{
	INPUT_05,		// Menu (Red) switch
	INPUT_06,		// Clear (Green) switch
	INPUT_16, 		// Blue key switch
	PLAY_SWITCH,	// Play/spin switch
};

// If changes are made, make sure to update the defines in "Inputs.h"
// in the intrusion section.
const BYTE INTRUSION_LOOKUP[] =
{
	INPUT_02,		// Main door switch
	INPUT_03,		// Logic door switch
	INPUT_00,		// Belly door switch
	INPUT_01,		// Cash door switch
	INPUT_04,		// Spare intrusion switch
	INPUT_06,		// Spare intrusion switch
	INPUT_05,		// Cable detect
};


//_____ G L O B A L S ______________________________________________________

volatile BYTE PowerOffEvents;

BYTE	PrevIntrusions;
BYTE    LastReportedIntrusion;
BYTE	ChangeIntruLoc[ SIZE_INTRU_HISTORY];
BYTE	ChangeIntruValue[ SIZE_INTRU_HISTORY];
BYTE 	ReadIndex;			// Index used to track processing (reading) of input buffer.
BYTE 	LoadIndex;			// Index used to track loading changes into input buffer.
BYTE 	ReadIntruIndex;		// Index used to track processing (reading) of intrusion buffer.
BYTE 	LoadIntruIndex;		// Index used to track loading changes into intrusion buffer.
BYTE	PowerEvents;
BYTE 	PowerOnEvents;
BYTE    BoardStatEvents;
BYTE	Intrusions;
BYTE	IntrusionSet;
BYTE	IntrusionClr;
BYTE	FirstTime = 1;

ULONG   Inputs;
ULONG   LastReported;
ULONG   PrevValue;
ULONG   ChangeLoc[ SIZE_HISTORY];
ULONG   ChangeValue[ SIZE_HISTORY];

// Time-based debouncing variables (matching PDCM implementation)
ULONG   buttonPressStartTime[NUM_SWITCHES];    // When each button press started
ULONG   buttonReleaseStartTime[NUM_SWITCHES];  // When each button release started
BOOL    buttonStates[NUM_SWITCHES];            // Current state of each button (FALSE = released, TRUE = pressed)

union MIXED_TYPES SwitchSet;
union MIXED_TYPES SwitchClr;


//-----------------------------------------------------------------------------
// InitInputs
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Initializes all the inputs and related variables
//
//-----------------------------------------------------------------------------
void InitInputs( void)
{
	BYTE i;
	
	PowerEvents = 0;
    PowerOnEvents = 0;
    PowerOffEvents = 0;

	IntrusionSet = 0;
	IntrusionClr = 0;

	SwitchSet.lVal = 0x00;
	SwitchClr.lVal = 0x00;

	ReadIndex = 0;
	LoadIndex = 0;
	ReadIntruIndex = 0;
	LoadIntruIndex = 0;
	PrevValue = 0xFFFFFFFF;
	PrevIntrusions = 0xFF;
	LastReported = 0xFFFFFFFF;
	LastReportedIntrusion = 0xFF;

	// Initialize time-based debouncing variables
	for (i = 0; i < NUM_SWITCHES; i++)
	{
		buttonPressStartTime[i] = 0;
		buttonReleaseStartTime[i] = 0;
		buttonStates[i] = FALSE; // All buttons start in released state
	}

	memset( (void *)&ChangeLoc, 0x00, sizeof( ChangeLoc));
	memset( (void *)&ChangeValue, 0x00, sizeof( ChangeValue));
	memset( (void *)&ChangeIntruLoc, 0x00, sizeof( ChangeIntruLoc));
	memset( (void *)&ChangeIntruValue, 0x00, sizeof( ChangeIntruValue));
}


//-----------------------------------------------------------------------------
// GetRawInputs
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Gets the current I/O values for all the input devices.
//
//-----------------------------------------------------------------------------
void GetRawInputs( void)
{
	BYTE  KeyVal, PlayVal;
	ULONG TmpVal;
	union MIXED_TYPES Tmp;

	// Get the key switch status. Shift down 2 bits to get the
    // bits lined up..
	KeyVal = (P4IN & KEY_MASK) >> KEY_SHIFT;

	// Need to move the bit upto location 0x80
	PlayVal = (P6IN & PLAY_MASK) << 5;

	// The 1st & 3rd bytes are zero, only the 2nd & 4th byte holds the
	// play and key switch bits
    EnTxQueue( 0);
    EnTxQueue( PlayVal);
    EnTxQueue( 0);
    EnTxQueue( KeyVal);

	// Process the intrusions
	// NOTE: Due to legacy issues the intrusion bits in GetRawInputs was left in there
	// original bit locations. Those intrusion bits were shifted down when it came to
	// reporting them as "events".  So to support this pho-pa I have been instructed
	// by Jos� to perpetuate this forward and not go back and correct it in the USBIO.
	Tmp.lVal = 0;

	// When set the cabinet is assumed wired for positive intrusion detection
	if( IntrusionLogic)
	{
		TmpVal = P2IN & INTRUSION_MASK;
	}
	else
	{
		TmpVal = (~P2IN & INTRUSION_MASK) ^ BOARD_CABLE_PRESENT;
	}

	TmpVal = TranslateIntrusion( TmpVal);
	Tmp.bVal[0] = (TmpVal << 4) | (TmpVal >> 4);

	// Load up the intrusion bits
	EnTxQueue( Tmp.bVal[0]);
}


//-----------------------------------------------------------------------------
// FindChangedInputs
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Find out which Input bits were changed from the last read
//
//-----------------------------------------------------------------------------
ULONG FindChangedInputs( ULONG NewValue)
{
	USHORT	Idx;
	ULONG	TmpReport;
	ULONG	Report;
	ULONG	Changed;
	ULONG	Debounce;

	// XORs the last value with this one to fine which bits were changed
	Debounce =(~(NewValue^PrevValue));  		// Sets bits when data was equal for two sequential reads
	Changed = (NewValue^LastReported);   		// Set when an input has change state either high or low
	Report = (Debounce & Changed);              // Report stable debounced changes

	if( Report == 0)
	{
		return( Report);                   		// Nothing has Changed
	}

    // Walk along NewValue and look for the bits that changed
	TmpReport = Report;
	for( Idx = 0; Idx < NUM_SWITCHES; Idx++)
	{
		// AND the changed bit and see if it's true
		if( TmpReport & LSB_LONG)
		{
			if( NewValue & LSB_LONG)
			{
				ChangeValue[ LoadIndex] |= (1l << Idx);		// Bit went Hi
			}
			else
			{
				ChangeValue[ LoadIndex] &= ~(1l << Idx);	// Bit went Low
			}

			ChangeLoc[ LoadIndex] |= (1l << Idx);			// Save bit location changed

			IncLoadIndex();									// Increment counter
			ChangeLoc[ LoadIndex] = 0;
			ChangeValue[ LoadIndex] = 0;					// Make room for new entries
		}

		// Shift the mask down
		TmpReport >>= 1;
		NewValue  >>= 1;
	}

	return( Report);
}


//-----------------------------------------------------------------------------
// FindChangedIntrusions
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Find out which intrusion bits were changed from the last read
//
//-----------------------------------------------------------------------------
BYTE FindChangedIntrusions( BYTE NewValue)
{
	USHORT	Idx;
	BYTE	TmpReport;
	BYTE	Report;
	BYTE	Changed;
	BYTE	Debounce;

	// XORs the last value with this one to fine which bits were changed
	Debounce =(~(NewValue^PrevIntrusions));  	// Sets bits when data was equal for two sequential reads
	Changed = (NewValue^LastReportedIntrusion); // Set when an input has change state either high or low
	Report = (Debounce & Changed);              // Report stable debounced changes

	if( Report == 0)
	{
		return( Report);                   		// Nothing has Changed
	}

    // Walk along NewValue and look for the bits that changed
	TmpReport = Report;
	for( Idx = 0; Idx < NUM_INTRUSIONS; Idx++)
	{
		// AND the changed bit and see if it's true
		if( TmpReport & LSB_LONG)
		{
			if( NewValue & LSB_LONG)
			{
				ChangeIntruValue[ LoadIntruIndex] |= (1l << Idx);	// Bit went Hi
			}
			else
			{
				ChangeIntruValue[ LoadIntruIndex] &= ~(1l << Idx);	// Bit went Low
			}

			ChangeIntruLoc[ LoadIntruIndex] |= (1l << Idx);			// Save bit location changed

			IncLoadIntruIndex();									// Increment counter
			ChangeIntruLoc[ LoadIntruIndex] = 0;
			ChangeIntruValue[ LoadIntruIndex] = 0;					// Make room for new entries
		}

		// Shift the mask up
		TmpReport >>= 1;
		NewValue  >>= 1;
	}

	return( Report);
}


//-----------------------------------------------------------------------------
// CheckEvents
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Manages the event checking, setting and clearing 
// Updated to use regular timing checks like PDCM implementation
//
//-----------------------------------------------------------------------------
void CheckEvents( void)
{
	ULONG	Report;
	ULONG	ReportIntrusion;
	static ULONG lastCheckTime = 0;
	ULONG currentTime = SpinTimer;

	// Check inputs at regular intervals (10ms) like PDCM implementation
	if (currentTime - lastCheckTime >= 10)
	{
		lastCheckTime = currentTime;
		
		// Get the current readings on the inputs
		ReadInputs();

		// Find which key switches changed state.
		Report = FindChangedInputs(Inputs);
		PrevValue = Inputs;

		// Clears reported bits OR sets new high bits
		LastReported = ((LastReported & ~Report) | (Inputs & Report));

		// See if all the input events have been processed
		if (ReadIndex != LoadIndex)
		{
			ProcessInputs();
			IncReadIndex();
		}

		// Check and see if someone is trying to break in. When set the
		// cabinet is assumed wired for positive intrusion detection
		if (IntrusionLogic)
		{
			Intrusions = P2IN & INTRUSION_MASK;
		}
		else
		{
			Intrusions = (~P2IN & INTRUSION_MASK) ^ BOARD_CABLE_PRESENT;
		}

		// Find which intrusion bits changed state.
		ReportIntrusion = FindChangedIntrusions(Intrusions);
		PrevIntrusions = Intrusions;

		// Clears reported bits OR sets new high bits
		LastReportedIntrusion = ((LastReportedIntrusion & ~ReportIntrusion) | (Intrusions & ReportIntrusion));

		// See if all the input events have been processed
		if (ReadIntruIndex != LoadIntruIndex)
		{
			ProcessIntrusion();
			IncReadIntruIndex();
		}
	}
}


//-----------------------------------------------------------------------------
// ReadInputs
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Reads the I/O values.
//
//-----------------------------------------------------------------------------
void ReadInputs( void)
{
    // Get the key switch status. Shift down 2 bits to make
    // things easier.
    Inputs  = (P4IN & KEY_MASK) >> KEY_SHIFT;

	// Move play bit up by 1 to 0x08
	Inputs |= (P6IN & PLAY_MASK) << PLAY_SHIFT;
}


//-----------------------------------------------------------------------------
// ProcessInputs
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This routine handles game specific input events (input signal changes).
// Implements PDCM-style time-based debouncing logic with configurable minimum
// press and release times.
//
//-----------------------------------------------------------------------------
void ProcessInputs( void)
{
	USHORT		Idx;
	ULONG		TblVal;
	ULONG		TmpLoc;
	ULONG		TmpVal;
	ULONG       currentTime;

	// Get current time in milliseconds from SpinTimer
	currentTime = SpinTimer;

	TmpLoc = ChangeLoc[ ReadIndex];
	TmpVal = ChangeValue[ ReadIndex];

	// Work through all the inputs
	for( Idx = 0; Idx < NUM_SWITCHES; Idx++)
	{
		TblVal = INPUT_LOOKUP[ Idx];

        if( TblVal != SPARE_BIT)
        {
            // Check if this button has changed state
            if( TmpLoc & LSB_LONG)
            {
                if( TmpVal & LSB_LONG)
                {
                    // Button released
                    if (buttonStates[Idx])
                    {
                        buttonReleaseStartTime[Idx] = currentTime;
                        
                        // Check if minimum press time was met
                        if ((currentTime - buttonPressStartTime[Idx]) >= MIN_PRESS_TIME_MS)
                        {
                            SwitchClr.lVal |= TblVal;  // Report button release
                            buttonStates[Idx] = FALSE;
                        }
                    }
                }
                else
                {
                    // Button pressed
                    if (!buttonStates[Idx])
                    {
                        // Only start a new press if enough release time has passed
                        if ((currentTime - buttonReleaseStartTime[Idx]) >= MIN_RELEASE_TIME_MS)
                        {
                            buttonPressStartTime[Idx] = currentTime;
                            SwitchSet.lVal |= TblVal;  // Report button press
                            buttonStates[Idx] = TRUE;

                            // Increment persistent press counter. To pair up
                            // with the PDCM, PDC, the play/spin counter is in index 3 
                            Counter_Increment( PLAY_CTR_INDX);   // Only one button - Play/Spin
                        }
                    }
                }
            }
        }

		// Shift the mask up
		TmpLoc >>= 1;
		TmpVal >>= 1;
	}
}


//-----------------------------------------------------------------------------
// ProcessIntrusion
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This routine handles reporting back to the game specific intrusion events
//
//-----------------------------------------------------------------------------
void ProcessIntrusion( void)
{
	USHORT		Idx;
	BYTE		TblVal;
	BYTE		TmpLoc;
	BYTE		TmpVal;

	TmpLoc = ChangeIntruLoc[ ReadIntruIndex];
	TmpVal = ChangeIntruValue[ ReadIntruIndex];

	// Work through all the inputs
	for( Idx = 0; Idx < NUM_INTRUSIONS; Idx++)
	{
		TblVal = INTRUSION_LOOKUP[ Idx];

		if( TblVal != SPARE_BIT)
		{
			// AND the changed bit and see if it's true
			if( TmpLoc & LSB_LONG)
			{
				if( TmpVal & LSB_LONG)
				{
					IntrusionSet |= TblVal;		// Bit went Hi
				}
				else
				{
					IntrusionClr |= TblVal;		// Bit went Low
				}
			}
		}

		// Shift the mask up
		TmpLoc >>= 1;
		TmpVal >>= 1;
	}
}


//-----------------------------------------------------------------------------
// TranslateIntrusion
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This routine handles reporting back to the game specific intrusion events
//
//-----------------------------------------------------------------------------
BYTE TranslateIntrusion( BYTE Incoming)
{
	BYTE	Idx;
	BYTE	TblVal;
	BYTE	Result;
	BYTE	TmpVal;

	Result = 0;
	TmpVal = Incoming;

	// Work through all the inputs
	for( Idx = 0; Idx < NUM_INTRUSIONS; Idx++)
	{
		TblVal = INTRUSION_LOOKUP[ Idx];

		if( TblVal != SPARE_BIT)
		{
			if( TmpVal & LSB_LONG)
			{
				Result |= TblVal;
			}
		}

		// Shift the mask up
		TmpVal >>= 1;
	}

	return( Result);
}


//-----------------------------------------------------------------------------
// LoadEvents
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Event handler used to retrieve and post event changes to the host
//
//-----------------------------------------------------------------------------
void LoadEvents( BYTE cmd)
{
	BYTE  i;
	BYTE  bVal;
	union MIXED_TYPES eVal;

	// It is unknown to this author why the values are negated and then and'd
	// to their respective variable but as it was done in the CJUSBIO we must
	// follow in those traditions previously set forth.
	if( cmd == REARM_EVENTS_CMD)
	{
		for( i = 0; i < NUM_BYTES; i++)
		{
			eVal.bVal[i] = GetNextRxByte();
		}
		SwitchSet.lVal &= ~eVal.lVal;

		for( i = 0; i < NUM_BYTES; i++)
		{
			eVal.bVal[i] = GetNextRxByte();
		}
		SwitchClr.lVal &= ~eVal.lVal;

		bVal = GetNextRxByte();
		IntrusionSet &= ~bVal;

		bVal = GetNextRxByte();
		IntrusionClr &= ~bVal;

		bVal = GetNextRxByte();
		PowerOffEvents &= ~bVal;

		bVal = GetNextRxByte();
		BoardStatEvents &= ~bVal;
		
		// There are two more byte which we are currently not interested
		// in, they are 1) Coin events and 2) Reserved
	}

  	// READ_EVENTS_CMD & READ_REARM_EVENTS_CMD
	else
	{
		SwitchSet.lVal &= INPUTS_MASK;
		SwitchClr.lVal &= INPUTS_MASK;
		// We adding this code to keep from having a set & clear getting
        // returned back in the same message packet.
        if( SwitchSet.lVal != 0)
        {
            // A set bit was detected
            for( i = 0; i < NUM_BYTES; i++)
            {
                EnTxQueue( SwitchSet.bVal[i]);
            }

            // Return zeros for the clear entries
            for( i = 0; i < NUM_BYTES; i++)
            {
                EnTxQueue( 0);
            }

            // Now perform the rearming but only reset the set bits
            // leaving the clear for the next message packet.
            if( cmd == READ_REARM_EVENTS_CMD)
            {
                SwitchSet.lVal = 0;
            }
        }

        else
        {
            // Obviously there are no set bits if we got here
            for( i = 0; i < NUM_BYTES; i++)
            {
                EnTxQueue( 0);
            }

            // Report back if there were any clear bits set
            for( i = 0; i < NUM_BYTES; i++)
            {
                EnTxQueue( SwitchClr.bVal[i]);
            }

            // Now perform the rearming but only reset the clear bits
            if( cmd == READ_REARM_EVENTS_CMD)
            {
                SwitchClr.lVal = 0;
            }
        }

		if( FirstTime)
		{
			FirstTime = 0;
			IntrusionClr = 0;
		}

		// Report anything observed during the power on sequence		
		if( PowerOnEvents)
		{
			IntrusionSet = PowerOnEvents;
			IntrusionClr = 0;
			PowerOnEvents = 0;
		}
		
		// Handle events that occurred while the game was powered off
		if( PowerOffEvents)
		{
			PowerEvents = PowerOffEvents & INTRUSION_MASK;
			PowerOffEvents = 0;
		}

		if( IntrusionSet != 0) {
		    EnTxQueue( IntrusionSet);
		    EnTxQueue ( 0);

		    if(cmd == READ_REARM_EVENTS_CMD) {
		        IntrusionSet = 0;
		    }
		}
		else
		{
		    EnTxQueue( 0);
		    EnTxQueue( IntrusionClr);

		    if(cmd == READ_REARM_EVENTS_CMD) {
		        IntrusionClr = 0;
		    }
		}

		//EnTxQueue( IntrusionSet);
		//EnTxQueue( IntrusionClr);
		EnTxQueue( PowerEvents);
		EnTxQueue( BoardStatEvents);
		EnTxQueue( 0);						// Coin events
		EnTxQueue( 0);						// Reserved events
		EnTxQueue( 0);						// MechReels events
		EnTxQueue( 0);						// MoreMechReels events

		// Now perform the rearming
 		if( cmd == READ_REARM_EVENTS_CMD)
 		{
			PowerEvents = 0;
			BoardStatEvents = 0;
			ErrorNumber = ERROR_NONE;		// Reset the error number as well
			ErrorCulprit = 0;
 		}
	}
}


