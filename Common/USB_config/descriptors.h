// (c)2010 by Texas Instruments Incorporated, All Rights Reserved.
/*-----------------------------------------------------------------------------+
|                                                                              |
|                              Texas Instruments                               |
|                                                                              |
|    This is an automatically generated script by MSP430 USB Descriptor Tool   |
|                                                                              |
|    Descriptor Tool Version: 3.0.10                                           |
|    Date: 2011/02/24 16:32:05                                                 |
|                                                                              |
|                                Descriptor.h                                  |
|-----------------------------------------------------------------------------*/

#ifndef _DESCRIPTORS_H_
#define _DESCRIPTORS_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------------------------------------------+
| Include files                                                                |
|-----------------------------------------------------------------------------*/
#include <USB_API/USB_Common/types.h>

//***********************************************************************************************
// CDC or HID - Define both for composite support
//***********************************************************************************************
#define _CDC_          // Needed for CDC interface

//***********************************************************************************************
// Define the endpoint to be used
//***********************************************************************************************
#define _ENDPOINT_1_

//***********************************************************************************************
// CONFIGURATION CONSTANTS
//***********************************************************************************************
// These constants configure the API stack and help define the USB descriptors.
// Refer to Sec. 6 of the MSP430 USB CDC API Programmer's Guide for descriptions of these constants.

// Configuration Constants that can change
// #define that relates to Device Descriptor
#define USB_VID               	0x173B        		// Vendor ID (VID)
#define USB_PID					0x0003				// Place holder for PID
#define USB_PID_IPIOCB         	0x0003        		// Product ID (PID)
#define USB_PID_PDM            	0x0004        		// 		"
#define USB_PID_IOM            	0x0005        		// 		"
#define USB_PID_FSM            	0x0006        		// 		"
#define USB_PID_IDM            	0x0007        		// 		"
#define USB_PID_PDM2           	USB_PID_PDM    		// 		"
#define USB_PID_CIOM            0x000A              //      "
#define USB_PID_HMM             0x000B              //      "
#define USB_PID_PDC             0x000C              //      "
#define USB_PID_CLS3           	0x0035        		// 		"
extern  USHORT	ProductID;

/*----------------------------------------------------------------------------+
| Firmware Version                                                            |
| How to detect version number of the FW running on MSP430?                   |
| on Windows Open ControlPanel->Systems->Hardware->DeviceManager->Ports->     |
|         Msp430->ApplicationUART->Details                                    |
+----------------------------------------------------------------------------*/
#define VER_FW_H              	0x02          		// Device release number, in binary-coded decimal
#define VER_FW_L              	0x00          		// Device release number, in binary-coded decimal
// If a serial number is to be reported, set this to the index within the string descriptor
// of the dummy serial number string.  It will then be automatically handled by the API.
// If no serial number is to be reported, set this to 0.
#define USB_STR_INDEX_SERNUM  	3             


#define DESCRIPTOR_TOTAL_LENGTH 0x20       			// wTotalLength, This is the sum of configuration descriptor length
#define USB_NUM_INTERFACES      1           		// Number of implemented interfaces.

#define CDC0_COMM_INTERFACE     0           		// Comm interface number of CDC0
#define CDC0_DATA_INTERFACE     1           		// Data interface number of CDC0

#ifdef _ENDPOINT_1_
	#define CDC0_OUTEP_ADDR     0x01       			// Output Endpoint Address of CDC0
	#define CDC0_INEP_ADDR      0x81       			// Input Endpoint Address of CDC0
#else
	#define CDC0_OUTEP_ADDR     0x02       			// Output Endpoint Address of CDC0
	#define CDC0_INEP_ADDR      0x82       			// Input Endpoint Address of CDC0
#endif

#define CDC_NUM_INTERFACES      1           		//  Total Number of CDCs implemented. should set to 0 if there are no CDCs implemented.
#define HID_NUM_INTERFACES      0           		//  Total Number of HIDs implemented. should set to 0 if there are no HIDs implemented.
#define MSC_NUM_INTERFACES      0           		//  Total Number of MSCs implemented. should set to 0 if there are no MSCs implemented.
// Interface numbers for the implemented CDSs and HIDs, This is to use in the Application(main.c) and in the interupt file(UsbIsr.c).
#define CDC0_INTFNUM            0
#define MSC_MAX_LUN_NUMBER      1       			// Maximum number of LUNs supported

#define USB_OUTEP_INT_EN 		BIT0 | BIT1 | BIT2
#define USB_INEP_INT_EN  		BIT0 | BIT1 | BIT2 
// MCLK frequency of MCU, in Hz
// For running higher frequencies the Vcore voltage adjustment may required.
// Please refer to Data Sheet of the MSP430 device you use
#define USB_MCLK_FREQ 			24000000            // MCLK frequency of MCU, in Hz
///#define USB_MCLK_FREQ 			8000000             // MCLK frequency of MCU, in Hz
#define USB_PLL_XT        		2                  	// Defines which XT is used by the PLL (1=XT1, 2=XT2)
#define USB_XT_FREQ       		USBPLL_SETCLK_4_0	// Indicates the freq of the crystal on the oscillator indicated by USB_PLL_XT
#define USB_DISABLE_XT_SUSPEND 	1            		// If non-zero, then USB_suspend() will disable the oscillator
                                             		// that is designated by USB_PLL_XT; if zero, USB_suspend won't
                                             		// affect the oscillator
#define USB_DMA_CHAN            0x00        		// Set to 0xFF if no DMA channel will be used 0..7 for selected DMA channel



// Controls whether the remote wakeup feature is supported by this device.
// A value of 0x20 indicates that is it supported (this value is the mask for
// the bmAttributes field in the configuration descriptor).
// A value of zero indicates remote wakeup is not supported.
// Other values are undefined, as they will interfere with bmAttributes.
#define USB_SUPPORT_REM_WAKE 	0x20	/// tedo 27June13 0x00

// Controls whether the application is self-powered to any degree.  Should be
// set to 0x40, unless the USB device is fully supplied by the bus.
#define USB_SUPPORT_SELF_POWERED 0x00	/// tedo 16July12 0x40

// Controls what the device reports to the host regarding how much power it will
// consume from VBUS.  Expressed in 2mA units; that is, the number of mA
// communicated is twice the value of this field.
#define USB_POWER 				0x00	/// 16July12 0xFA ///0x32

//Configuration constants that can not change ( Fixed Values)
#define CDC_CLASS 				2
#define HID_CLASS 				3
#define MSC_CLASS 				4

#define MAX_PACKET_SIZE   		0x40              // Max size of the USB packets.

//***********************************************************************************************
// DESCRIPTOR CONSTANTS
//***********************************************************************************************
#define SIZEOF_DEVICE_DESCRIPTOR  0x12
#define SIZEOF_REPORT_DESCRIPTOR  36
#define USBHID_REPORT_LENGTH      64  // length of whole HID report (including Report ID)
#define CONFIG_STRING_INDEX       4
#define INTF_STRING_INDEX         5
#define USB_CONFIG_VALUE          0x01

//***********************************************************************************************
// OUTWARD DECLARATIONS
//***********************************************************************************************

//Calculates the endpoint descriptor block number from given address
#define EDB(addr) ((addr&0x07)-1)

/* Structure for generic part of configuration descriptor */
struct abromConfigurationDescriptorGenric
{
	BYTE sizeof_config_descriptor;            // bLength
 	BYTE desc_type_config;                    // bDescriptorType: 2
	BYTE sizeof_configuration_descriptor1;    // wTotalLength
	BYTE sizeof_configuration_descriptor2;
	BYTE usb_num_configurations;              // bNumInterfaces
	BYTE bconfigurationvalue;                 // bConfigurationValue
	BYTE config_string_index;                 // iConfiguration Description offset
 	BYTE bmattributes;                        // bmAttributes, bus power, remote wakeup
	BYTE usb_max_power;                       // Max. Power Consumption at 2mA unit
};

/************************************************CDC Descriptor**************************/
#if 1
struct abromConfigurationDescriptorCdc
{
// interface descriptor (9 bytes)
    BYTE blength_intf;	                      // blength: interface descriptor size
    BYTE desc_type_interface;	              // bdescriptortype: interface
    BYTE interface_number_cdc;                // binterfacenumber
    BYTE balternatesetting;                   // balternatesetting: alternate setting
    BYTE bnumendpoints;                       // bnumendpoints: three endpoints used
    BYTE binterfaceclass;                     // binterfaceclass: communication interface class
    BYTE binterfacesubclass;                  // binterfacesubclass: abstract control model
    BYTE binterfaceprotocol;                  // binterfaceprotocol: common at commands 
    BYTE intf_string_index;	                  // interface:

// Bulk out end point related fields
    BYTE sizeof_outep_descriptor;             // blength: endpoint descriptor size
    BYTE desc_type_outep;	                  // bdescriptortype: endpoint
    BYTE cdc_outep_addr;	                  // bendpointaddress: (out3)
    BYTE outep_desc_attr_type_bulk;	          // bmattributes: bulk
    BYTE outep_wmaxpacketsize1;
    BYTE outep_wmaxpacketsize2;               // wmaxpacketsize, 64 bytes
    BYTE outep_binterval; 	                  // binterval: ignored for bulk transfer

// Bulk in related fields
    BYTE sizeof_inep_descriptor;              // blength: endpoint descriptor size
    BYTE desc_type_inep;	                  // bdescriptortype: endpoint
    BYTE cdc_inep_addr;	                      // bendpointaddress: (in3)
    BYTE inep_desc_attr_type_bulk;	          // bmattributes: bulk
    BYTE inep_wmaxpacketsize1;
    BYTE inep_wmaxpacketsize2;  		      // wmaxpacketsize, 64 bytes
    BYTE inep_binterval;	                  // binterval: ignored for bulk transfer
}	;
#else
struct abromConfigurationDescriptorCdc
{
// interface descriptor (9 bytes)
    BYTE blength_intf;	                      // blength: interface descriptor size
    BYTE desc_type_interface;	              // bdescriptortype: interface
    BYTE interface_number_cdc;                // binterfacenumber
    BYTE balternatesetting;                   // balternatesetting: alternate setting
    BYTE bnumendpoints;                       // bnumendpoints: three endpoints used
    BYTE binterfaceclass;                     // binterfaceclass: communication interface class
    BYTE binterfacesubclass;                  // binterfacesubclass: abstract control model
    BYTE binterfaceprotocol;                  // binterfaceprotocol: common at commands 
    BYTE intf_string_index;	                  // interface:
//header functional descriptor
    BYTE blength_header;	                  // blength: endpoint descriptor size
    BYTE bdescriptortype_header;	          // bdescriptortype: cs_interface
    BYTE bdescriptorsubtype_header;	          // bdescriptorsubtype: header func desc
    BYTE bcdcdc1;
    BYTE bcdcdc2;	                          // bcdcdc: spec release number

//call managment functional descriptor
    BYTE bfunctionlength;	                  // bfunctionlength
    BYTE bdescriptortype_c;	                  // bdescriptortype: cs_interface
    BYTE bdescriptorsubtype_c;	              // bdescriptorsubtype: call management func desc
    BYTE bmcapabilities;	                  // bmcapabilities: d0+d1
    BYTE intf_number_cdc;                     // bdatainterface: 0

//acm functional descriptor
    BYTE bfunctionlength_acm;	              // bfunctionlength
    BYTE bdescriptortype_acm;	              // bdescriptortype: cs_interface
    BYTE bdescriptorsubtype_acm;	          // bdescriptorsubtype: abstract control management desc
    BYTE bmcapabilities_acm;	              // bmcapabilities

// Union Functional Descriptor
    BYTE bLength_ufd;                         // Size, in bytes
    BYTE bdescriptortype_ufd;                 // bDescriptorType: CS_INTERFACE
    BYTE bdescriptorsubtype_ufd;              // bDescriptorSubtype: Union Functional Desc
    BYTE bmasterinterface_ufd;                // bMasterInterface -- the controlling intf for the union
    BYTE bslaveinterface_ufd;                 // bSlaveInterface -- the controlled intf for the union

//Interrupt end point related fields
    BYTE sizeof_epintep_descriptor;           // blength: endpoint descriptor size
    BYTE desc_type_epintep;	                  // bdescriptortype: endpoint
    BYTE cdc_intep_addr;	                  // bendpointaddress: (in2)
    BYTE epintep_desc_attr_type_int;	      // bmattributes: interrupt
    BYTE epintep_wmaxpacketsize1;
    BYTE epintep_wmaxpacketsize;   		      // wmaxpacketsize, 64 bytes
    BYTE epintep_binterval;                   // binterval

// Data interface descriptor (9 bytes)
    BYTE blength_slaveintf;	                  // blength: interface descriptor size
    BYTE desc_type_slaveinterface;	          // bdescriptortype: interface
    BYTE interface_number_slavecdc;           // binterfacenumber
    BYTE balternatesetting_slave;             // balternatesetting: alternate setting
    BYTE bnumendpoints_slave;                 // bnumendpoints: three endpoints used
    BYTE binterfaceclass_slave;               // binterfaceclass: data interface class
    BYTE binterfacesubclass_slave;            // binterfacesubclass: abstract control model
    BYTE binterfaceprotocol_slave;            // binterfaceprotocol: common at commands
    BYTE intf_string_index_slave;	          // interface:

// Bulk out end point related fields
    BYTE sizeof_outep_descriptor;             // blength: endpoint descriptor size
    BYTE desc_type_outep;	                  // bdescriptortype: endpoint
    BYTE cdc_outep_addr;	                  // bendpointaddress: (out3)
    BYTE outep_desc_attr_type_bulk;	          // bmattributes: bulk
    BYTE outep_wmaxpacketsize1;
    BYTE outep_wmaxpacketsize2;               // wmaxpacketsize, 64 bytes
    BYTE outep_binterval; 	                  // binterval: ignored for bulk transfer

// Bulk in related fields
    BYTE sizeof_inep_descriptor;              // blength: endpoint descriptor size
    BYTE desc_type_inep;	                  // bdescriptortype: endpoint
    BYTE cdc_inep_addr;	                      // bendpointaddress: (in3)
    BYTE inep_desc_attr_type_bulk;	          // bmattributes: bulk
    BYTE inep_wmaxpacketsize1;
    BYTE inep_wmaxpacketsize2;  		      // wmaxpacketsize, 64 bytes
    BYTE inep_binterval;	                  // binterval: ignored for bulk transfer
}	;
#endif



/* Global structure having Generic,CDC,HID, MSC structures */
struct  abromConfigurationDescriptorGroup
{
    /* Generic part of config descriptor */
    const struct abromConfigurationDescriptorGenric abromConfigurationDescriptorGenric;
    const struct abromConfigurationDescriptorCdc stCdc[CDC_NUM_INTERFACES];
};

extern const struct  abromConfigurationDescriptorGroup abromConfigurationDescriptorGroup;
extern BYTE abromDeviceDescriptor[SIZEOF_DEVICE_DESCRIPTOR];
extern BYTE const abromStringDescriptor[];
extern BYTE const abromReportDescriptor[SIZEOF_REPORT_DESCRIPTOR];

/* Handle Structure - Will be populated in descriptors.c based on number of CDC,HID interfaces */
struct tUsbHandle
{
    BYTE ep_In_Addr;               // Input EP Addr 
    BYTE ep_Out_Addr;              // Output EP Addr 
    BYTE edb_Index;                // The EDB index 
    BYTE dev_Class;                // Device Class- 2 for CDC, 3 for HID 
    WORD intepEP_X_Buffer;         // Interupt X Buffer Addr 
    WORD intepEP_Y_Buffer;         // Interupt Y Buffer Addr 
    WORD oep_X_Buffer;             // Output X buffer Addr 
    WORD oep_Y_Buffer;             // Output Y buffer Addr 
    WORD iep_X_Buffer;             // Input X Buffer Addr 
    WORD iep_Y_Buffer;             // Input Y Buffer Addr 
};

extern const struct tUsbHandle stUsbHandle[CDC_NUM_INTERFACES + HID_NUM_INTERFACES + MSC_NUM_INTERFACES]; 
extern const tDEVICE_REQUEST_COMPARE tUsbRequestList[];

#ifdef __cplusplus
}
#endif

#endif

/*------------------------ Nothing Below This Line --------------------------*/

