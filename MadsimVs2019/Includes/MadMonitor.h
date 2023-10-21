/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : Meta-Abstract Device Simulation Subsystem                   */
/*  COPYRIGHT    : (c) 2004 HTF CONSULTING                                     */
/*                                                                             */
/*******************************************************************************/
/*                                                                             */
/*  Exe file ID  : MAD_Monitor.exe                                             */
/*                                                                             */
/*  Module  NAME : MAD_Monitor.h                                               */
/*  DESCRIPTION  : Header file with macros, defines &                          */
/*                 function prototypes for the PnP, PM Monitor program         */
/*                                                                             */
/*                                                                             */
/*******************************************************************************/

/*
/* Comment about this function
*/
#ifndef __NOTIFY_H
#define __NOTIFY_H

// The structure to keep the device info
//
#define MAX_DEVICENAME 100
#define MAX_DEVICEPATH MAX_DEVICENAME+50
#define UNICODE 1

#define ID_EDIT 1

#define  IDM_OPEN   	100
#define  IDM_CLOSE  	101
#define  IDM_EXIT   	102
#define  IDM_HIDE   	103
#define  IDM_PLUGIN 	104
#define  IDM_UNPLUG 	105
#define  IDM_EJECT  	106
#define  IDM_ENABLE  	107
#define  IDM_DISABLE  	108

#define IDD_DIALOG      115
#define IDD_DIALOG1     116
#define IDD_DIALOG2     117
#define ID_OK	        118
#define ID_CANCEL   	119
#define IDC_SERIALNO   1000
#define IDC_DEVICEID   1001
#ifndef IDC_STATIC
#define IDC_STATIC   	 -1
#endif

// Copied Macros from ntddk.h
//
#ifndef _WINDOWS
#define CONTAINING_RECORD(address, type, field) ((type *)( \
						  (PCHAR)(address) - \
						  (ULONG_PTR)(&((type *)0)->field)))
#endif

typedef enum {PLUGIN = 1, UNPLUG, EJECT} USER_ACTION_TYPE;

#define InitializeListHead(ListHead) (\
	(ListHead)->Flink = (ListHead)->Blink = (ListHead))

#define RemoveHeadList(ListHead) \
	(ListHead)->Flink;\
	{RemoveEntryList((ListHead)->Flink)}

#define IsListEmpty(ListHead) \
	((ListHead)->Flink == (ListHead))

#define RemoveEntryList(Entry) {\
	PLIST_ENTRY _EX_Blink;\
	PLIST_ENTRY _EX_Flink;\
	_EX_Flink = (Entry)->Flink;\
	_EX_Blink = (Entry)->Blink;\
	_EX_Blink->Flink = _EX_Flink;\
	_EX_Flink->Blink = _EX_Blink;\
	}

#define InsertTailList(ListHead,Entry) {\
	PLIST_ENTRY _EX_Blink;\
	PLIST_ENTRY _EX_ListHead;\
	_EX_ListHead = (ListHead);\
	_EX_Blink = _EX_ListHead->Blink;\
	(Entry)->Flink = _EX_ListHead;\
	(Entry)->Blink = _EX_Blink;\
	_EX_Blink->Flink = (Entry);\
	_EX_ListHead->Blink = (Entry);\
	}

typedef struct _DEVICE_INFO
	{
	HANDLE		hDevice; // file handle
	HDEVNOTIFY	hHandleNotification; // notification handle
	TCHAR		DeviceName[MAX_DEVICENAME];// friendly name of device description
	TCHAR		DevicePath[MAX_DEVICEPATH];// 
	ULONG		ulSerialNo; // Serial number of the device.
	BOOL        bActive;
	LIST_ENTRY	ListEntry;
	} DEVICE_INFO,* PDEVICE_INFO;

typedef struct _DIALOG_RESULT
	{
	ULONG	SerialNo;
	PWCHAR	DeviceId;
	} DIALOG_RESULT,* PDIALOG_RESULT;


LRESULT FAR PASCAL WndProc(HWND hwnd, UINT message, WPARAM wParam,
	LPARAM lParam); 

BOOLEAN EnumExistingDevices(HWND   hWnd);

BOOL HandleDeviceInterfaceChange(HWND hwnd, DWORD evtype,
	PDEV_BROADCAST_DEVICEINTERFACE dip);

BOOL HandleDeviceChange(HWND hwnd, DWORD evtype, PDEV_BROADCAST_HANDLE dhp);

LRESULT HandleCommands(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOLEAN Cleanup(HWND hWnd);

//BOOL GetDeviceDescription(LPTSTR InputName, LPTSTR OutBuffer, PULONG SerialNo);

BOOLEAN OpenBusInterface(IN ULONG SerialNo, IN PWCHAR DeviceId,
	IN USER_ACTION_TYPE Action);

INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#endif


