// (c)2009 by Texas Instruments Incorporated, All Rights Reserved.
/*----------------------------------------------------------------------------+
|                                                                             |
|                              Texas Instruments                              |
|                                                                             |
|                          MSP430 USB-Example (CDC/HID Driver)                |
|                                                                             |
+-----------------------------------------------------------------------------+
|  Source: usbEventHandling.c, File Version 1.00 2009/12/03                  |
|  Author: RSTO                                                               |
|                                                                             |
|  Description:                                                               |
|  Event-handling placeholder functions.                                      |
|  All functios are called in interrupt context.                              |
|                                                                             |
+----------------------------------------------------------------------------*/

#include "USB_API/USB_Common/device.h"
#include "USB_API/USB_Common/types.h"
#include "USB_API/USB_Common/defMSP430USB.h"
#include "USB_config/descriptors.h"
#include "USB_API/USB_Common/usb.h"
#include "F5xx_F6xx_Core_Lib/HAL_UCS.h"
#include "USB_API/USB_CDC_API/UsbCdc.h"

#include <intrinsics.h>
#include <string.h>
#include "usbConstructs.h"
#include "main.h"


//_____ D E C L A R A T I O N S ____________________________________________

static BYTE	RxQueue[RX_MAX_QUEUE];
static BYTE TxQueue[TX_MAX_QUEUE];
static BYTE Out_Packet[RXBUF_SIZE];
static BYTE In_Packet[TXBUF_SIZE];

PACKET_HEADER Header;
WORD  RxHead;
WORD  RxTail;
WORD  TxHead;
WORD  TxTail;

// Global flags set by events
volatile BYTE bCDCDataReceived_event = FALSE;// Indicates data has been received without an open rcv operation


/*
If this function gets executed, it's a sign that the output of the USB PLL has failed.
returns TRUE to keep CPU awake
*/
BYTE USB_handleClockEvent()
{
#if 0
	//Something happened to the PLL.  This might also show up in the system as an oscillator fault on XT2.
    //The USB connection is probably lost.  Software should ensure any faults on XT2 are resolved,
    //then can attempt to call USB_enable()/USB_connect() again.
    USB_disconnect();
    USB_disable();
#endif
    return ( TRUE) ;                    //Since this event is associated with a change in state, it's a good practice to return TRUE
                                        //so the main loop can adapt.
}

/*
If this function gets executed, it indicates that a valid voltage has just been applied to the VBUS pin.
returns TRUE to keep CPU awake
*/
BYTE USB_handleVbusOnEvent()
{
    //TO DO: You can place your code here
#if 1			/// tedo
    USBKEYPID  = 0x9628;            // set KEY and PID to 0x9628 -> access to configuration registers enabled
    USBPWRCTL |= VUSBEN;
    USBKEYPID  = 0x9600;            // access to configuration registers disabled
#endif			/// tedo

	//We switch on USB and connect to the BUS
    if (USB_enable() == kUSB_succeed)
    {
        USB_reset();
        USB_connect();  // generate rising edge on DP -> the host enumerates our device as full speed device

        BeatLoop = HEART_BEAT_RUNTIME;		/// tedo (16.Oct.13)
    }

    return TRUE;   //return TRUE to wake the main loop (in the case the CPU slept before interrupt)
}

/*
If this function gets executed, it indicates that a valid voltage has just been removed from the VBUS pin.
returns TRUE to keep CPU awake
*/
BYTE USB_handleVbusOffEvent()
{
    //TO DO: You can place your code here
#if 1
    USBKEYPID  = 0x9628;            // set KEY and PID to 0x9628 -> access to configuration registers enabled
    USBPWRCTL |= USBDETEN;
///    USBPWRCTL &= ~VUSBEN;
    USBKEYPID  = 0x9600;            // access to configuration registers disabled
#endif

	XT2_Stop(); 						/// tedo don't want off    XT2_Stop();
    BeatLoop = HEART_BEAT_NO_USB;		/// tedo (16.Oct.13)

    return TRUE;   //return TRUE to wake the main loop (in the case the CPU slept before interrupt)
}

/*
If this function gets executed, it indicates that the USB host has issued a USB reset event to the device.
returns TRUE to keep CPU awake
*/
BYTE USB_handleResetEvent()
{
    //TO DO: You can place your code here

    return TRUE;   //return TRUE to wake the main loop (in the case the CPU slept before interrupt)
}

/*
If this function gets executed, it indicates that the USB host has chosen to suspend this device after a period of active operation.
returns TRUE to keep CPU awake
*/
BYTE USB_handleSuspendEvent()
{
#if 0
	//If this device draws power from the host over VBUS, then this event is the signal for software to ensure that
    //no more than 2.5mA is drawn over VBUS.  Code can be placed here to do this, or it can be placed in the main loop
    //under ST_ENUM_SUSPENDED (but make sure this handler returns TRUE to wake the main loop, if LPM0 was entered).

   //Or, if the device was attached to a bus situation that's somehow providing VBUS but has no active host (host is in standby,
    //or attached to a powered hub with no upstream host), then this will show up as a suspend event without successful enumeration.
    //A good course of action might be to "cancel" the attempt to enumerate:
    if (!(USB_connectionInfo() & kUSB_Enumerated)){
        USB_disconnect();
        USB_disable();
    }               //Now, if this branch executes, the USB state will shift to ST_USB_DISCONNECTED when the function returns.

#endif
    return (TRUE);  //Return TRUE so that the main loop can adapt.
 }

/*
If this function gets executed, it indicates that the USB host has chosen to resume this device after a period of suspended operation.
returns TRUE to keep CPU awake
*/
BYTE USB_handleResumeEvent()
{
    //TO DO: You can place your code here

    return TRUE;   //return TRUE to wake the main loop (in the case the CPU slept before interrupt)
}

/*
If this function gets executed, it indicates that the USB host has enumerated this device :
after host assigned the address to the device. 
returns TRUE to keep CPU awake
*/
BYTE USB_handleEnumCompleteEvent()
{
    //TO DO: You can place your code here

    return TRUE;   //return TRUE to wake the main loop (in the case the CPU slept before interrupt)
}


#ifdef _CDC_

/*
This event indicates that data has been received for interface intfNum, but no data receive operation is underway.
returns TRUE to keep CPU awake
*/
BYTE USBCDC_handleDataReceived(BYTE intfNum)
{
    //TO DO: You can place your code here

    bCDCDataReceived_event = TRUE;

    return TRUE;   //return FALSE to go asleep after interrupt (in the case the CPU slept before interrupt)
}

/*
This event indicates that a send operation on interface intfNum has just been completed.
returns TRUE to keep CPU awake
*/
BYTE USBCDC_handleSendCompleted(BYTE intfNum)
{
    //TO DO: You can place your code here

    return FALSE;   //return FALSE to go asleep after interrupt (in the case the CPU slept before interrupt)
}

/*
This event indicates that a receive operation on interface intfNum has just been completed.
*/
BYTE USBCDC_handleReceiveCompleted(BYTE intfNum)
{
    //TO DO: You can place your code here

    return FALSE;   //return FALSE to go asleep after interrupt (in the case the CPU slept before interrupt)
}

#endif // _CDC_


/////////////////////////////////////////
// CJ stuff
////////////////////////////////////////

void USB_Task( void)
{
	BYTE xCnt;
	BYTE data_recd;

    // Check the USB state and directly main loop accordingly
    switch(USB_connectionState())
    {
       case ST_USB_DISCONNECTED:
/// tedo             __bis_SR_register(LPM3_bits + GIE); 	             // Enter LPM3 w/ interrupts enabled
            _NOP();                                              // For Debugger
            break;
            
       case ST_USB_CONNECTED_NO_ENUM:
            break;
            
       case ST_ENUM_ACTIVE:
/// tedo            __bis_SR_register(LPM0_bits + GIE); 	             // Enter LPM0 (can't do LPM3 when active)
/// tedo            _NOP();                                              // For Debugger
            
            // Exit LPM on USB receive and perform a receive operation 
            if(bCDCDataReceived_event)                           // Some data is in the buffer; begin receiving a command              
            {
              data_recd = cdcReceiveDataInBuffer((BYTE*)Out_Packet, MAX_PACKET_SIZE, CDC0_INTFNUM);

        	  // Enqueue the bytes read into RxQueue
		      for(xCnt = 0; xCnt < data_recd; xCnt++)
		      {
				 EnRxQueue( Out_Packet[xCnt]);
		      }
              bCDCDataReceived_event = FALSE;
            }
            break;
            
       case ST_ENUM_SUSPENDED:
/// tedo            __bis_SR_register(LPM3_bits + GIE); 	        // Enter LPM3 w/ interrupts
            _NOP();
            break;
            
       case ST_ENUM_IN_PROGRESS:
            break;
       
       case ST_NOENUM_SUSPENDED:
            __bis_SR_register(LPM3_bits + GIE);
            _NOP();
            break;
            
       case ST_ERROR:
            _NOP();
            break;
            
       default:;
	}
}

// ---------------------------------------------------------------------------
// >>> InitQueues <<<
//
// Initializes the RxQueue and TxQueue
// ---------------------------------------------------------------------------
void InitQueues(void)
{
   RxHead = RxTail = (RX_MAX_QUEUE - 1);
   TxHead = TxTail = (TX_MAX_QUEUE - 1);
}

// ---------------------------------------------------------------------------
// >>> EnRxQueue <<<
//
// Enqueues data in the RxQueue
// ---------------------------------------------------------------------------
void EnRxQueue(BYTE x)
{
   _EnRxQueue(x); //This macro was replaced by the function to reduce size
}

// ---------------------------------------------------------------------------
// >>> EnTxQueue <<<
//
// Enqueues data in the TxQueue
// ---------------------------------------------------------------------------
void EnTxQueue(BYTE x)
{
   _EnTxQueue(x); //This macro was replaced by the function to reduce size
}

// ---------------------------------------------------------------------------
// >>> SendTxQueue <<<
//
// Sends data in TXQueue to the host across the USB
// ---------------------------------------------------------------------------
void SendTxQueue(void)
{
	BYTE i = sizeof( PACKET_HEADER);

	// Copy the packet header information first 
    memcpy( (void *)In_Packet, (void *)&Header, i);

	// Store current interrupt state, then disable all interrupts
	uint16_t globalInterruptState = __get_SR_register() & GIE;
	__disable_interrupt();
    
	while((!_TxQueueEmpty) && (i < RXBUF_SIZE))
    {
		In_Packet[i++] = GetNextTxByte();
	}

	// Restore previous interrupt state
    __bis_SR_register( globalInterruptState);

    cdcSendDataInBackground( (BYTE*)In_Packet, RXBUF_SIZE, CDC0_INTFNUM, 0);

    _InitTxQueue;
}

// ---------------------------------------------------------------------------
// >>> GetNextRxByte <<<
//
// Polls CommandQueue until there is a byte in the queue, then returns
// that byte
// ---------------------------------------------------------------------------
BYTE GetNextRxByte(void)
{
   _WaitRxQueueNotEmpty;   // Wait till the RxQueue gets something
   RxHead = ((RxHead == (RX_MAX_QUEUE - 1)) ? 0 : (RxHead + 1));
   return RxQueue[RxHead];
}

// ---------------------------------------------------------------------------
// >>> GetNextTxByte <<<
//
// Returns the next byte in TxQueue if TxQueue is not empty. Should check to
// make sure that the TxQueue is not empty before calling this function
// (use the _TxQueueEmpty macro to check)
// ---------------------------------------------------------------------------
BYTE GetNextTxByte(void)
{
   if(!_TxQueueEmpty)
   {
      TxHead = ((TxHead == (TX_MAX_QUEUE - 1)) ? 0 : (TxHead + 1));
      return TxQueue[TxHead];
   }

   // We should NEVER get here but if we done return the "Windows" uninitialized
   // value.  This is here to shut the compiler up.
   return( 0xCC);
}


/*----------------------------------------------------------------------------+
| End of source file                                                          |
+----------------------------------------------------------------------------*/
/*------------------------ Nothing Below This Line --------------------------*/
