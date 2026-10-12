//__________________________________________________________________________
// Main.c
//__________________________________________________________________________
// Copyright 2019 PlayAGS, Inc.
// http://www.playags.com
//
// Program Description:
//
// Main source file for the Cabinet I/O Module board
//
// Release 6.0
//    -Initial Revision
//    -18 September 2019
//
//__________________________________________________________________________
//
// Please refer to the MSP430 USB API Stack Programmer's Guide,located
// in the root directory of this installation for more details.
//__________________________________________________________________________

//_____  I N C L U D E S ___________________________________________________

#include <Delay.h>
#include "USB_API/USB_Common/device.h"
#include "USB_API/USB_Common/types.h"           // Basic Type declarations
#include "USB_config/descriptors.h"
#include "USB_API/USB_Common/usb.h"             // USB-specific functions
#include "USB_API/USB_CDC_API/UsbCdc.h"
#include "F5xx_F6xx_Core_Lib/HAL_UCS.h"
#include "F5xx_F6xx_Core_Lib/HAL_PMM.h"
#include <msp430.h>
#include <stdint.h>

#include <intrinsics.h>
#include <string.h>
#include "usbConstructs.h"
#include "CIOMspecific.h"
#include "BITE.h"
#include "Inputs.h"
#include "SwitchCounter.h"
#include "main.h"


//_____ D E F I N I T I O N S ______________________________________________

USHORT MechBellParms[][BELL_MAX_SEQ_ENTRIES]
//                            Period Freq Duration Times Delay
                          = {{  75,   5,    1750,    1,     0},
                             {  25,   5,    2000,    1,     0},
                             {  20,   5,     400,    3,   750},
                             {   0,   0,       0,    0,     0},
                             {   0,   0,       0,    0,     0}};


//_____ G L O B A L S ________________________________________________________

volatile BYTE PowerState = 0;

BOOL    ClearRAM = FALSE;
BOOL    PowerEventFlg = FALSE;
BOOL    IntrusionLogic = TRUE;
BOOL    DeBounceFlag = TRUE; // No longer used for debouncing, but kept for compatibility
BOOL    PlayButtonInUse = FALSE;
BOOL    WirelessChargeInUse = FALSE;
BYTE    PortVal;
BYTE    GpioDir = GPIO_MASK;            // want as outputs
BYTE    Platform = 0xFF;
BYTE    ErrorNumber = ERROR_NONE;
BYTE    ErrorCulprit = 0;
BYTE    ExternalFanLevel = 0;
BYTE    Schedule = 0;                   // If set post command processing is required
BYTE    SerialNumber[DIE_ID_LENGTH];
SHORT   PrevTemp[ NUM_FANS];
USHORT  FanCnt[ NUM_FANS];
USHORT  FanRPM[ NUM_FANS];
USHORT  FanStat[ NUM_FANS];
USHORT  MechBellOn;
USHORT  MechBellOnCnt = 0;
USHORT  MechBellFreqCnt = 0;
USHORT  MechBellFreq = 0;
USHORT  MechBellPeriod = 0;
USHORT  MechBellPeriodCnt = 0;
USHORT  MechBellTimes = 0;
USHORT  MechBellTimesCnt = 0;
USHORT  MechBellDelay = 0;
USHORT  MechBellDelayCnt = 0;
USHORT  BatteryVolt[ ADC_SAMPLES];
USHORT  AvgVoltage = 0;
USHORT  PDSignature = 0;
ULONG   BoardID = 0;
ULONG   HeartBeat = 0;
ULONG   BeatLoop = HEART_BEAT_ERROR;
ULONG   DeBounceCnt = 0;
ULONG   SpinTimer = 0x00FFFFFF;         // Want routines using immediately
ULONG   ms_counter = 0;                 // Additional ms counter for PDC compatibility

// Shadow NVRAM temporary storage
NVRAM_STRUCT TempStorage;


/*----------------------------------------------------------------------------+
| Main Routine                                                                |
+----------------------------------------------------------------------------*/
int main( void)
{
#if 0   // Pushing pause on coming up on battery
    // Enable the Port1.1 to see if we came up on battery
    P1DIR = 0xFD;

    // Wait ~150 microseconds before the lines settle before seeing if we
    // came up on battery power.
    DELAY_US( 150);
    if( (P1IN & BIT1) == 0)
    {
        StayDown();
    }
#endif

    // Initialize the system
    PowerState = PS_OPERATIONAL;

    ReadDieId();                    // Use for serial number
    InitSystem();
    DELAY_MS( 1000);                // Let things settle before proceeding

    // See if Built-In-Test was Enabled
    if( is_BITE_enabled())
    {
        BeatLoop = HEART_BEAT_BITE;
        PerformBITE();
    }
    BeatLoop = HEART_BEAT_ERROR;

    // Read the initial battery levels
    InitADC();
    BatteryMonitor();

    // No OS here. Need to call each task in round-robin mode.
    while( 1)
    {
        USB_Task();

        // Check for any new incoming commands
        if( !_RxQueueEmpty)
        {
            ProcessCmd();
        }

        // Monitor the inputs so when we have a query from the host
        // we don't have to bother reading before responding.
        CheckEvents();

        // Take care of the hardware specific routines
        CIOM_Specific();

        // Check if periodic counter commit is needed (every 10 minutes)
        Counter_PeriodicCommit();

        // See if we had a power event occur and process accordingly
        if( PowerEventFlg)
        {
            PowerEventHdlr();
            PowerEventFlg = FALSE;
        }

        if( Schedule)
        {
            switch( Schedule)
            {
                case SHUT_DOWN_CMD:
                    PowerState = PS_SLEEPING;
                    PowerEventHdlr();
                    break;

                case POWER_CYCLE_CMD:
                    //Set GPIO 6.3 to low
                    P6OUT &= ~POWER_CYCLER;
                    DELAY_MS( 20);
                    //Set GPIO 6.3 back to high after a 1 sec
                    P6OUT |= POWER_CYCLER;
                    break;
            }

            Schedule = 0;
        }
    }  // while( 1)
}


//----------------------------------------------------------------------------
// PowerEventHdlr
//----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Process the power-up or power-down event. Setup the power levels, shutdown
// start processes as needed.
//
//-----------------------------------------------------------------------------
void PowerEventHdlr( void)
{
#ifdef NMI_SHUTDOWN
    SFRIE1 |= NMIIE;                            // NMI enabled
#endif

    // Port1.1 interrupt trapped the power lost signal. If we were operational then prepare for shut things down.
    if( PowerState == PS_SLEEPING)
    {
        PowerOffEvents |= LOST_SYSTEM_POWER;

#ifndef NMI_SHUTDOWN
        // Set to wake-up when power is restored, this will be
        // a transition from low to high on
        P1IES &= ~BIT1;                         // Lo/Hi edge
        P1IFG &= ~BIT1;                         // IFG cleared - gets set when changing PxOUT, PxDIR or PxREN
        P1IE  |= BIT1;                          // Interrupt enabled
#endif

        // Set up to interrupt us such an intrusion event occur
        P2IN   = 0x7F;                          // See top of page 354 of slau208j - trying to keep p2.1 from firing when interrupt is enabled
        P2IES  = ~0x7F;                         // Lo/Hi edge
        P2IFG &= ~0x7F;                         // IFG cleared
        P2IE   = 0x7F;                          // Interrupt enabled

        // Disable oscillator fault before configuring FLL because of DCOFFG
        UCSCTL8_L = 0;
        UCSCTL6_L |= SMCLKOFF;
        UCSCTL4 |= SELA_1;                      // VLO Clock Sources ACLK
        UCSCTL8_L = 0x01;                       // Enable ACLK

        SFRIE1 &= ~OFIE;
        Init_FLL_Settle(8000, 244);
        SFRIE1 |= OFIE;

        // Used in watchdog routine for intrusion detection
        PortVal = 0;

        // Reduce power
        USB_disconnect();

        // Power may be going away from this part that doesn't mean the host
        // has lost power, pretend VBus went away.
        USB_handleVbusOffEvent();

        // Disable USB interrupts
        USBIE = 0;          /// tedo 7/16

        DisablePorts();
        SetVCore( 0);       // This should walk the power down from 3 to 0

        // Stay in here until power resumes.  If an intrusion event happens
        // the interrupt routine will service is and this will bring it
        // back to sleep.
        while( PowerState == PS_SLEEPING)
        {
/// 8/20            WDTCTL = WDTPW + WDTTMSEL + WDTSSEL1 + (WDTIS2 + WDTIS1);     // WDT 15.625ms, VLOCLK, interval timer
 /* /// 1/28/21 */           WDTCTL = WDTPW + WDTTMSEL + WDTSSEL1 + WDTIS2;     // WDT 1sec, VLOCLK, interval timer
 /* /// 1/28/21 */           SFRIE1 |= WDTIE;                          // Enable WDT interrupt
            __bis_SR_register( LPM4_bits + GIE);      // Enter LPM4, enable interrupts
            // For debugger only
            __no_operation();
        }

        // Stop watchdog timer
        WDTCTL = WDTPW + WDTHOLD;

        // Wait until power to returns
        // Don't interrupt on intrusion while under power
        P2IE  = 0x00;

        // On startup we get this setup with InitSystem
        // Bring the system back up
        SetVCore( 3);
#if 1
        // Port 1 - Power reset P1.1, power fail bit P1.2(not used); Fan PWMs P1.3-5;
        P1SEL |= BIT3 + BIT4 + BIT5;            // P1.3 - P1.5 options select
        P1DIR  = 0xF9;                          // Power monitor P1.1 & P1.2, P1.3 - P1.5 PWM fan outputs
        P1OUT  = 0xF9;                          // Drive fans hi and intrusion power on
#ifndef NMI_SHUTDOWN
        P1REN |= BIT1;                          // Enable internal resistance
        P1OUT |= BIT1;                          // Set as pull-up resistance
        P1IES |= BIT1;                          // Hi/Lo edge
        P1IFG &= ~BIT1;                         // IFG cleared - gets set when changing PxOUT, PxDIR or PxREN
        P1IE  |= BIT1;                          // Interrupt enabled
#endif

        // Port 2 - Intrusion bits P2.0 - P2.6 (setup as interrupts when sleeping)
        P2DIR = 0x80;
        P2REN = 0x7F;                           // Enable internal resistance
        P2OUT = 0x7F;                           // Set as pull-up resistance

        // Port 3 - Platform type (P3.2 - P3.7)
        P3DIR |= 0x03;
        P3OUT  = 0x00;

        // Port 4 - LEDs P4.0 - P4.1; Keys P4.2 - P4.4; Backplane Ground detect P4.5
        P4DIR = 0xC3;                           // P4.0 - P4.1 On board status LEDs
        P4REN |= BIT2 + BIT3 + BIT4;            // Enable internal resistance
        P4OUT |= BIT2 + BIT3 + BIT4;            // Set as pull-up resistance
        P4OUT |= BIT0 + BIT1;                   // Set LEDs on

        // Port 5 - See InitClock for assignments.

        // Port 6 - A/D conversion for temp P6.4; GPIO P6.0 - P6.3
        P6SEL = 0x10;                           // Enable A/D channel A4 (P6.4)
        P6DIR = 0xEF;                           // P6.4 is an input
        P6OUT = 0xEE;

        // With v6.13 we are assuming there's a Play button & lamp attached on
        // GPIOs P6.2 & P6.3 and the wireless charger enable pin on P6.1 but
        // disabled
        GpioDir &= ~PLAY_SWITCH_BIT;            // Input
        GpioDir |= PLAY_LAMP_BIT;               // Output
        GpioDir |= WIRELESS_CHARGER_BIT;        // Output
        P6DIR = GpioDir & GPIO_MASK;
        P6REN |= PLAY_SWITCH_BIT;               // Enable internal resistance
        P6OUT |= PLAY_SWITCH_BIT;               // Set as pull-up resistance
        P6OUT &= ~PLAY_LAMP_BIT;                // Start with lamp off
        P6OUT &= ~WIRELESS_CHARGER_BIT;         // Set low
        PlayButtonInUse = TRUE;
        WirelessChargeInUse = FALSE;

        // Port 7 - 1-wire temp sensor P7.0 - P7.2; Fan RPM sense bits P7.4 - P7.6
        P7SEL = 0x70;                           // Want to use the capture feature
        P7DIR = 0x8F;                           // Set unused pins as outputs
        P7REN = 0x70;
        P7OUT = 0x07;                           // 1-wire devices expect to start & end high

        // Port 8 - P8.0 Built In Test Enable, active low
        P8DIR  = 0xFE;                          // P8.0 is an input
        P8REN  = 0x01;                          // Enable internal resistance
        P8OUT  = 0xFF;                          // Set as pull-up resistance

        // Port J
        PJDIR  = 0xFFFF;
        PJOUT  = 0x0000;

#else
        InitPorts();
#endif

        // Set the clock back on
        InitClock();
        UCSCTL8_L = 0x07;                       // Enable SMCLK, MCLK & ACLK
        UCSCTL6_L &= ~SMCLKOFF;                 // Turn on SMCLK

        // For some reason which I don't know I need to recall
        // InitClock to get the USB to startup.
        InitClock();

        // Force a wakeup if necessary and indicate the Vbus is back
        USB_forceRemoteWakeup();
        USB_handleVbusOnEvent();
        BoardStatEvents |= EVENT_USB_DETECTED;

        // Process any intrusions while asleep
        if( PortVal & BIT0)
        {
            PowerOffEvents |= INTRUSION_2;  // Main door
        }
        if( PortVal & BIT1)
        {
            PowerOffEvents |= INTRUSION_3;  // Logic door
        }
        if( PortVal & BIT2)
        {
            PowerOffEvents |= INTRUSION_1;  // Belly door
        }
        if( PortVal & BIT3)
        {
            PowerOffEvents |= INTRUSION_0;  // Cash door
        }
        if( PortVal & BIT4)
        {
            PowerOffEvents |= INTRUSION_4;  // Spare 1
        }
        if( PortVal & BIT5)
        {
            PowerOffEvents |= INTRUSION_5;  // Cable detect
        }
        if( PortVal & BIT6)
        {
            PowerOffEvents |= INTRUSION_6;  // Spare 2
        }
    }
}


//-----------------------------------------------------------------------------
// InitSystem
//-----------------------------------------------------------------------------
//
// Return Value : status - < 0 indicates an error
// Parameters   : None
//
// Initialize the I/O pins, clocks, various interfaces and variables
//
//-----------------------------------------------------------------------------
void InitSystem( void)
{
    WDTCTL = WDTPW + WDTHOLD;                   // Stop watchdog timer

    // Initialize the device
    __disable_interrupt();                      // Disable global interrupts

#ifdef NMI_SHUTDOWN
    SFRIFG1 &= ~NMIIE;                          // NMI flag cleared
    SFRRPCR |= SYSNMIIES | SYSNMI;              // Set to NMI, and interrupt on falling edge
    SFRIE1  |= NMIIE;                           // NMI enabled
#endif

    InitPorts();                                // Init ports (do first ports because clocks do change ports)
    SetVCore( 3);                               // USB core requires the VCore set to 1.8 volt, independent of CPU clock freq
    InitClock();
    UCSCTL8_L = 0x07;                           // Enable SMCLK, MCLK & ACLK
    UCSCTL6_L &= ~SMCLKOFF;                     // Turn on SMCLK

    __enable_interrupt();                       // enable global interrupts

    InitTimerA1();

    // Initialize persistent switch press counters
    Counter_Init();

    // Enable various USB event handling routines
    USB_init();
    USB_setEnabledEvents( kUSB_VbusOnEvent + kUSB_VbusOffEvent + kUSB_receiveCompletedEvent
///                        + kUSB_dataReceivedEvent + kUSB_UsbSuspendEvent + kUSB_UsbResumeEvent + kUSB_UsbResetEvent);
    + kUSB_dataReceivedEvent + kUSB_UsbResumeEvent + kUSB_UsbResetEvent);

    // See if we're already attached physically to USB, and if so, connect to it
    // Normally applications don't invoke the event handlers, but this is an exception.
    if( USB_connectionInfo() & kUSB_vbusPresent)
    {
        USB_handleVbusOnEvent();
        BoardStatEvents |= EVENT_USB_DETECTED;
    }
    // USB is not connected but we still need the XT2 oscillator up and running for the
    // other peripherals
    else
    {
        XT2_Start( XT2DRIVE_3);
        BeatLoop = HEART_BEAT_NO_USB;
    }

    // Initialize the USB TX and RX queues
    InitQueues();

    // Initialize I/O Board specific features
    InitUser();
}


//-----------------------------------------------------------------------------
// InitUser
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Initialize user related items
//
//-----------------------------------------------------------------------------
void InitUser( void)
{
    // Initialize input related variables
    InitInputs();

    // See if we lost RAM integrity if we did or this is our 1st time
    // initialize the runtime variables and attached devices
    if( PDSignature != 0xFACE)
    {
        ClearRAM = TRUE;
    }

    // The game may request a RAM clear hence the method for initialization
    BoardStatEvents = 0;
    if( ClearRAM)
    {
        // If the sig didn't match flag the fact, likely battery is dead.
        if( PDSignature != 0xFACE)
        {
            PDSignature = 0xFACE;
            BoardStatEvents |= EVENT_RAM_CLEAR;
        }

        AvgVoltage = 0;
    }

    // Want to make sure these start out as zero
    memset( (void *)&FanRPM, 0x00, sizeof(USHORT)*NUM_FANS);
    memset( (void *)&PrevTemp, 0x00, sizeof(USHORT)*NUM_FANS);

    // Check door states and save for reporting later
    // When set the cabinet is assumed wired for positive intrusion detection
    if( IntrusionLogic)         ///#ifdef ONTARIO_BANDADE
    {
        PowerOnEvents = P2IN & INTRUSION_MASK;
    }
    else
    {
        PowerOnEvents = (~P2IN & INTRUSION_MASK) ^ BOARD_CABLE_PRESENT;

        // To support "Big Red2" with the IOM we need to leave the door wiring
        // as is which is negative logic. The logic case and cable present are
        // still looked at as positive logic so the f/w will compensate for
        // the reporting opens the same way
        PowerOnEvents ^= BOARD_LOGIC_DOOR;
    }

    PowerOnEvents = TranslateIntrusion( PowerOnEvents);

    // Initialized despite state of RAM or request
    PowerEventFlg = FALSE;
    PowerState = PS_OPERATIONAL;
    HeartBeat = 0;
    ErrorNumber = ERROR_NONE;
    ErrorCulprit = 0;

    // Initialize the PWMs driving the fans
    CIOM_Init();
}


/*----------------------------------------------------------------------------+
| System Initialization Routines                                              |
+----------------------------------------------------------------------------*/

// Initializes the clocks.  Starts the DCO at USB_MCLK_FREQ (the CPU freq set with the Desc
// Tool), using the REFO as the FLL reference.  Configures the high-freq crystal, but
// doesn't start it yet.
void InitClock( void)
{
    // Disable oscillator fault before configuring FLL because of DCOFFG
    SFRIE1 &= ~OFIE;

    // Enable XT2 pins
    P5SEL |= 0x0C;

    // Use the REFO oscillator as the FLL reference, and also for ACLK
    UCSCTL3 = (UCSCTL3 & ~(SELREF_7)) | (SELREF__REFOCLK);
    UCSCTL4 = (UCSCTL4 & ~(SELA_7)) | (SELA__REFOCLK);

    // Start the FLL, at the freq indicated by the config constant USB_MCLK_FREQ
    Init_FLL_Settle(USB_MCLK_FREQ/1000, USB_MCLK_FREQ/32768);


#if 0
    // Executing the following two commands keeps the system from
    // automatically enumerating the USB, removing and reinserting
    // the USB connection is required uncommented.
    SFRIFG1 = 0;
    SFRIE1 |= OFIE;
#endif
}


//-----------------------------------------------------------------------------
// InitPorts
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Initialize the I/O ports
//
//-----------------------------------------------------------------------------
void InitPorts( void)
{
    // Identify I/O board platform type but first we need to define Port 3
    // where we get our ID reference.

    // Port 3 - Platform type (P3.2 - P3.7)
    P3DIR |= 0x03;
    P3OUT  = 0x00;
    Platform = P3IN & PLATFORM_MASK;

    // Board specific settings
    switch( Platform)
    {
        case CIOM:
            // Port 1 - Power reset P1.1, power fail bit P1.2(not used); Fan PWMs P1.3-5;
            P1SEL |= BIT3 + BIT4 + BIT5;            // P1.3 - P1.5 options select
            P1DIR  = 0xF9;                          // Power monitor P1.1 & P1.2, P1.3 - P1.5 PWM fan outputs
            P1OUT  = 0xF9;                          // Drive fans hi and intrusion power on
#ifndef NMI_SHUTDOWN
            P1REN |= (BIT1 | BIT2);                          // Enable internal resistance
            P1OUT |= (BIT1 | BIT2);                          // Set as pull-up resistance
            P1IES |= (BIT1 | BIT2);                          // Hi/Lo edge
            P1IFG &= ~(BIT1 | BIT2);                         // IFG cleared - gets set when changing PxOUT, PxDIR or PxREN
            P1IE  |= (BIT1 | BIT2);                          // Interrupt enabled
#endif

            // Port 2 - Intrusion bits P2.0 - P2.6 (setup as interrupts when sleeping)
            P2DIR = 0x80;
            P2REN = 0x7F;                           // Enable internal resistance
            P2OUT = 0x7F;                           // Set as pull-up resistance

            // Port 4 - LEDs P4.0 - P4.1; Keys P4.2 - P4.4; Backplane Ground detect P4.5
            P4DIR = 0xC3;                           // P4.0 - P4.1 On board status LEDs
            P4REN |= BIT2 + BIT3 + BIT4;            // Enable internal resistance
            P4OUT |= BIT2 + BIT3 + BIT4;            // Set as pull-up resistance
            P4OUT |= BIT0 + BIT1;                   // Set LEDs on

            // Port 5 - See InitClock for assignments.

            // Port 6 - A/D conversion for temp P6.4; GPIO P6.0 - P6.3
            P6SEL = 0x10;                           // Enable A/D channel A4 (P6.4)
            P6DIR = 0xEF;                           // P6.4 is an input
            P6OUT = 0xEE;

            // With v6.13 we are assuming there's a Play button & lamp attached on
            // GPIOs P6.2 & P6.3 and the wireless charger enable pin on P6.1 but
            // disabled
            GpioDir &= ~PLAY_SWITCH_BIT;            // Input
            GpioDir |= PLAY_LAMP_BIT;               // Output
            GpioDir |= WIRELESS_CHARGER_BIT;        // Output
            P6DIR = GpioDir & GPIO_MASK;
            P6REN |= PLAY_SWITCH_BIT;               // Enable internal resistance
            P6OUT |= PLAY_SWITCH_BIT;               // Set as pull-up resistance
            P6OUT &= ~PLAY_LAMP_BIT;                // Start with lamp off
            P6OUT &= ~WIRELESS_CHARGER_BIT;         // Set low
            PlayButtonInUse = TRUE;
            WirelessChargeInUse = FALSE;

            // Port 7 - 1-wire temp sensor P7.0 - P7.2; Fan RPM sense bits P7.4 - P7.6
            P7SEL = 0x70;                           // Want to use the capture feature
            P7DIR = 0x8F;                           // Set unused pins as outputs
            P7REN = 0x70;
            P7OUT = 0x77;                           // 1-wire devices expect to start & end high

            // Port 8 - P8.0 Built In Test Enable, active low
            P8DIR  = 0xFE;                          // P8.0 is an input
            P8REN  = 0x01;                          // Enable internal resistance
            P8OUT  = 0xFF;                          // Set as pull-up resistance

            // Port J
            PJDIR  = 0xFFFF;
            PJOUT  = 0x0000;
            break;


        // We appear to have an identity crisis - put the board into bootmode
        // so it can be reprogrammed.
        default:
            __disable_interrupt();                  // Ensure no application interrupts fire during BSL
            ((void (*)())0x1000)();                 // This sends execution to the BSL. When execution
                                                    // returns to the user app, it will be via the reset
                                                    // vector, meaning execution will re-start.
            while(1);                               // SHOULD not reach this point
    }

    // Set our identity
    BoardID = PLATFORM_CIOM;
    ProductID = USB_PID_CIOM;
}


//-----------------------------------------------------------------------------
// DisablePorts
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// When the system is powering down set the I/O port into a mode where they
// don't source any current.
//
//-----------------------------------------------------------------------------
void DisablePorts( void)
{
    P1OUT = 0x00; P3OUT = 0x00; P4OUT = 0x00; P5OUT = 0x00;
    P6OUT = 0x00; P7OUT = 0x00; P8OUT = 0x00; PJOUT = 0x00;
    P1DIR = 0xFD; P3DIR = 0xFF; P4DIR = 0xFF; P5DIR = 0xFF;
    P6DIR = 0xFF; P7DIR = 0xFF; P8DIR = 0xFF; PJDIR = 0xFF;

#ifdef NMI_SHUTDOWN
    P2OUT = 0x00;
    P2DIR = 0xFF;
#endif
}


//-----------------------------------------------------------------------------
// UpdateNvRam
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This routine copies the contents of the shadow RAM into NVRAM or in our
// case the information memory Info-A
//
//-----------------------------------------------------------------------------
void UpdateNvRam( void)
{
    int     i;
    uint8_t *pDest, *pSrc;
    uint8_t size = sizeof( TempStorage);

    // Initialize pointers
    pDest = (uint8_t *)&NVRAM_START;
    pSrc  = (uint8_t *)&TempStorage;

    // Disable interrupts during flash operation so an ISR (which executes
    // from flash) can't be fetched while the flash controller is mid-erase/write.
    __disable_interrupt();

    // Erasing memory in Info-A
    FCTL4 = FWPW;                           // Clear Lock bit
    FCTL3 = FWPW+LOCKA;                     // Unlock SegmentA
    FCTL1 = FWPW+ERASE;                     // Set Bank Erase bit
    *pDest = 0;                             // Dummy erase byte
    FCTL3 = FWPW+LOCKA;                     // Lock SegmentA

    // Copy the contents of TempStorage
    FCTL4 = FWPW;                           // Clear Lock bit
    FCTL3 = FWPW+LOCKA;                     // Unlock SegmentA
    FCTL1 = FWPW+WRT;                       // Set WRT bit for write operation
    for( i = 0; i < size; i++)
    {
        *pDest++ = *pSrc++;
    }

    FCTL1 = FWKEY;                          // Clear WRT bit
    FCTL3 = FWPW+LOCKA;                     // Lock SegmentA

    __enable_interrupt();
}


//----------------------------------------------------------------------------
// UNMI_ISR
//----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// User NMI interrupt handler
//
//-----------------------------------------------------------------------------
#pragma vector = UNMI_VECTOR
__interrupt VOID UNMI_ISR( void)
{
#ifdef NMI_SHUTDOWN
    USHORT i;
#endif

    switch( __even_in_range(SYSUNIV, SYSUNIV_BUSIFG))
    {
        case SYSUNIV_NONE:
          __no_operation();
          break;
        case SYSUNIV_NMIIFG:
#ifdef NMI_SHUTDOWN
            do
            {
                SFRIFG1 &= ~0x10;               // Clear NMI Fault flag
                for( i = 0xFFFF; i > 0; --i);   // Time for flag to set
            } while ( (SFRIFG1 & NMIIFG) );

            PowerEventFlg = TRUE;
            if( (SFRRPCR & SYSNMIIES) == 0)     // Power restored
            {
                PowerState = PS_OPERATIONAL;
                _BIC_SR_IRQ( LPM4_bits);        // Exit LPM4

                SFRRPCR |= SYSNMIIES;           // Interrupt on falling edge
            }
            else                                // Lost power
            {
                PowerState = PS_SLEEPING;

                SFRRPCR &= ~SYSNMIIES;          // Interrupt on raising edge
            }

#else
          __no_operation();
#endif
          break;
        case SYSUNIV_OFIFG:
            UCSCTL7 &= ~(DCOFFG + XT1LFOFFG + XT2OFFG); //Clear OSC flaut Flags fault flags
            SFRIFG1 &= ~OFIFG;                    // Clear OFIFG fault flag
            break;
        case SYSUNIV_ACCVIFG:
            __no_operation();
          break;
        case SYSUNIV_BUSIFG:
            // If bus error occurred - the cleaning of flag and re-initializing of USB is required.
            SYSBERRIV = 0;                        // clear bus error flag
            USB_disable();                        // Disable
            break;
    }
}


//----------------------------------------------------------------------------
// InitTimerA1
//----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Initialize TimerA1 interrupt.
//
//-----------------------------------------------------------------------------
void InitTimerA1( void)
{
    TA1CCR0 = TIMER_COUNTER;                    // Striving for a 1usec interrupt
    TA1CTL = TASSEL_2 + MC_1 + TACLR + TAIE;    // SMCLK, up-mode to TA1CCR0, clear TAR
}


//----------------------------------------------------------------------------
// TIMER1_A1_ISR
//----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
//  As coded and with TACLK = 32768Hz, toggle rates are:
//  CCR1 = 32768/(2*16) = 1024Hz
//
// Timer1_A1 Interrupt Vector (TAIV) handler
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// get_ms_time
//-----------------------------------------------------------------------------
//
// Return Value : Current time in milliseconds
// Parameters   : None
//
// Returns a millisecond timestamp for timing operations using the hardware timer
// This timer is incremented in the TIMER1_A1_ISR function
//
//-----------------------------------------------------------------------------
ULONG get_ms_time(void)
{
    // Return the current millisecond count
    // This value is updated by the timer interrupt
    // Using ms_counter for PDC compatibility
    return ms_counter;
}


#pragma vector=TIMER1_A1_VECTOR
__interrupt void TIMER1_A1_ISR(void)
{
    switch(__even_in_range(TA1IV,14))
    {
        case  0: break;                         // No interrupt
        case  2: break;                         // CCR1 not used
        case  4: break;                         // CCR2 not used
        case  6: break;                         // reserved
        case  8: break;                         // reserved
        case 10: break;                         // reserved
        case 12: break;                         // reserved
        case 14:                                // overflow
            SpinTimer++;
            ms_counter++;  // Update PDC-compatible counter

            // Blink the LED to show a heartbeat
            if( ++HeartBeat >= BeatLoop)
            {
                HeartBeat = 0;
                P4OUT ^= LED_HEART_BEAT;   // Toggle LED
            }

            // We no longer need the DeBounceFlag as we now use a time-based approach
            // with SpinTimer for checking inputs regularly

            // If set, ring the mechanical bell
            if( MechBellOn)
            {
                ++MechBellPeriodCnt;

                // Turn the bell off
                if( ++MechBellFreqCnt == MechBellFreq)
                {
                    P6OUT &= ~MECH_BELL;
                }
                else if( MechBellPeriodCnt >= MechBellPeriod)
                {
                    P6OUT |= MECH_BELL;
                    MechBellFreqCnt = 0;
                    MechBellPeriodCnt = 0;
                }

                // Stop ringing when the times up
                if( ++MechBellOnCnt >= MechBellOn)
                {
                    P6OUT &= ~MECH_BELL;
                    MechBellOn = 0;

                    // Are we ringing the bell multiple times with a
                    // delay between rings
                    if( --MechBellTimes)
                    {
                        // Get the bell ready to ring again
                        P6DIR |= MECH_BELL;
                        P6OUT |= MECH_BELL;
                    }

                    // We're done, shut everything down
                    else
                    {
                        MechBellDelay = 0;
                    }
                }
            }

            // Do we have a delay between a sequences
            else if( MechBellDelay)
            {
                ++MechBellDelayCnt;
                if( MechBellDelayCnt >= MechBellDelay)
                {
                    MechBellDelayCnt = 0;

                    // If Times is zero, we need to clear the delay
                    if( MechBellTimes == 0)
                    {
                        MechBellDelay = 0;
                    }

                    // We still have more ringing to do, reload
                    // timer and clear the counter
                    else
                    {
                        MechBellOn = MechBellOnCnt;
                        MechBellOnCnt = 0;
                    }
                }
            }
            break;

        default: break;
    }
}


//----------------------------------------------------------------------------
// TIMER_B0_ISR
//----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
//  Used to determine the RPM of the fans
//  CCR1 = 32768/(2*16) = 1024Hz
//
// Timer_B1 Interrupt Vector (TB0IV) handler
//
//-----------------------------------------------------------------------------
#pragma vector=TIMER0_B1_VECTOR
__interrupt void TIMER_B1_ISR (void)
{
    switch(__even_in_range(TB0IV,14))
    {
        case  0: break;                         // No interrupt
        case  2: break;                         // TB0CCR1
        case  4:                                // TB0CCR2
            ++FanCnt[ FAN_0];
            break;
        case  6:                                // TB0CCR3
            ++FanCnt[ FAN_1];
            break;
        case  8:                                // TB0CCR4
            ++FanCnt[ FAN_2];
            break;
        case 10: break;                         // TB0CCR5
        case 12: break;                         // TB0CCR6
        case 14: break;                         // overflow
        default: break;
    }
}


//----------------------------------------------------------------------------
// Port_1
//----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Port 1 interrupt service routine, gets signaled when a power event has
// occurred (i.e., power up & power down).
//
//-----------------------------------------------------------------------------
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
    switch(__even_in_range(P1IV, P1IV_P1IFG7))
    {
        // Vector  P2IV_NONE:  No Interrupt pending
        case P1IV_NONE:   break;                // No interrupt
        case P1IV_P1IFG0: break;
        case P1IV_P1IFG1:
#ifdef NMI_SHUTDOWN
#else
            P1IFG &= ~BIT1;                     // IFG cleared
            PowerEventFlg = TRUE;
            if( P1IN & BIT1)                    // Power restored
            {
                PowerState = PS_OPERATIONAL;
                _BIC_SR_IRQ( LPM4_bits);        // Exit LPM4
            }
            else                                // Lost power
            {
                PowerState = PS_SLEEPING;
            }
#endif
            break;
        case P1IV_P1IFG2:                       // reserved
#ifdef NMI_SHUTDOWN
#else
#if 1
            P2IFG &= ~BIT2;                     // IFG cleared
            PowerEventFlg = TRUE;
            if( P1IN & BIT2)                    // Power restored
            {
                PowerState = PS_OPERATIONAL;
                _BIC_SR_IRQ( LPM4_bits);        // Exit LPM4
            }
            else                                // Lost power
            {
                PowerState = PS_SLEEPING;
            }
#endif
#endif
            break;
        case P1IV_P1IFG3: break;                //    "
        case P1IV_P1IFG4: break;                //    "
        case P1IV_P1IFG5: break;                //    "
        case P1IV_P1IFG6: break;                //    "
        case P1IV_P1IFG7: break;                //    "
    }
}


//----------------------------------------------------------------------------
// Port_2
//----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Port 2 interrupt service routine, used for intrusion detection.
//
//-----------------------------------------------------------------------------
#pragma vector=PORT2_VECTOR
__interrupt void Port_2(void)
{
    //
    // Context save interrupt flag before calling interrupt vector.
    // Reading interrupt vector generator will automatically clear IFG flag
    //
    switch(__even_in_range(P2IV, P2IV_P2IFG7))
    {
        // Vector  P2IV_NONE:  No Interrupt pending
        case P2IV_NONE: break;                  // No interrupt
        case P2IV_P2IFG0:
            P2IFG &= ~BIT0;                     // IFG cleared
            PowerOffEvents |= INTRUSION_2;      // Main door
            break;
        case P2IV_P2IFG1:
            P2IFG &= ~BIT1;
            PowerOffEvents |= INTRUSION_3;      // Logic door
            break;
        case P2IV_P2IFG2:
            P2IFG &= ~BIT2;
            PowerOffEvents |= INTRUSION_1;      // Belly door
            break;
        case P2IV_P2IFG3:
            P2IFG &= ~BIT3;
            PowerOffEvents |= INTRUSION_0;      // Cash door
            break;
        case P2IV_P2IFG4:
            P2IFG &= ~BIT4;
            PowerOffEvents |= INTRUSION_4;      // Spare 1
            break;
        case P2IV_P2IFG5:
            P2IFG &= ~BIT5;
            PowerOffEvents |= INTRUSION_5;      // Cable detect
            break;
        case P2IV_P2IFG6:
            P2IFG &= ~BIT6;
            PowerOffEvents |= INTRUSION_6;      // Spare 2
            break;
        case P2IV_P2IFG7: break;                // reserved
    }
}


//----------------------------------------------------------------------------
// InitADC
//----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Initialize A/D interrupt.
//
//-----------------------------------------------------------------------------
void InitADC( void)
{
    ADC12CTL0 = ADC12ON + ADC12SHT0_8 + ADC12MSC;   // Turn on ADC12, set sampling time
                                                    // set multiple sample conversion
    ADC12CTL1 = ADC12SHP + ADC12CONSEQ_2;           // Use sampling timer, repeat-single-channel
    ADC12CTL2 = ADC12RES_2;                         // 12 bit resolution
    ADC12MCTL0 = ADC12SREF_0 + ADC12INCH_4;         // ref+=AVcc, channel = A4
    ADC12IE = 0x01;                                 // Enable ADC12IFG.0
}


//----------------------------------------------------------------------------
// BatteryMonitor
//----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Starts the A/D process for reading the battery voltage.
//
//-----------------------------------------------------------------------------
void BatteryMonitor( void)
{
    ADC12CTL0 |= ADC12ENC;                      // Enable conversions
    ADC12CTL0 |= ADC12SC;                       // Start conversion

    // After the initial reading check the voltage range
    if( AvgVoltage != 0)
    {
        if( AvgVoltage < ADC_MIN_BAT_VOLT)
        {
            BoardStatEvents |= EVENT_BATTERY_LOW;
        }
    }
}


//----------------------------------------------------------------------------
// ADC12ISR
//----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// A/D interrupt service routine
//
//-----------------------------------------------------------------------------
#pragma vector=ADC12_VECTOR
__interrupt void ADC12ISR (void)
{
    static unsigned char index = 0;

    switch(__even_in_range(ADC12IV,34))
    {
        case  0: break;                         // Vector  0:  No interrupt
        case  2: break;                         // Vector  2:  ADC overflow
        case  4: break;                         // Vector  4:  ADC timing overflow

        case  6:                                // Vector  6:  ADC12IFG0
            BatteryVolt[index] = ADC12MEM0;     // Move results
            index++;                            // Increment results index, modulo; Set Breakpoint1 here

            // When we get our samples worth stop the conversions plus
            // calculate the average voltage
            if( index == ADC_SAMPLES)
            {
                ADC12CTL0 &= ~ADC12ENC;         // Stop conversions

                AvgVoltage = 0;
                for( index = 0; index < ADC_SAMPLES; index++)
                {
                    AvgVoltage += BatteryVolt[ index];
                }
                AvgVoltage = AvgVoltage >> 3;

                ADC12IFG  &= ~ADC12IFG0;        // Clear interrupt
                index = 0;
            }
            break;

        case  8: break;                         // Vector  8:  ADC12IFG1
        case 10: break;                         // Vector 10:  ADC12IFG2
        case 12: break;                         // Vector 12:  ADC12IFG3
        case 14: break;                         // Vector 14:  ADC12IFG4
        case 16: break;                         // Vector 16:  ADC12IFG5
        case 18: break;                         // Vector 18:  ADC12IFG6
        case 20: break;                         // Vector 20:  ADC12IFG7
        case 22: break;                         // Vector 22:  ADC12IFG8
        case 24: break;                         // Vector 24:  ADC12IFG9
        case 26: break;                         // Vector 26:  ADC12IFG10
        case 28: break;                         // Vector 28:  ADC12IFG11
        case 30: break;                         // Vector 30:  ADC12IFG12
        case 32: break;                         // Vector 32:  ADC12IFG13
        case 34: break;                         // Vector 34:  ADC12IFG14
        default: break;
    }
}

#if 1       /// 11/Feb/2021 - not used
//----------------------------------------------------------------------------
// WDT_ISR
//----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Watchdog Timer interrupt service routine. Used to monitor intrusions while
// the game is off.
//
//-----------------------------------------------------------------------------
#pragma vector=WDT_VECTOR
__interrupt void WDT_ISR(void)
{
    // Enable the port
    P2DIR = 0x80;
    P2REN = 0x7F;                           // Enable internal resistance
    P2OUT = 0x7F;                           // Set as pull-up resistance

    // Need power in order to read intrusion switches but we must
    // wait ~150 microseconds before the lines settle
    __delay_cycles( 1200);

    if( IntrusionLogic)
    {
        PortVal = P2IN & INTRUSION_MASK;
    }
    else
    {
        PortVal = (~P2IN & INTRUSION_MASK) ^ BOARD_CABLE_PRESENT;
    }

    // Disable the port
    P2OUT = 0x00;
    P2DIR = 0xFF;
}
#endif


//----------------------------------------------------------------------------
// ReadDieId
//----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Retrieve a unique processor ID using the TLV Die record found in the MSP430.
// This is being used as a serial number for tracking purposes.
//
//-----------------------------------------------------------------------------
static inline uint8_t read8(uint16_t addr)
{
    return *((volatile uint8_t*)addr);
}


void ReadDieId( void)
{
    uint8_t  i;
    uint8_t  tag;
    uint8_t  length;
    uint16_t ptr = TLV_BASE_ADDR;

    while( ptr < TLV_END_ADDR)
    {
        tag    = read8(ptr);
        length = read8(ptr + 1);

        if( tag == 0xFF)
        {
            break;              // end-of-TLV marker
        }

        // Lot/Wafer ID = 4 bytes; Die X position = 2 bytes;
        // Die Y position = 2 bytes; Test results = 2 bytes;
        if( tag == TAG_DIE_ID)
        {
            if(length == DIE_ID_LENGTH)
            {
                for( i = 0; i < DIE_ID_LENGTH; i++)
                {
                    SerialNumber[i] = read8(ptr + 2 + i);
                }

                // Found it so we're done
                return;
            }
        }

        ptr += (2 + length);
    }
}


#if 0   // Pushing pause on coming up on battery

//----------------------------------------------------------------------------
// StayDown
//----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// If coming up on battery, put the processor into LPM4.5 mode until power is
// restored.
//
//-----------------------------------------------------------------------------
void StayDown( void)
{
    // Stop watchdog timer
    WDTCTL = WDTPW + WDTHOLD;
#if 0   // 4/14/22
    SFRIFG1 &= ~NMIIE;                      // NMI flag cleared
    SFRRPCR |= SYSNMI;                      // Set to NMI function
    SFRRPCR &= ~SYSNMIIES;                  // Interrupt on rising edge
    SFRIE1  |= NMIIE;                       // NMI enabled
#endif

    DisablePorts();
    SetVCore( 0);                           // This should walk the power down from 3 to 0

    // Setup to enable LPM4.5 mode
    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode to activate previously configured port settings

    // Open PMM registers for write access
    PMMCTL0_H = PMMPW_H;                    // PMM Password

    // Disable SVS
    SVSMHCTL &= ~(SVMHE + SVSHE + SVSHFP);  // Disable High side SVS + Normal mode (per Errata below)
    SVSMLCTL &= ~(SVMLE + SVSLE + SVSLFP);  // Disable Low side SVS + Normal mode (per Errata below)

    PMMIFG &= ~PMMLPM5IFG;                  // Clear flag to allow wakeup from LPM4.5

    // This turns off the PMM to allow for LPM4.5
    PMMCTL0_L |= PMMREGOFF;                 // Set Flag to enter LPM4.5 with LPM4 request

    // Per MSP430F552x, MSP430F551x Device Errata sheet SLAZ054L -
    // Any write to the SVSMxCTL register must be followed by a settling delay
    // (PMMIFG.SVSMLDLYIFG = 0 and PMMIFG.SVSMHDLYIFG = 0) before entering LPM2,
    // LPM3, or LPM4

    // Wait until SVM high side and SVM low side is settled
    while( (PMMIFG & SVSMHDLYIFG) == 0 || (PMMIFG & SVSMLDLYIFG) == 0);

    // Clear already set flags
    PMMIFG &= ~(SVSMLDLYIFG + SVSMHDLYIFG);
///    DELAY_US( 150);

    // Lock PMM registers for write access
    PMMCTL0_H = 0x00;

    // Enter LPM4.5 mode
    __bis_SR_register( LPM4_bits + GIE);    // Enter LPM4, enable interrupts
    __no_operation();

    // Wakeup
    PM5CTL0 &= ~LOCKIO;                     // Clear LOCKIO an enable ports'
 }

#endif
