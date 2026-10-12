//__________________________________________________________________________
//  Main.h
//__________________________________________________________________________
// Copyright 2019 PlayAGS, Inc.
// http://www.playags.com
//
// Program Description:
//
// Main header file for the Cabinet I/O Module board.
//
// Release 6.0
//    -Initial Revision
//    -18 September 2019
//
//__________________________________________________________________________


#ifndef MAIN_H_
#define MAIN_H_

//_____ D E F I N I T I O N S ______________________________________________

#define NMI_SHUTDOWN            1           // Comment out if using traditional reset

#define FW_VERSION_MAJOR        6           // Firmware revision - major
#define FW_VERSION_MINOR        15           //         "         - minor

// LED definitions
#define LED_HEART_BEAT          (BIT0)      // Green LED on P4
#define LED_USB_ACTIVITY        (BIT1)      // Red    "  on P4
#define LED_MASK                (0x03)      // BIT0 - BIT3

#define TIMER_COUNTER           24000/6     // Drives the Timer1 interrupt a 1msec
#define HEART_BEAT_RUNTIME      500         // Flashing rate at half second rate
#define HEART_BEAT_BITE         250         // Quarter second flash rate
#define HEART_BEAT_NO_USB       125         // Eighth second flash rate
#define HEART_BEAT_ERROR        1000        // 1 second flash rate
#define MSEC_10                 10          // 10 msec (actually we're spinning at 8ms)
#define MSEC_20                 20          // 20 msec
#define MSEC_50                 50          // 50 msec
#define MSEC_65                 65          // 65 msec
#define SEC_10_0                10000l      // 10 sec
#define DIE_ID_LENGTH           (0x0A)      // 10 bytes on most F5xx parts

// Unique processor identifier (serial number)
#define TLV_BASE_ADDR           (0x1A00)        // TLV device descriptors
#define TLV_END_ADDR            (0x1AFF)
#define TAG_DIE_ID              (0x08)
#define DIE_ID_LENGTH           (0x0A)           // 10 bytes on most F5xx parts

// Debouncing parameters (matching PDCM implementation)
#define MIN_PRESS_TIME_MS       20          // Minimum time button must be pressed
#define MIN_RELEASE_TIME_MS     100         // Minimum time button must be released

#define ADC_SAMPLES             8           // Number of samples to produce average
                                            // 3.3 * 2 = 6.6 because of voltage divider
                                            // 2^12bits = 4096
#define ADC_MIN_BAT_VOLT        0x5D3       // 2*(3.3/4096) = 0.00161  (2.4v/0.00161 = 0x5D3)
#define ADC_VOLT_MULTIPLIER     0.00161
#define ADC_TEMP_MULTIPLIER     0.0625
#define FIX_PT_SCALER           100.0
#define FIX_PT_EXPO_FACTOR      2
#define FORMER_N2_BYTES         8

#define FAN_0                   0
#define FAN_1                   1
#define FAN_2                   2
#define NUM_FANS                3
#define NUM_1_WIRE_BUSSES       3

// Gameside bitmappings
#define OUTPUT_13               0x00002000l
#define PLAY_LAMP               OUTPUT_13

#define GPIO_MASK               0x0F
#define BACKPLANE_DETECT_MASK   (BIT5)
#define MECH_BELL               (BIT0)
#define POWER_CYCLER            (BIT3)
#define WIRELESS_CHARGER_BIT    (BIT1)
#define PLAY_SWITCH_BIT         (BIT2)
#define PLAY_LAMP_BIT           (POWER_CYCLER)  // Double duty, power cycle flag only at startup

#define BELL_PERIOD             0
#define BELL_FREQ               1
#define BELL_ON                 2
#define BELL_TIMES              3
#define BELL_DELAY              4
#define BELL_MAX_SEQ_ENTRIES    5

// Platform definitions
#define CIOM                    0x08
#define PLATFORM_MASK           0xFC
#define PLATFORM_CIOM           (0xC5000020)    // CIOM "macola" number

#define MSB_LONG                (0x80000000l)   // Most significant bit in a long
#define LSB_LONG                (0x00000001l)   // Least significant bit in a long

#define NUM_BYTES               4
union MIXED_TYPES
{
    BYTE    bVal[ NUM_BYTES];
    ULONG   lVal;
};

// Intrusion bits - "PowerOffEvents" & "PowerOnEvents"
#define V_INTRUSION             (BIT0)      // Required for positive logic detection
#define INTRUSION_0             (0x01)      // Main door
#define INTRUSION_1             (0x02)      // Logic door
#define INTRUSION_2             (0x04)      // Belly door
#define INTRUSION_3             (0x08)      // Cash door
#define INTRUSION_4             (0x10)      // Spare door1
#define INTRUSION_5             (0x20)      // Cable present
#define INTRUSION_6             (0x40)      // Spare door2
#define NUM_INTRUSIONS          (7)

#define LOST_SYSTEM_POWER       (0x80)

// Commands used to communicate with this device
enum
{
    READ_VERSION_CMD            = 0x00,
    READ_RAW_INPUTS_CMD         = 0x01,
    READ_EVENTS_CMD             = 0x02,
    REARM_EVENTS_CMD            = 0x03,
    READ_REARM_EVENTS_CMD       = 0x04,

    READ_ID_CMD                 = 0x0A,
    LAMP_CONTROL_CMD            = 0x0C,
    SET_LAMP_STATE_CMD          = 0x0D,
    FLASH_LAMP_CMD              = 0x0E,

    READ_SYS_HEALTH_CMD         = 0x17,
    EXTERNAL_FAN_CONTROL_CMD    = 0x18,
    READ_SYS_STATUS_CMD         = 0x19,

    CONFIG_PLAY_BUTTON          = 0x20,     // Not used on gameside
    STATUS_PLAY_BUTTON          = 0x21,     //          "
    CONFIG_CHARGER              = 0x22,
    STATUS_CHARGER              = 0x23,

    READ_PRESS_COUNTS_CMD       = 0xA0,     // Read button press counters
    RESET_PRESS_COUNTS_CMD      = 0xA1,     // Clear button press counters
  
    UPDATE_FIRMWARE_CMD         = 0xE0,
    CONFIG_GPIO                 = 0xE1,
    SET_GPIO                    = 0xE2,
    GET_GPIO                    = 0xE3,
    BACKPLANE_DETECT            = 0xE4,
    MECH_BELL_PLAY              = 0xE5,
    MECH_BELL_PLAY_SEQ          = 0xE6,
    MECH_BELL_SET_SEQ           = 0xE7,

    RETRIEVE_RESPONSE_CMD       = 0xE8,     // Keep track of the last status reply (i.e., success or error code)
    SHUT_DOWN_CMD               = 0xE9,
    GET_SERIAL_NUMBER_CMD       = 0xEA,
    
    SET_INTRUSION_POLARITY      = 0xF1,     // Ontario requires positive logic, prior to this we used negative logic
    GET_INTRUSION_POLARITY      = 0xF2,
    POWER_CYCLE_CMD             = 0xF3,
    GET_CABINET_TYPE            = 0xF4,
    SET_CABINET_TYPE            = 0xF5
};


// Error conditions related to the IO commands
enum
{
    ERROR_INVALID_SUPPORT_TYPE  = 0x8000,   // 0x8000
    ERROR_INVALID_TYPE,                     // 0x8001
    ERROR_INVALID_NUMBER,                   // 0x8002
    ERROR_INVALID_TIMEOUT_VALUE,            // 0x8003
    ERROR_NOT_PRESENT,                      // 0x8004
    ERROR_NOT_ENABLED,                      // 0x8005
    ERROR_BUSY,                             // 0x8006
    ERROR_FEATURE_NOT_SUPPORTED,            // 0x8007
    ERROR_DEVICE_ERROR,                     // 0x8008
    ERROR_INSUFFICIENT_RESERVE,             // 0x8009
    ERROR_HARDWARE_NOT_SUPPORTED,           // 0x800A
    ERROR_COMMAND_NOT_SUPPORTED,            // 0x800B
    ERROR_NULL,                             // 0x800C - Null pointer parameter
    ERROR_INPUT_TOO_LONG,                   // 0x800D - Input data too long
    ERROR_INVALID_CMD,                      // 0x800E - Invalid USB command
    ERROR_INVALID_BUTTON_INDEX              // 0x800F - Index greater than valid range
};

// Board operating states
enum
{
    PS_SLEEPING,
    PS_OPERATIONAL,
    PS_START_ON_BATTERY
};

// Error conditions related to the board
enum
{
    ERROR_NONE                  = 0x00,
    ERROR_OVER_TEMP_LIMIT,
    ERROR_FAN_PROBLEM,
    ERROR_ONE_WIRE_PROBLEM,
    ERROR_FAN_READ_PROBLEM
};


// NVRAM Table Structures
typedef struct
{
    BYTE    DeckConfig;                     // Default is 5-select halo deck
    BYTE    CabinetType;
} NVRAM_STRUCT;


//_____ M A C R O S ________________________________________________________

#define is_BITE_enabled()       (~P8IN & BIT0)


//_____ D E C L A R A T I O N S ____________________________________________

extern           NVRAM_STRUCT TempStorage;
extern __no_init NVRAM_STRUCT NVRAM_START;
extern __no_init BYTE         NVRAM_END;

extern BOOL   IntrusionLogic;
extern BOOL   DeBounceFlag;
extern BOOL   PlayButtonInUse;
extern BOOL   WirelessChargeInUse;
extern BYTE   GpioDir;
extern BYTE   ErrorNumber;
extern BYTE   ErrorCulprit;
extern BYTE   ExternalFanLevel;
extern BYTE   Schedule;
extern BYTE   SerialNumber[];

extern USHORT AvgVoltage;
extern USHORT PDSignature;
extern USHORT MechBellOn;
extern USHORT MechBellOnCnt;
extern USHORT MechBellFreq;
extern USHORT MechBellFreqCnt;
extern USHORT MechBellPeriod;
extern USHORT MechBellPeriodCnt;
extern USHORT MechBellTimes;
extern USHORT MechBellTimesCnt;
extern USHORT MechBellDelay;
extern USHORT MechBellDelayCnt;
extern USHORT MechBellParms[][5];
extern ULONG  BeatLoop;
extern ULONG  SpinTimer;
extern ULONG  ms_counter;
extern ULONG  BoardID;
extern ULONG DeBounceCnt;

extern void PowerEventHdlr( void);
extern void InitSystem( void);
extern void InitUser( void);
extern void InitClock( void);
extern void InitTimerA1( void);
extern void InitADC( void);
extern void InitPorts( void);
extern void DisablePorts( void);
extern void UpdateNvRam( void);
extern void BatteryMonitor( void);
extern void ReadDieId( void);
extern void StayDown( void);
extern ULONG get_ms_time(void);

#endif /* MAIN_H_ */
