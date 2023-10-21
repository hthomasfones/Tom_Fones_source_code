/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : Model-Abstract-Demo Device Simulation Subsystem             */
/*  COPYRIGHT    : (c) 2014 HTF CONSULTING                                     */
/*                                                                             */
/*******************************************************************************/
/*                                                                             */
/*  Exe file ID  : MadMonitor.exe                                              */
/*                                                                             */
/*  Module  NAME : MadMonitor.c                                                */
/*  DESCRIPTION  : A Windows SDK program to monitor Plug-n-Play & Power Mngt.  */
/*                 activity and present a log of activity in the client area   */
/*                                                                             */
/*                                                                             */
/*******************************************************************************/

#define POWER
#define ID_TIMER    1

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <setupapi.h>
#include <dbt.h>
#include <time.h>
#include <winioctl.h>
#include <initguid.h>
//#define PHYSICAL_ADDRESS LARGE_INTEGER
#include "..\Includes\MadDefinition.h"
#include "..\Includes\MadBusIoctls.h"
#include "..\Includes\PowerNotify.h"
#include "..\Includes\MadGUIDs.h"
#include "MadMonitor.h"
#include "MadMonFunxns.h"

// Global variables
//
HINSTANCE hInst;
HWND ghWndList;
TCHAR gszTitle[] = TEXT("Plug-n-Play / PM Monitor - MAD_Device(s)");
LIST_ENTRY gListHead = {NULL, NULL};
HDEVNOTIFY hInterfaceNotification;
TCHAR gOutText[500];
UINT gListBoxIndex = 0;
GUID gInterfaceGuid;// = GUID_ABSTRACT_PNP_DEVICE_INTERFACE_CLASS;
time_t gLastChekTime = 0;
short int gnTimerCount = 0;
BOOLEAN gbSysDownActv = FALSE;
POWERNOTIFY_TYPE gPowerAxn = INIT_POWERAXN;
BOOLEAN gbPlugged[10] = {FALSE, FALSE, FALSE, FALSE, FALSE,
                         FALSE, FALSE, FALSE, FALSE, FALSE};


//*** Here begins main
//*
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	               LPSTR lpszCmdParam, int nCmdShow)

{
static TCHAR szAppName[] = TEXT("MAD_Monitor.exe");
HWND hWnd;
MSG msg;
UINT_PTR uTimerId;
WNDCLASS wndclass;

	gInterfaceGuid = GUID_DEVINTERFACE_MADDEVICE; //GUID_MAD_DEVICE_INTERFACE_CLASS;
	hInst = hInstance;

	if (!hPrevInstance)
		{
		wndclass.style = CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc = WndProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 0;
		wndclass.hInstance = hInstance;
		wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndclass.hbrBackground = GetStockObject(WHITE_BRUSH);
		wndclass.lpszMenuName = TEXT("GenericMenu");
		wndclass.lpszClassName = szAppName;

		RegisterClass(&wndclass);
		}

	hWnd = CreateWindow(szAppName, gszTitle, WS_OVERLAPPEDWINDOW,
		   	            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
             		   	CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

    uTimerId = SetTimer(hWnd, ID_TIMER, (UINT_PTR)100, NULL); //*100 msecs.

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

    _tzset(); //* Initialize time - Adjusting for time zone
    time(&gLastChekTime);	

	while (GetMessage(&msg, NULL, 0, 0))
		{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		}

	return (0);
}


LRESULT FAR PASCAL WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)

{
static char szPowerAxn[] = POWERAXNFILE;
#ifdef DEVNODE
    static WCHAR wcInfoMsg[] = L"DBT_DEVNODES_CHANGED";
    static WCHAR wcFormat[]  = L"%ws";
    static char szInfoMsg[]  = "DBT_DEVNODES_CHANGED";
#endif  
	
DWORD nEventType = (DWORD)wParam; 
PDEV_BROADCAST_HDR pDevAlertHedr = (PDEV_BROADCAST_HDR)lParam;
DEV_BROADCAST_DEVICEINTERFACE DevBrdcastInterface;
BOOLEAN bRC;
int nRC;
POWERNOTIFY_TYPE PowerAxn;
long int nSize = sizeof(POWERNOTIFY_TYPE);
char szInfoMsg[50] = "";
//time_t *pTime = &gLastChekTime;

	switch (message)
		{
		case WM_COMMAND:
			HandleCommands(hWnd, message, wParam, lParam);
			return 0;

		case WM_CREATE:
			ghWndList = CreateWindow(TEXT("listbox"), NULL,
					   	            (WS_CHILD|WS_VISIBLE|LBS_NOTIFY|WS_VSCROLL|WS_BORDER),
             					   	CW_USEDEFAULT, CW_USEDEFAULT,
					   	            CW_USEDEFAULT, CW_USEDEFAULT,
					              	hWnd, (HMENU)ID_EDIT, hInst, NULL);

			DevBrdcastInterface.dbcc_size       = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
			DevBrdcastInterface.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
			DevBrdcastInterface.dbcc_classguid  = gInterfaceGuid;
			hInterfaceNotification = RegisterDeviceNotification(hWnd,
				                                                &DevBrdcastInterface, 0);
			InitializeListHead(&gListHead);
			EnumExistingDevices(hWnd);
            //sprintf(szInfoMsg, "Size of DEVICE_INFO = %d", sizeof(DEVICE_INFO));
			//MessageBox(NULL, szInfoMsg, NULL, MB_ICONINFORMATION);
			return 0;

		case WM_SIZE:
			MoveWindow(ghWndList, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
			return 0;

		case WM_SETFOCUS:
			SetFocus(ghWndList);
			return 0;

        case WM_TIMER:
            if (!gbSysDownActv)
				{
				gnTimerCount++;
				if (gnTimerCount < 20)
					return 0;
				}
			
            //* Check for recent Power Activity logging
			gnTimerCount = 0;
                        bRC = Chek_File_Update(szPowerAxn, &PowerAxn,
                                               (short int)nSize, &gLastChekTime);
	        if (!bRC)
				return 0; 

            //* Check for recent power state change 
			nRC = memcmp((PVOID)&gPowerAxn, (PVOID)&PowerAxn, nSize);
			if (nRC == 0) //* Compares equal - no change
				{
				//MessageBox(NULL, "Equal compare", NULL, MB_ICONINFORMATION);

				return 0;
				}

            //* Analyze recent Power Activity - and display results
			Analyze_Power_Action(&gPowerAxn, &PowerAxn); 
			memcpy((PVOID)&gPowerAxn, (PVOID)&PowerAxn, nSize);

            return 0;

		case WM_DEVICECHANGE:
#ifdef DEVNODE
// The DBT_DEVNODES_CHANGED broadcast message is sent 
// everytime a device is added or removed. This message
// is typically handled by Device Manager kind of apps, 
// which uses refresh it's window whenever something changes.
// The lParam is always NULL in this case.
//
			//if (wParam == DBT_DEVNODES_CHANGED)
				//{
        		//LB_Insert(wcInfoMsg, NULL);
				//SendMessage(ghWndList, LB_INSERTSTRING, gListBoxIndex,
					        (LPARAM)wcInfoMsg);
				//gListBoxIndex++;
				//}
#endif  		   

			if (pDevAlertHedr == NULL)
				return 0;

			if (pDevAlertHedr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
				HandleDeviceInterfaceChange(hWnd, nEventType,
					                        (PDEV_BROADCAST_DEVICEINTERFACE)pDevAlertHedr);
			else 
				if (pDevAlertHedr->dbch_devicetype == DBT_DEVTYP_HANDLE)
					HandleDeviceChange(hWnd, nEventType,
					                   (PDEV_BROADCAST_HANDLE)pDevAlertHedr);
					
			return 0;

#ifdef POWER			
// If you want to see all the POWER broadcast then
// turn on the following code. This app doesn't do
// anything special during suspend/resume. Read 
// platform SDK docs for further info.
//
		case WM_POWERBROADCAST:
			HandlePowerBroadcast(hWnd, wParam, lParam);
			return 0;   		 
#endif

		case WM_CLOSE:
			CleanupPdevList(hWnd);
			UnregisterDeviceNotification(hInterfaceNotification);
			return  DefWindowProc(hWnd, message, wParam, lParam);

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
	} //* end switch

	return DefWindowProc(hWnd, message, wParam, lParam);
}


