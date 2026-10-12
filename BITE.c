//__________________________________________________________________________
// BITE.c
//__________________________________________________________________________
// Copyright 2019 PlayAGS, Inc.
// http://www.playags.com
//
// Program Description:
//
// Source file dedicated to performing Built-In Test when Enabled
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

#include "Inputs.h"
#include "BITE.h"
#include "CIOMspecific.h"
#include "main.h"


//_____ M A C R O S ________________________________________________________


//_____ D E F I N I T I O N S ______________________________________________


//_____ G L O B A L S ______________________________________________________

extern BYTE  Intrusions;
extern ULONG Inputs;


//-----------------------------------------------------------------------------
// PerformBITE
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Called whenever the BITE bit is set.  Performs the built-in-test procedures
// as defined in some reference document.
//
//-----------------------------------------------------------------------------
void PerformBITE( void)
{
    BYTE   KeyFound = 0xFF;
    BYTE   IntruFound = 0xFF;
    USHORT Idx;

    while( is_BITE_enabled())
    {
        // Read the intrusion switches and turn the USB activity LED
        // on if the switch is on.
        // NOTE: The ATP must call out that only one switch
        //       at a time can be turned on then off before
        //       proceeding to the next switch
        Intrusions = ~P2IN & INTRUSION_MASK;
        for( Idx = 0; Idx < NUM_INTRUSIONS; ++Idx)
        {
            // Track one switch at a time
            if( (Intrusions & 0x01) && (IntruFound == 0xFF))
            {
                IntruFound = Idx;
                P4OUT |= LED_USB_ACTIVITY;
            }
            else if( Idx == IntruFound)
            {
                IntruFound = 0xFF;
                P4OUT = ~(P4OUT & LED_USB_ACTIVITY);
            }

            Intrusions >>= 1;
        }

        // Read the key switches and as above turn the USB
        // activity LED for a switch being on.
        // NOTE: The ATP must call out that only one switch
        //       at a time can be turned on then off before
        //       proceeding to the next switch
        Inputs = ((P4IN & KEY_MASK) >> KEY_SHIFT) | (P6IN & PLAY_MASK);
        for( Idx = 0; Idx < NUM_SWITCHES; ++Idx)
        {
            // Track one switch at a time
            if( (Inputs & 0x01) && (KeyFound == 0xFF))
            {
                KeyFound = Idx;
                P4OUT |= LED_USB_ACTIVITY;
            }
            else if( Idx == KeyFound)
            {
                KeyFound = 0xFF;
                P4OUT = ~(P4OUT & LED_USB_ACTIVITY);
            }

            Inputs >>= 1;
        }

        CIOM_Specific();

        // Toggle the USB LED based on heart beat's status
        if( P4OUT & LED_HEART_BEAT)
        {
            P4OUT = ~(P4OUT & LED_USB_ACTIVITY);
        }
        else
        {
            P4OUT |= LED_USB_ACTIVITY;
        }

    }
}
