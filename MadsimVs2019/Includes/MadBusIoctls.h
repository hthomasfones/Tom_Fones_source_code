/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : MAD Device Simulation Framework                             */
/*  COPYRIGHT    : (c) 2013 HTF Consulting                                     */
/*                                                                             */
/* This source code is provided by exclusive license for non-commercial use to */
/* XYZ Company                                                                 */
/*                                                                             */
/*******************************************************************************/
/*                                                                             */
/*  Exe file ID  : MadBus.sys, MadSimUI.exe, MadEnum.exe                       */
/*                                                                             */
/*  Module  NAME : MadBusIoctl.h                                               */
/*                                                                             */
/*  DESCRIPTION  : Definitions of Ioctls supported by MadBus.sys               */
/*                 Derived from WDK-Toaster\bus                                */
/*                                                                             */
/*******************************************************************************/


#ifndef _MADBUSIOCTL_H
#define _MADBUSIOCTL_H

#define MADSIM_HARDWARE_ID1         L"MadBus1\\VEN_9808&DEV_1001\0"
#define MADSIM_HARDWARE_ID_LENGTH1  (sizeof(MADSIM_HARDWARE_ID1) / sizeof(WCHAR))
//
#define MADSIM_HARDWARE_ID2         L"MadBus2\\VEN_9808&DEV_2001\0"  
#define MADSIM_HARDWARE_ID_LENGTH2  (sizeof(MADSIM_HARDWARE_ID2) / sizeof(WCHAR))
#define MADSIM_HARDWARE_ID_MAX_LEN  \
        max(MADSIM_HARDWARE_ID_LENGTH1, MADSIM_HARDWARE_ID_LENGTH2)

#define FILE_DEVICE_MADBUS FILE_DEVICE_BUS_EXTENDER

#undef  IOCTL_MADBUS_PLUGIN_HARDWARE
#undef  IOCTL_MADBUS_UNPLUG_HARDWARE
#undef  IOCTL_MADBUS_EJECT_HARDWARE
#undef  IOCTL_MADBUS_SET_POWER_STATE
#undef  IOCTL_MADBUS_DONT_DISPLAY_IN_UI_DEVICE

//************ IOCTL defines for interacting with client applications ********
//* Defines for Plug-n-Play operations
#define IOCTL_MADBUS(_index_) \
    CTL_CODE (FILE_DEVICE_MADBUS, _index_, METHOD_BUFFERED, (FILE_WRITE_DATA | FILE_READ_DATA))

#define IOCTL_MADBUS_PLUGIN_HARDWARE               IOCTL_MADBUS (0x0)
#define IOCTL_MADBUS_UNPLUG_HARDWARE               IOCTL_MADBUS (0x1)
#define IOCTL_MADBUS_EJECT_HARDWARE                IOCTL_MADBUS (0x2)
#define IOCTL_MADBUS_SET_POWER_STATE               IOCTL_MADBUS (0x3)
#define IOCTL_MADBUS_DONT_DISPLAY_IN_UI_DEVICE     IOCTL_MADBUS (0x4)

//* Defines for Device simulating operations
#define MADBUS_TYPE (ULONG)43000      	     // 32768..65535 "User defined" range
#define IOCTL_MADBUS_ENUM_BASE (USHORT)2833  // 2048..4095 "User defined" range

enum {MADBUS_ENUM_INITIALIZE  = IOCTL_MADBUS_ENUM_BASE,
      MADBUS_ENUM_MAP_WHOLE_DEVICE,
      MADBUS_ENUM_UNMAP_WHOLE_DEVICE,

#ifdef UI_WINDOW_WIRED // If the device is connected to a wire rather than storage & therefore I/O is visible in the UI
      MADBUS_ENUM_GETOUTPUT,
      MADBUS_ENUM_SETINPUT,
#endif //UI_WINDOW_WIRED

      //MADBUS_ENUM_MAP_WRITEBUFR,
     //MADBUS_ENUM_SET_INTR,
      //MADBUS_ENUM_GEN_INT,
      //MADBUS_ENUM_GETDEVICE,
      //MADBUS_ENUM_SETDEVICE,
      //MADBUS_ENUM_SET_TRACE,
      //MADBUS_ENUM_GET_TRACE,
      //MADBUS_ENUM_ALLOC_MEM,
      //MADBUS_ENUM_RELEASE_MEM,
      //MADBUS_ENUM_ASSERT,
      //MADBUS_ENUM_BREAK,
      //MADBUS_ENUM_EXCEPTION,
      //MADBUS_ENUM_HANG,
      //MADBUS_ENUM_VERIFIER
      };

#define IOCTL_MADBUS_INITIALIZE \
    CTL_CODE(MADBUS_TYPE,  MADBUS_ENUM_INITIALIZE, METHOD_BUFFERED, \
    		FILE_WRITE_DATA | FILE_READ_DATA)

    #define IOCTL_MADBUS_MAP_WHOLE_DEVICE  \
        CTL_CODE(MADBUS_TYPE,  MADBUS_ENUM_MAP_WHOLE_DEVICE, METHOD_BUFFERED, \
    	         FILE_WRITE_DATA | FILE_READ_DATA)

    #define IOCTL_MADBUS_UNMAP_WHOLE_DEVICE  \
        CTL_CODE(MADBUS_TYPE,  MADBUS_ENUM_UNMAP_WHOLE_DEVICE, METHOD_BUFFERED, \
    	         FILE_WRITE_DATA | FILE_READ_DATA)

#ifdef UI_WINDOW_WIRED // If the device is connected to a wire rather than storage & therefore I/O is visible in the UI
    #define IOCTL_MADBUS_GETOUTPUT  \
        CTL_CODE(MADBUS_TYPE,  MADBUS_ENUM_GETOUTPUT, METHOD_BUFFERED, \
    	             FILE_WRITE_DATA | FILE_READ_DATA)

    #define IOCTL_MADBUS_SETINPUT  \
        CTL_CODE(MADBUS_TYPE,  MADBUS_ENUM_SETINPUT, METHOD_BUFFERED, \
    	             FILE_WRITE_DATA | FILE_READ_DATA)
#endif //UI_WINDOW_WIRED

   //#define IOCTL_MADBUS_MAP_WRITEBUFR  \
    //    CTL_CODE(MADBUS_TYPE, MADBUS_ENUM_MAP_WRITEBUFR, METHOD_BUFFERED, \
    //	             FILE_WRITE_DATA | FILE_READ_DATA)

// #define IOCTL_MADBUS_GEN_INT \
//    CTL_CODE(MADBUS_TYPE,  MADBUS_ENUM_GEN_INT, METHOD_BUFFERED, \
//    		FILE_WRITE_DATA | FILE_READ_DATA)

//#define IOCTL_MADBUS_GETDEVICE  \
//    CTL_CODE(MADBUS_TYPE,  MADBUS_ENUM_GETDEVICE, METHOD_BUFFERED, \
//    		FILE_WRITE_DATA | FILE_READ_DATA)

//#define IOCTL_MADBUS_SETDEVICE  \
//    CTL_CODE(MADBUS_TYPE,  MADBUS_ENUM_SETDEVICE, METHOD_BUFFERED, \
//    		FILE_WRITE_DATA | FILE_READ_DATA)

//  #define IOCTL_MADBUS_GET_TRACE  \
//    CTL_CODE(MADBUS_TYPE,  MADBUS_ENUM_GET_TRACE, METHOD_BUFFERED, \
//    		FILE_READ_DATA)

//#define IOCTL_MADBUS_SET_TRACE  \
//    CTL_CODE(MADBUS_TYPE,  MADBUS_ENUM_SET_TRACE, METHOD_BUFFERED, \
//    		FILE_WRITE_DATA)

//#define IOCTL_MADBUS_ALLOC_MEM  \
//    CTL_CODE(MADBUS_TYPE, MADBUS_ENUM_ALLOC_MEM, METHOD_BUFFERED,  \
//             FILE_READ_DATA)

//#define IOCTL_MADBUS_RELEASE_MEM  \
//    CTL_CODE(MADBUS_TYPE, MADBUS_ENUM_RELEASE_MEM, METHOD_BUFFERED, \
//			 FILE_READ_DATA)

//#define IOCTL_MADBUS_ASSERT  \
//    CTL_CODE(MADBUS_TYPE, MADBUS_ENUM_ASSERT, METHOD_BUFFERED, FILE_READ_DATA)

//#define IOCTL_MADBUS_BREAK  \
//    CTL_CODE(MADBUS_TYPE, MADBUS_ENUM_BREAK, METHOD_BUFFERED, FILE_READ_DATA)

//#define IOCTL_MADBUS_EXCEPTION  \
//    CTL_CODE(MADBUS_TYPE, MADBUS_ENUM_EXCEPTION, METHOD_BUFFERED, FILE_READ_DATA)

//#define IOCTL_MADBUS_HANG  \
//    CTL_CODE(MADBUS_TYPE, MADBUS_ENUM_HANG, METHOD_BUFFERED, FILE_READ_DATA)

//#define IOCTL_MADBUS_VERIFIER  \
//    CTL_CODE(MADBUS_TYPE, MADBUS_ENUM_VERIFIER, METHOD_BUFFERED, FILE_READ_DATA)


//**************  Data structures used in PlugIn and UnPlug ioctls *************
typedef struct _MADBUS_PLUGIN_HARDWARE
{
    // sizeof (struct _MADBUS_HARDWARE)
    IN ULONG Size;                          
    
    // Unique serial number of the device to be enumerated.
    // Enumeration will be failed if another device on the 
    // bus has the same serail number.
    IN ULONG SerialNo;

    // An array of (zero terminated wide character strings). The array itself
    //  also null terminated (ie, MULTI_SZ)
    //IN  WCHAR   HardwareIDs[MADSIM_HARDWARE_IDS_LENGTH /*1*/]; 
    IN  WCHAR   HardwareIDs[MADSIM_HARDWARE_ID_MAX_LEN];

} MADBUS_PLUGIN_HARDWARE, *PMADBUS_PLUGIN_HARDWARE;

typedef struct _MADBUS_UNPLUG_HARDWARE
{
    // sizeof (struct _REMOVE_HARDWARE)
     IN ULONG Size;                                    

    // Serial number of the device to be plugged out    
    ULONG   SerialNo;
    
    ULONG Reserved[2];    

} MADBUS_UNPLUG_HARDWARE, *PMADBUS_UNPLUG_HARDWARE;

typedef struct _MADBUS_EJECT_HARDWARE
{
    // sizeof (struct _EJECT_HARDWARE)
     IN ULONG Size;                                    

    // Serial number of the device to be ejected
    ULONG   SerialNo;
    
    ULONG Reserved[2];    

} MADBUS_EJECT_HARDWARE, *PMADBUS_EJECT_HARDWARE;

typedef struct _MADBUS_SET_POWER_STATE
{
    IN ULONG Size;                                    

    // Serial number of the device to be ejected
    //
    ULONG   SerialNo;
    
    LONG nDevPowerState;    

} MADBUS_SET_POWER_STATE, *PMADBUS_SET_POWER_STATE;

typedef struct _MADBUS_MAP_WHOLE_DEVICE
    {
	PHYSICAL_ADDRESS  liDeviceRegs;
    PVOID             pDeviceRegs;
    PVOID             pPioRead;
    PVOID             pPioWrite;
    PVOID             pDeviceData;
    } MADBUS_MAP_WHOLE_DEVICE, *PMADBUS_MAP_WHOLE_DEVICE;

// Define Interface reference/dereference routines for
//  Interfaces exported by IRP_MN_QUERY_INTERFACE
typedef VOID (*PINTERFACE_REFERENCE)(PVOID Context);
typedef VOID (*PINTERFACE_DEREFERENCE)(PVOID Context);

typedef BOOLEAN (*PMADBUS_GET_POWER_LEVEL)(IN PVOID Context, OUT PUCHAR Level);

typedef BOOLEAN (*PMADBUS_SET_POWER_LEVEL)(IN PVOID Context, OUT UCHAR Level);

typedef BOOLEAN (*PMADBUS_IS_CHILD_PROTECTED)(IN PVOID Context);

// Interface for getting and setting power level etc.,
typedef struct _MADBUS_INTERFACE_STANDARD {
   USHORT                           Size;
   USHORT                           Version;
   PINTERFACE_REFERENCE             InterfaceReference;
   PINTERFACE_DEREFERENCE           InterfaceDereference;
   PVOID                            Context;
   PMADBUS_GET_POWER_LEVEL     GetPowerLevel;
   PMADBUS_SET_POWER_LEVEL     SetPowerLevel;
   PMADBUS_IS_CHILD_PROTECTED  IsSafetyLockEnabled; //):
} MADBUS_INTERFACE_STANDARD,   *PMADBUS_INTERFACE_STANDARD;

#endif



