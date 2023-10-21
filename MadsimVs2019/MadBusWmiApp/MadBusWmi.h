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
/*  Exe file ID  : MadWmiApp.exe                                               */
/*                                                                             */
/*  Module  NAME : MadBusWmi.h                                                 */
/*                                                                             */
/*  DESCRIPTION  : Function prototypes, #include(s)                            */
/*                 Derived from WDK-Toaster\Wmi                                */
/*                                                                             */
/*******************************************************************************/


#include <windows.h>
#include <comutil.h>
#include <wbemcli.h>
#include <stdio.h>
#include <tchar.h>
#include <comdef.h>

#define ARRAY_SIZE(_X_)     (sizeof((_X_))/sizeof((_X_)[0]))

//These WMI Class names *MUST* agree with the class definitions in MadBus.mof  --> MadBus.bmf, MadBusMof.h
//
#define MADBUS_INFO_CLASS         L"MadBusWmiInfo"
#define MADBUS_INFO_ERRORCOUNT    L"ErrorCount"
//
#define MADBUS_MAD_CONTROL_CLASS     L"MadBusWmiControl"
#define MADBUS_MAD_CONTROL_METHOD_1  L"MadBusWmiControl1"
#define MADBUS_MAD_CONTROL_METHOD_2  L"MadBusWmiControl2"
#define MADBUS_MAD_CONTROL_METHOD_3  L"MadBusWmiControl3"
#define IN_DATA1_VALUE          25
#define IN_DATA2_VALUE          30
//
bool ParseCommandLine(__in ULONG Argc, __in_ecount(Argc) PWCHAR Argv[], 
                      __deref_out_opt PWCHAR* ComputerName, __deref_out_opt PWCHAR* UserId, __deref_out_opt PWCHAR* Password, __deref_out_opt PWCHAR* DomainName);

void DisplayUsage();
//
HRESULT GetAndSetValuesInClass(__in IWbemServices* WbemServices, __in_opt PWCHAR UserId, __in_opt PWCHAR Password, __in_opt PWCHAR DomainName);

// Private methods.
//
HRESULT ExecuteMethodOneInInstance(__in IWbemServices* WbemServices, __in IWbemClassObject* ClassObj, __in const BSTR InstancePath, __in ULONG InData);

HRESULT
ExecuteMethodTwoInInstance(__in IWbemServices* WbemServices, __in IWbemClassObject* ClassObj, __in const BSTR InstancePath,
                           __in ULONG InData1, __in ULONG InData2);

HRESULT ExecuteMethodThreeInInstance(__in IWbemServices* WbemServices, __in IWbemClassObject* ClassObj, __in const BSTR InstancePath,
	                                 __in ULONG InData1, __in ULONG InData2);

HRESULT SetInterfaceSecurity(__in IUnknown* InterfaceObj, __in_opt PWCHAR UserId, __in_opt PWCHAR Password, __in_opt PWCHAR DomainName);

HRESULT ExecuteMethodsInClass(__in IWbemServices* WbemServices, __in_opt PWCHAR UserId, __in_opt PWCHAR Password, __in_opt PWCHAR DomainName);

HRESULT GetAndSetValuesInClass(__in IWbemServices* WbemServices, __in_opt PWCHAR UserId, __in_opt PWCHAR Password, __in_opt PWCHAR DomainName);
