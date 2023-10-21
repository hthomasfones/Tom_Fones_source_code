/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : MAD Device Simulation Framework                             */
/*  COPYRIGHT    : (c) 2022 HTF Consulting                                     */
/*                                                                             */
/* This source code is provided by exclusive license for non-commercial use to */
/* XYZ Company                                                                 */
/*                                                                             */
/*******************************************************************************/
/*                                                                             */
/*  Exe file ID  : MadBus.sys, MadDevice.sys, MadSimUI.exe, MadTestApp.exe,    */
/*                 MadEnum.exe, MadMonitor.exe, MadWmi.exe                     */
/*                                                                             */
/*  Module  NAME : MadGUIDs.h                                                  */
/*                                                                             */
/*  DESCRIPTION  : Definitions of all GUIDs used in the above programs         */
/*                 Derived from WDK-Toaster\*                                  */
/*                                                                             */
/*******************************************************************************/

// GUID definition are required to be outside of header inclusion pragma
// to avoid error during precompiled headers.
DEFINE_GUID(GUID_MADDEVICE_INTERFACE_STANDARD,
            0xe0b27630, 0x5434, 0x11d3, 0xb8, 0x90, 0x0, 0xc0, 0x4f, 0xad, 0x51, 0x71);
// {E0B27630-5434-11d3-B890-00C04FAD5171}

// Define an Interface Guid for bus enumerator class.
// This GUID is used to register (IoRegisterDeviceInterface) 
// an instance of an interface so that enumerator application 
// can send an ioctl to the bus driver.
DEFINE_GUID (GUID_DEVINTERFACE_MADBUS_ONE, 
             0xfa8eb396, 0x3eee, 0x4469, 0xbd, 0x5f, 0x8b, 0x4b, 0x61, 0x26, 0xb3, 0x7f);
//* {fa8eb396-3eee-4469-bd5f-8b4b6126b37f}

//And a 2nd bus device interface for a 2nd bus instance
DEFINE_GUID(GUID_DEVINTERFACE_MADBUS_TWO,
            0x1e241f9a, 0x258, 0x4f11, 0x84, 0x9f, 0xe2, 0xd7, 0x5c, 0x2, 0x1e, 0x82);
// {1E241F9A-0258-4F11-849F-E2D75C021E82}

// Define an Interface Guid for the Model-Abstract-Demo device class.
// This GUID is used to register (IoRegisterDeviceInterface) 
// an instance of an interface so that user application 
// can control the device.
DEFINE_GUID (GUID_DEVINTERFACE_MADDEVICE, //MAD_DEVICE_INTERFACE_CLASS, 
	         0xb4ee4371, 0xf4ba, 0x4fd9, 0x99, 0x7b, 0x5e, 0xa6, 0x58, 0xe0, 0x03, 0x0b);
//* {b4ee4371-f4ba-4fd9-997b-5ea658e0030b}

//* The (Setup) Class GUID for a generic BUS Driver is the well-known
//* class SYSTEM - with the GUID of: 4D36E97D-E325-11CE-BFC1-08002BE10318 
//* See the .inf file

//*******************************************************************
// Define a Setup Class GUID for Abstract Class. This is same
// as the Abstract_Device CLASS guid in the INF file(s).
DEFINE_GUID(GUID_DEVCLASS_MADDEVICE,  
            0xc07f84c6, 0x9092, 0x4dbe, 0xab, 0xab, 0x46, 0x5d, 0x3d, 0x99, 0xe5, 0xf2);
//* {c07f84c6-9092-4dbe-abab-465d3d99e5f2}

//*******************************************************************
// Define a Setup Class GUID for the scsi disk.
// This is an alias for the well-known ScsiAdapter class guid
DEFINE_GUID(GUID_DEVCLASS_MADDISK,
            0x4d36e97b, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18);
// {4d36e97b-e325-11ce-bfc1-08002be10318}
 
//*********************************************************************

// Define an Interface Guid to access the proprietary abstract bus interface.
// This guid is used to identify a specific interface in IRP_MN_QUERY_INTERFACE
// handler.
DEFINE_GUID(GUID_MAD_INTERFACE_STANDARD, 
            0x9390a1da, 0xba77, 0x44aa, 0x97, 0x14, 0xe2, 0xb0, 0xd2, 0x9b, 0xb6, 0x21);
//* {9390a1da-ba77-44aa-9714-e2b0d29bb621}

// Define a Guid for the abstract bus type. This is returned in response to
// IRP_MN_QUERY_BUS_INTERFACE on PDO.
DEFINE_GUID(GUID_MAD_BUS_TYPE_UNDEF, 
            0xdce62eb6, 0x5c51, 0x413d, 0x87, 0xec, 0x16, 0xac, 0x27, 0x78, 0xf5, 0xc0);

// WMI GUIDs are produced by MofComp.exe of MadDevice.mof & found in MadDeviceMof.h
//* Define a Guid for the WMI Bus Data
// Define a WMI GUID to get maddev device info.
// Define a WMI GUID to represent device arrival notification WMIEvent class.

// GUID definition are required to be outside of header inclusion pragma to avoid
// error during precompiled headers.
