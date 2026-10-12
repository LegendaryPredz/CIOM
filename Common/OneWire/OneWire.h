//__________________________________________________________________________
//	OneWire.h
//__________________________________________________________________________
// Copyright 2012 Cadillac Jack, Inc.
// http://www.cadillacjack.com
//
// Program Description:
//
// Header file used to define function calls and data structures used to
// communicate with Maxim 1-wire devices.
//
// Release 4.0
//    -Initial Revision
//    -23 April 2012
//
// Original author: David Siroky <siroky@dasir.cz>
// License: MIT
//
//__________________________________________________________________________


#ifndef ONEWIRE_H_
#define ONEWIRE_H_

//_____  I N C L U D E S ___________________________________________________

#include <stdint.h>


//_____ D E F I N I T I O N S ______________________________________________

typedef struct onewire_t
{
    volatile BYTE *port_out;
    const volatile BYTE *port_in;
    volatile BYTE *port_ren;
    volatile BYTE *port_dir;
    int pin;
} ONEWIRE;

#define ONEWIRE_SKIP_ROM				0xCC
#define ONEWIRE_SEARCH_ROM				0xF0
#define ONEWIRE_MATCH_ROM				0x55
#define ONEWIRE_READ_ROM				0x33

#define DS1822_FAMILY_ID                0x22
#define DS1822_START_CONVERSION         0x44
#define DS1822_READ_SCRATCHPAD          0xBE
#define DS1822_ERROR                    -1000   // Return code. Outside temperature range.

#define DS2431_FAMILY_ID				0x2D
#define DS2431_WRITE_SCRATCHPAD			0x0F
#define DS2431_READ_SCRATCHPAD			0xAA
#define DS2431_COPY_SCRATCHPAD			0x55
#define DS2431_READ_MEMORY				0xF0
#define DS2431_MATCH_ROM				0x55
#define DS2431_COPY_SUCCESS				0xAA
#define DS2431_ERROR                    DS1822_ERROR

#define DEV_BUTTON_DECK					2		// Device type

//_____ M A C R O S ________________________________________________________

inline void OneWire_LineLow( ONEWIRE *ow)
{
  *(ow->port_dir) |= ow->pin;
  *(ow->port_out) &= ~ow->pin;
  *(ow->port_ren) &= ~ow->pin;
}


inline void OneWire_LineHigh( ONEWIRE *ow)
{
  *(ow->port_dir) |= ow->pin;
  *(ow->port_out) |= ow->pin;
  *(ow->port_ren) &= ~ow->pin;
}


inline void OneWire_LineRelease( ONEWIRE *ow)
{
  *(ow->port_dir) &= ~ow->pin; 	// input
  *(ow->port_ren) |= ow->pin;
  *(ow->port_out) |= ow->pin; 	// internal resistor pullup
}


//_____ D E C L A R A T I O N S ____________________________________________

SHORT OneWire_Reset( ONEWIRE *ow);
SHORT OneWire_Search( ONEWIRE *ow, BYTE *id, SHORT depth, SHORT reset);
SHORT OneWire_ReadROM( ONEWIRE *ow, BYTE *id, SHORT depth, SHORT reset);
void  OneWire_WriteBit( ONEWIRE *ow, SHORT bit);
SHORT OneWire_ReadBit( ONEWIRE *ow);
void  OneWire_WriteByte( ONEWIRE *ow, BYTE byte);
BYTE  OneWire_ReadByte( ONEWIRE *ow);
BYTE  OneWire_TouchByte( ONEWIRE *ow, BYTE databyte);
void  OneWire_Block( ONEWIRE *ow, BYTE *datablock, SHORT data_len);
SHORT OneWire_ReadTemp_DS1822( ONEWIRE *ow);


#if 0 	// was for AGP2
SHORT OneWire_WriteBlock_DS2431( ONEWIRE *ow, ULONG MacolaNum);


// SmartID configuration structure - partial, we don't need full to mimic button deck
typedef struct _MEM_CONFIG
{
    ULONG   MacolaNum;
    BYTE	TimeStamp[16];
    BYTE    BuildSite;
    BYTE    DevType;
///    BYTE    SerialNum[14];               // Leaving room for null char (only in 1-w DS2431)
///    BYTE    Spare[93];
} MEM_CONFIG;
#endif

#endif /* ONEWIRE_H_ */
