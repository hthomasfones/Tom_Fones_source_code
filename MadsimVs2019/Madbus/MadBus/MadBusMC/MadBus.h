/**********************************************************************/
/*                                                                    */
/*  PRODUCT      : Model-Abstract-Demo Device Simulation Subsystem    */
/*  COPYRIGHT    : (c) 2015 HTF CONSULTING                            */
/*                                                                    */
/**********************************************************************/
/*                                                                    */
/*  Exe file ID  : MadBus.sys                                         */
/*                                                                    */
/*  Module  NAME : MadBus.mc                                          */
/*  DESCRIPTION  : Message Compiler source file for defining          */
/*                 Event Log messages                                 */
/*                                                                    */
/*                                                                    */
/**********************************************************************/
/*
Abstract:

    Constant definitions for the I/O error code log values.

*/
#ifndef _HotIILOG_
#define _HotIILOG_

//
//  Status values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-------------------------+-------------------------------+
//  |Sev|C|       Facility          |               Code            |
//  +---+-+-------------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//

//These are really Locale-Ids - see below
//http://msdn.microsoft.com/en-us/library/windows/desktop/dd318693(v=vs.85).aspx
//If you are looking in the MC.exe generated header file you are probably lost
//Please look in the .MC file

//
//  Values are 32 bit values laid out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
#define FACILITY_RPC_STUBS               0x3
#define FACILITY_RPC_RUNTIME             0x2
#define FACILITY_MADBUS_INFO_CODE        0x7
#define FACILITY_IO_ERROR_CODE           0x4


//
// Define the severity codes
//
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_ERROR            0x3


//
// MessageId: MADBUS_DRIVER_LOAD
//
// MessageText:
//
// The Model-Abstract-Demo bus driver has loaded.
//
#define MADBUS_DRIVER_LOAD               ((NTSTATUS)0x60FF0001L)

//
// MessageId: MADBUS_DRIVER_UNLOAD
//
// MessageText:
//
// The Model-Abstract-Demo bus driver has unloaded.
//
#define MADBUS_DRIVER_UNLOAD             ((NTSTATUS)0x60FF0002L)

//
// MessageId: MADBUS_DRIVER_LOAD_ERROR
//
// MessageText:
//
// The Model-Abstract-Demo bus driver encountered a load error.
//
#define MADBUS_DRIVER_LOAD_ERROR         ((NTSTATUS)0xE0FF0003L)

//
// MessageId: MADBUS_CREATE_DEVICE_ERROR
//
// MessageText:
//
// The Model-Abstract-Demo bus driver encountered a device create error.
//
#define MADBUS_CREATE_DEVICE_ERROR       ((NTSTATUS)0xE0FF0004L)

//
// MessageId: MADBUS_CREATE_CHILD_DEVICE_ERROR
//
// MessageText:
//
// The Model-Abstract-Demo bus driver encountered a device create error.
//
#define MADBUS_CREATE_CHILD_DEVICE_ERROR ((NTSTATUS)0xE0FF0005L)

//
// MessageId: MADBUS_HARDWARE_FAULT
//
// MessageText:
//
// The Model-Abstract-Demo bus device had a hard error.
//
#define MADBUS_HARDWARE_FAULT            ((NTSTATUS)0xE0FF0006L)

//
// MessageId: MADBUS_WMI_REGISTRATION_ERROR
//
// MessageText:
//
// The Model-Abstract-Demo bus device had a hard error.
//
#define MADBUS_WMI_REGISTRATION_ERROR    ((NTSTATUS)0xE0FF0007L)

#endif /* _HotIILOG_ */
