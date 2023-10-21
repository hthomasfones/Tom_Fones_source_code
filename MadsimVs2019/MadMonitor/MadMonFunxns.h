/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : Model-Abstract-Demo Device Simulation Subsystem             */
/*  COPYRIGHT    : (c) 2004 HTF CONSULTING                                     */
/*                                                                             */
/*******************************************************************************/
/*                                                                             */
/*  Exe file ID  : MAdMonitor.exe                                              */
/*                                                                             */
/*  Module  NAME : MadMonFunxns.h                                              */
/*  DESCRIPTION  : Function prototypes fot the utility functions for           */
/*                 the Windows SDK program which monitors Plug-n-Play & PM     */
/*                                                                             */
/*                                                                             */
/*                                                                             */
/*******************************************************************************/


#define UNICODE 1
#define INITGUID

//#include <windows.h>
//#include <stdlib.h>
//#include <string.h>
//#include <setupapi.h>
//#include <dbt.h>
//#include <winioctl.h>


//* Function prototypes
//*
BOOL HandlePowerBroadcast(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT HandleCommands(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL HandleDeviceInterfaceChange(HWND hWnd, DWORD evtype, PDEV_BROADCAST_DEVICEINTERFACE dip);
BOOL HandleDeviceChange(HWND hWnd, DWORD evtype, PDEV_BROADCAST_HANDLE dhp);
BOOLEAN EnumExistingDevices(HWND hWnd);
BOOLEAN CleanupPdevList(HWND hWnd);
BOOLEAN OpenBusInterface(IN ULONG SerialNo, IN PWCHAR DeviceId, IN USER_ACTION_TYPE Action);
_inline BOOLEAN IsValid(ULONG No);
_inline void LB_Insert(PWCHAR pFormatStr, PWCHAR pArg);
void Log_Event(char* szEventText, char* szDevName, ULONG  ulSerialNo);
void Init_DevBrdcast_Hndl(PDEV_BROADCAST_HANDLE pDevbrdcastHndl,
						  USHORT uDevType, HANDLE hDevice);  
BOOLEAN Chek_File_Update(char szFilename[], POWERNOTIFY_TYPE *pPowerAxn, 
                         short int nLen, time_t *ptLastChek);
void 	Analyze_Power_Action(POWERNOTIFY_TYPE *pPrevPowerAxn,
							 POWERNOTIFY_TYPE *pCurPowerAxn);
