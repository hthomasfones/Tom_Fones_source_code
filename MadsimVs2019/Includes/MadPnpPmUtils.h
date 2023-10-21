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
/*  Exe file ID  : MadTestApp.exe, MadEnum.exe, MadMonitor.exe                 */
/*                                                                             */
/*  Module  NAME : MadPnpPmUtils.h                                             */
/*                                                                             */
/*  DESCRIPTION  : Function prototypes                                         */
/*                 working with a simulation                                   */
/*                 Derived from WDK-Toaster\exe\*                              */
/*                                                                             */
/*******************************************************************************/

#define MAX_DEVICENAME 100
#define MAX_DEVICEPATH MAX_DEVICENAME+50

#ifndef DEVINFO
typedef struct _DEVICE_INFO
{
	HANDLE		hDevice; // file handle
	HDEVNOTIFY	hHandleNotification; // notification handle
	TCHAR		DeviceName[MAX_DEVICENAME];// friendly name of device description
	TCHAR		DevicePath[MAX_DEVICEPATH];// 
	ULONG		ulSerialNo; // Serial number of the device.
	BOOL        bActive;
	LIST_ENTRY	ListEntry;
} DEVICE_INFO, * PDEVICE_INFO;
#define DEVINFO
#endif

//*** EnumExistingDevices
//*
BOOLEAN GetPnpDevice(LPGUID pInterfaceGuid, ULONG ulSerNum, PDEVICE_INFO pDevInfo);

//* Hoping to retrieve friendly name or device description and then Serial Num
//*
BOOL GetDeviceDescription(LPTSTR DevPath, LPTSTR OutBuffer, PULONG pSerialNo);




	







