/*
 * 1-Wire implementation for MSP430
 *
 * @author: David Siroky <siroky@dasir.cz>
 * @license: MIT
 */

#include <msp430.h>
#include <string.h>

#include "USB_API/USB_Common/device.h"
#include "USB_API/USB_Common/types.h"          	// Basic Type declarations

#include "Delay.h"
#include "OneWire.h"


//#####################################################################
//#####################################################################

/// @return: 0 if ok
SHORT OneWire_Reset( ONEWIRE *ow)
{
	OneWire_LineLow(ow);
	DELAY_US(550); 			// 480us minimum
	OneWire_LineRelease(ow);

	DELAY_US(70); 			// slave waits 15-60us
	if( *(ow->port_in) & ow->pin)
	{
	    return 1; 			// line should be pulled down by slave
	}

	DELAY_US(300); 			// slave TX presence pulse 60-240us
	if( !(*(ow->port_in) & ow->pin))
	{
		return 2; 			// line should be "released" by slave
	}

	return 0;
}


//-----------------------------------------------------------------------------
// OneWire_Search
//-----------------------------------------------------------------------------
//
// Return Value : none
// Parameters   : ow - I/O port structure device is connected to
//				  id - 1-wire device identification number
//				  depth - used as part of the recursive search
//				  reset - when set performs a reset pulse
//
// If successful returns the device id for the 1-wire device found on the bus.
//
//-----------------------------------------------------------------------------
SHORT OneWire_Search( ONEWIRE *ow, BYTE *id, SHORT depth, SHORT reset)
{
  SHORT i, b1, b2;
  SHORT OWresult = 0;

  if( depth == 64)
  {
      // we have all 64 bit in this search branch
	  OWresult = 1;
	  return( OWresult);			// device found
  }

  if( reset)
  {
    if( OneWire_Reset(ow) != 0)
    {
    	OWresult = -1;
    	return( OWresult);			// reset failed
    }

    OneWire_WriteByte( ow, 0xF0); 	// search ROM command

    // send currently recognized bits
    for( i = 0; i < depth; i++)
    {
      b1 = OneWire_ReadBit(ow);
      b2 = OneWire_ReadBit(ow);
      OneWire_WriteBit(ow, id[i / 8] & (1 << (i % 8)));
    }
  }

  // check another bit
  b1 = OneWire_ReadBit(ow);
  b2 = OneWire_ReadBit(ow);
  if (b1 && b2)
  {
	  OWresult = -2;
	  return( OWresult);			// no response to search
  }

  if( !b1 && !b2) 	// two devices with different bits on this position
  {
    // check devices with this bit = 0
    OneWire_WriteBit(ow, 0);
    id[depth / 8] &= ~(1 << (depth % 8));
    OneWire_Search( ow, id, depth + 1, 0);

    // check devices with this bit = 1
    id[depth / 8] |= 1 << (depth % 8);
    OneWire_Search( ow, id, depth + 1, 1); // different branch, reset must be issued
  }
  else if (b1)
  {
    // devices have 1 on this position
    OneWire_WriteBit(ow, 1);
    id[depth / 8] |= 1 << (depth % 8);
    OneWire_Search( ow, id, depth + 1, 0);
  }
  else if (b2)
  {
    // devices have 0 on this position
	OneWire_WriteBit(ow, 0);
    id[depth / 8] &= ~(1 << (depth % 8));
    OneWire_Search( ow, id, depth + 1, 0);
  }

  return( OWresult);
}


//-----------------------------------------------------------------------------
// OneWire_ReadROM
//-----------------------------------------------------------------------------
//
// Return Value : none
// Parameters   : ow - I/O port structure device is connected to
//				  id - 1-wire device identification number
//				  depth - used as part of the recursive search
//				  reset - when set performs a reset pulse
//
// If successful returns the device id for the 1-wire device found on the bus.
//
//-----------------------------------------------------------------------------
SHORT OneWire_ReadROM( ONEWIRE *ow, BYTE *id, SHORT depth, SHORT reset)
{
  SHORT OWresult = 0;

  if( OneWire_Reset(ow) != 0)
  {
	  OWresult = -1;				// reset failed
  }
  else
  {
	  OneWire_WriteByte( ow, ONEWIRE_READ_ROM);
	  OneWire_Block(ow, id, 8 );
	  OWresult = 1;
  }

  return( OWresult);
}


//-----------------------------------------------------------------------------
// OneWire_WriteBit
//-----------------------------------------------------------------------------
//
// Return Value : none
// Parameters   : ow - I/O port structure device is connected to
//				  bit - value to send to 1-wire device
//
// Write a bit onto One Wire Bus. Provide 10us recovery time.
//
//-----------------------------------------------------------------------------
void OneWire_WriteBit( ONEWIRE *ow, SHORT bit)
{
	DELAY_US(2); 			// recovery, min 1us
	OneWire_LineLow(ow);
	if( bit)
	{
		DELAY_US( 6); 		// max 15us
	}
	else
	{
		DELAY_US( 64); 		// min 60us
	}
	OneWire_LineRelease(ow);

	// rest of the write slot
	if( bit)
	{
		DELAY_US( 64);
	}
	else
	{
		DELAY_US( 6);
	}
}


//-----------------------------------------------------------------------------
// OneWire_ReadBit
//-----------------------------------------------------------------------------
//
// Return Value : value read back from 1-wire device
// Parameters   : ow - I/O port structure device is connected to
//
// Read a bit from One Wire Bus. Provide 10us recovery time
//
//-----------------------------------------------------------------------------
SHORT OneWire_ReadBit( ONEWIRE *ow)
{
	SHORT bit;

	DELAY_US( 2); 				// recovery, min 1us
	OneWire_LineLow(ow);

	DELAY_US( 5); 				// hold min 1us
	OneWire_LineRelease(ow);

	DELAY_US( 6); 				// 15us window
	bit = *(ow->port_in) & ow->pin;
	DELAY_US( 60); 				// rest of the read slot

	return bit;
}


//-----------------------------------------------------------------------------
// OneWire_WriteByte
//-----------------------------------------------------------------------------
//
// Return Value : none
// Parameters   : ow - I/O port structure device is connected to
//
// Write a byte to the 1-wire device.
//
//-----------------------------------------------------------------------------
void OneWire_WriteByte( ONEWIRE *ow, BYTE byte)
{
	SHORT i;

	for(i = 0; i < 8; i++)
	{
		OneWire_WriteBit( ow, byte & 1);
		byte >>= 1;
	}
}


//-----------------------------------------------------------------------------
// OneWire_ReadByte
//-----------------------------------------------------------------------------
//
// Return Value : none
// Parameters   : ow - I/O port structure device is connected to
//
// Read a byte from the 1-wire device.
//
//-----------------------------------------------------------------------------
BYTE OneWire_ReadByte( ONEWIRE *ow)
{
  SHORT i;
  BYTE  byte = 0;

  for(i = 0; i < 8; i++)
  {
    byte >>= 1;
    if( OneWire_ReadBit(ow))
    {
    	byte |= 0x80;
    }
  }

  return byte;
}


//-----------------------------------------------------------------------------
// OneWire_TouchByte
//-----------------------------------------------------------------------------
//
// Return Value : data byte read back from device
// Parameters   : ow - I/O port structure device is connected to
//				  databyte - value to write out
//
// Write a byte onto One Wire Bus and sample result
//
//-----------------------------------------------------------------------------
BYTE OneWire_TouchByte( ONEWIRE *ow, BYTE databyte)
{
	BYTE result_tb, loop;

	result_tb = 0;
    for( loop = 0;loop < 8; loop++)
    {
        // shift the result to get it ready for the next bit
        result_tb >>= 1;                // ***************** this is broke!!!!!!!
        result_tb &= 0x7f;
        if( databyte & 0x01)            // if sending a one then also read a bit
        {
            if( OneWire_ReadBit( ow) == 1)
            {
                result_tb |= 0x80;
            }
        }
        else                    // else write a 0
        {
        	OneWire_WriteBit(ow, 0);
        }

        // shift the date byte for the next bit
        databyte >>= 1;
    }

    return result_tb;
}


//-----------------------------------------------------------------------------
// OneWire_Block
//-----------------------------------------------------------------------------
//
// Return Value : none
// Parameters   : ow - I/O port structure device is connected to
//				  datablock - data to write out to the device as well as to
//						receive.
//				  data_len - number of bytes to transmit
//
// Write a block of 1-wire data bytes and return sampled in same buffer
//
//-----------------------------------------------------------------------------
void OneWire_Block( ONEWIRE *ow, BYTE *datablock, SHORT data_len)
{
	SHORT loop;

    for( loop = 0;loop < data_len; loop++)
    {
        datablock[loop] = OneWire_TouchByte( ow, datablock[loop]);
    }
}


//-----------------------------------------------------------------------------
// OneWire_ReadTemp_DS1822
//-----------------------------------------------------------------------------
//
// Return Value : The 16 bit signed temperature read from the DS1822
// Parameters   : ow - I/O port structure device is connected to
//
//  This function will start a conversion and read back the temperature
//  from a DS1820 temperature sensor.
//
//-----------------------------------------------------------------------------
SHORT OneWire_ReadTemp_DS1822( ONEWIRE *ow)
{
	BYTE 	id[8];
    SHORT	temperature;

    // Reset, presence.
///    if( OneWire_Search( ow, id, 0, 1) < 0)
	if( OneWire_ReadROM( ow, id, 0, 1) < 0)
    {
        return DS1822_ERROR; // Error
    }


#if 0	// Don't want to do this now as it makes us
    	// wait too long for a result.  Read the previous
    	// result then kick off the next...
    // Send start conversion command.
    OneWire_WriteByte( ow, DS1822_START_CONVERSION);

    // Wait until conversion is finished.
    // Bus line is held low until conversion is finished.
    while( !OneWire_ReadBit( ow))
    {
    }

    // Reset, presence.
    if( OneWire_Search( ow, id, 0, 1) < 0)
    {
        return -1000; // Error
    }
#endif

    // Send READ SCRATCHPAD command.
    OneWire_WriteByte( ow, DS1822_READ_SCRATCHPAD);

    // Read only two first bytes (temperature low, temperature high)
    // and place them in the 16 bit temperature variable.
    temperature = OneWire_ReadByte( ow);
    temperature |= (OneWire_ReadByte( ow) << 8);

    // Reset, presence.
///    if( OneWire_Search( ow, id, 0, 1) < 0)
   	if( OneWire_ReadROM( ow, id, 0, 1) < 0)
    {
        return DS1822_ERROR; // Error
    }

    // Send start conversion command now so when we come back the next time
    // around the information will be ready
    OneWire_WriteByte( ow, DS1822_START_CONVERSION);

    return temperature;
}


#if 0  // was for AGP2
//-----------------------------------------------------------------------------
// OneWire_WriteBlock_DS2431
//-----------------------------------------------------------------------------
//
// Return Value : The 16 bit signed temperature read from the DS1822
// Parameters   : ow - I/O port structure device is connected to
//
//  This function writes the "macola" number into memory.  This is needed to
//  fake the AGP2 into thinking it's plugged into a certain type of button
//  deck.
//
//-----------------------------------------------------------------------------
SHORT OneWire_WriteBlock_DS2431( ONEWIRE *ow, ULONG MacolaNum)
{
	BYTE	i;
	BYTE	result, reTries;
	BYTE 	Block[16];

    // Write out the SmartID information
    for( i = 0; i < 3; i++)
    {
    	memset( Block, 0xFF, sizeof(Block));

    	// Reset, presence.
        if( OneWire_Reset( ow) != 0)
        {
        	return( DS2431_ERROR);
        }

        // Write to the scratchpad
    	Block[0] = ONEWIRE_SKIP_ROM;     		// Search ROM command
    	Block[1] = DS2431_WRITE_SCRATCHPAD;     // Write to the scratchpad
		Block[2] = i << 3;     					// Address (XX)
		Block[3] = 0x00;     					// address (0x00XX)

		// Writing out 3 8byte block
		switch( i)
		{
		case 0:
			Block[4] = (MacolaNum >> 24) & 0x000000FF;
			Block[5] = (MacolaNum >> 16) & 0x000000FF;
			Block[6] = (MacolaNum >> 8)  & 0x000000FF;
			Block[7] = MacolaNum         & 0x000000FF;
			break;
		case 1:
			break;
		case 2:
			Block[5] = DEV_BUTTON_DECK;
			break;
		}

		// Ship Write Scratchpad
		OneWire_Block( ow, Block, 14);

		// Reset, presence.
		OneWire_Reset( ow);

		// Read back the scratchpad
    	Block[0] = ONEWIRE_SKIP_ROM;     		// Search ROM command
    	Block[1] = DS2431_READ_SCRATCHPAD;      // Read to the scratchpad
		Block[2] = i << 3;     					// Address (XX)
		Block[3] = 0x00;     					// address (0x00XX)
		Block[4] = 0x07;     					// Read E/S

		// Ship Read Scratchpad
		OneWire_Block( ow, Block, 12);

		// Reset, presence.
		OneWire_Reset( ow);

		// Read back the scratchpad
    	Block[0] = ONEWIRE_SKIP_ROM;     		// Search ROM command
    	Block[1] = DS2431_COPY_SCRATCHPAD;      // Read to the scratchpad
		Block[2] = i << 3;     					// Address (XX)
		Block[3] = 0x00;     					// address (0x00XX)
		Block[4] = 0x07;     					// Read E/S

		// Ship Copy Scratchpad
		OneWire_Block( ow, Block, 5);

		// Wait for the copy to complete
	    do
	    {
	    	DELAY_MS( 4);
	        result = OneWire_TouchByte( ow, 0xFF);
	        if( ++reTries > 20)
	        {
	        	return( DS2431_ERROR);
	        }
	    } while( result != DS2431_COPY_SUCCESS);
    }

    return( 0);
}
#endif
