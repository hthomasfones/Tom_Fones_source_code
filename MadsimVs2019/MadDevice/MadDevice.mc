;/**********************************************************************/
;/*                                                                    */
;/*  PRODUCT      : Model-Abstract-Demo Device Simulation Subsystem    */
;/*  COPYRIGHT    : (c) 2014 HTF CONSULTING                            */
;/*                                                                    */
;/**********************************************************************/
;/*                                                                    */
;/*  Exe file ID  : MadDevice.sys                                      */
;/*                                                                    */
;/*  Module  NAME : MadDevice.mc                                       */
;/*  DESCRIPTION  : Message Compiler source file for defining          */
;/*                 Event Log messages                                 */
;/*                                                                    */
;/*                                                                    */
;/**********************************************************************/

;/*
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;*/

;#ifndef _HotIILOG_
;#define _HotIILOG_
;
;//
;//  Status values are 32 bit values layed out as follows:
;//
;//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
;//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
;//  +---+-+-------------------------+-------------------------------+
;//  |Sev|C|       Facility          |               Code            |
;//  +---+-+-------------------------+-------------------------------+
;//
;//  where
;//
;//      Sev - is the severity code
;//
;//          00 - Success
;//          01 - Informational
;//          10 - Warning
;//          11 - Error
;//
;//      C - is the Customer code flag
;//
;//      Facility - is the facility code
;//
;//      Code - is the facility's status code
;//
;
MessageIdTypedef=NTSTATUS

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0
               RpcRuntime=0x2:FACILITY_RPC_RUNTIME
               RpcStubs=0x3:FACILITY_RPC_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
               MadDevice=0x7:FACILITY_MADDEV_INFO_CODE)

MessageId=0x0001
Facility=MadDevice
Severity=Informational
SymbolicName=MADDEVICE_DRIVER_LOAD
Language=English
The Model-Abstract-Demo driver is loaded.
.

MessageId=0x0002
Facility=MadDevice
Severity=Informational
SymbolicName=MADDEVICE_DRIVER_UNLOAD 
Language=English
The Model-Abstract-Demo driver has unloaded.
.

MessageId=0x0003
Facility=MadDevice
Severity=Error
SymbolicName=MADDEVICE_DRIVER_LOAD_ERROR
Language=English
The Model-Abstract-Demo driver encountered a load error.
.

MessageId=0x0004
Facility=MadDevice
Severity=Error
SymbolicName=MADDEVICE_CREATE_DEVICE_ERROR
Language=English
The Model-Abstract-Demo device driver encountered a device create error.
.

MessageId=0x0005
Facility=MadDevice
Severity=Error
SymbolicName=MADDEVICE_HARDWARE_FAULT
Language=English
The Model-Abstract-Demo device had a hard error.
.

;#endif /* _HotIILOG_ */
