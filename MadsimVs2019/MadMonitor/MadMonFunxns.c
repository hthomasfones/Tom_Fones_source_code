/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : Model-Abstract-Demo Device Simulation Subsystem             */
/*  COPYRIGHT    : (c) 2014 HTF CONSULTING                                     */
/*                                                                             */
/*******************************************************************************/
/*                                                                             */
/*  Exe file ID  : MadMonitor.exe                                              */
/*                                                                             */
/*  Module  NAME : MadMonFunxns.c                                              */
/*  DESCRIPTION  : Utility functions for the Windows SDK program which monitors*/
/*                 Plug-n-Play & Power Mngt. activity                          */
/*                                                                             */
/*                                                                             */
/*******************************************************************************/

#include <windows.h>
//#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setupapi.h>
#include <dbt.h>
#include <winioctl.h>
#include <strsafe.h>
#include <fcntl.h>
#include <io.h>
#include <sys\stat.h>
#include <time.h>
//#define PHYSICAL_ADDRESS LARGE_INTEGER
#include "..\Includes\MadDefinition.h"
#include "..\Includes\MadBusIoctls.h"
#include "..\Includes\MadGUIDs.h"
#include "..\Includes\PowerNotify.h"
#include "resource.h"
#include "MadMonitor.h"
#define DEVINFO
#include "..\Includes\MadPnpPmUtils.h"
#include "MadMonFunxns.h"

// Global variables
//
extern HINSTANCE hInst;
extern HWND ghWndList;
extern TCHAR szTitle[];
extern LIST_ENTRY gListHead;
extern HDEVNOTIFY hInterfaceNotification;
extern TCHAR gOutText[];
extern UINT gListBoxIndex;
extern GUID gInterfaceGuid;
extern BOOLEAN gbSysDownActv;
extern BOOLEAN gbPlugged[];

//* Global but local to this source module
//*
#define DEVINFOARRAYSIZE 100
PDEVICE_INFO DevInfoPntrs[DEVINFOARRAYSIZE];
short int gnDevInfoNum = 0;

//*** Function definitions
//*
LRESULT HandleCommands(HWND hWnd, UINT uMsg, WPARAM  wParam, LPARAM lParam)

{
PDIALOG_RESULT pResult;
static char szAbout[] = "MAD_vMonitor.exe.\nPlug-n-Play and Power Mngt. Monitor Utility.\nCopyright (C) 2004 - HTF Consulting";
UINT uRC;

	switch (wParam)
		{
		case IDM_OPEN:
			CleanupPdevList(hWnd); // close all open handles
			EnumExistingDevices(hWnd);
			break;

		case IDM_CLOSE:
			CleanupPdevList(hWnd);
			break;

		case IDM_DISPLAY_CLEAR:
			//uRC = MessageBox(hWnd, "DISPLAY - Clear!", NULL, MB_ICONINFORMATION);
         	SendMessage(ghWndList, LB_RESETCONTENT, 0, 0);
			break;

		case IDM_HELP_ABOUT:
			uRC = MessageBox(hWnd, szAbout, "MAD_Monitor", MB_ICONINFORMATION);
			break;

		case IDM_HIDE:
			pResult = (PDIALOG_RESULT)DialogBox(hInst,
									            MAKEINTRESOURCE(IDD_DIALOG1),
									  	        hWnd, (DLGPROC)DlgProc);  		 
			if (pResult && pResult->SerialNo && IsValid(pResult->SerialNo))
				{
				UINT bytes;
				PDEVICE_INFO deviceInfo = NULL;
				PLIST_ENTRY thisEntry;

    			// Find out the deviceInfo that matches this SerialNo.
				// We need the deviceInfo to get the handle to the device.
				//
				for (thisEntry = gListHead.Flink; thisEntry != &gListHead;
					 thisEntry = thisEntry->Flink)
					{
					deviceInfo = CONTAINING_RECORD(thisEntry, DEVICE_INFO,
					                               ListEntry);
					if (pResult->SerialNo == deviceInfo->ulSerialNo)
						break;

					deviceInfo = NULL;
					} //* end for loop

    			// If found send I/O control
				//
				if (deviceInfo  &&
					!DeviceIoControl(deviceInfo->hDevice,
                 	 			     IOCTL_MADBUS_DONT_DISPLAY_IN_UI_DEVICE,
								     NULL, 0, NULL, 0, &bytes, NULL))
    					MessageBox(hWnd, TEXT("Request Failed or Invalid Serial No"),
	    			               TEXT("Error"), MB_OK);
				free(pResult);
				}
			break;

		case IDM_PLUGIN:
			pResult = (PDIALOG_RESULT)DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG),
									  	       hWnd, (DLGPROC)DlgProc);
			if (pResult)
				{
				if ((!pResult->SerialNo)  ||
					(!OpenBusInterface(pResult->SerialNo, pResult->DeviceId, PLUGIN)))
    					MessageBox(hWnd, TEXT("Request Failed"), TEXT("Error"), MB_OK);  						

				free(pResult);
				}
			break;

		case IDM_UNPLUG:
			pResult = (PDIALOG_RESULT)DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1),
									  	       hWnd, (DLGPROC)DlgProc);

			if (pResult && IsValid(pResult->SerialNo))
				{
				if (!OpenBusInterface(pResult->SerialNo, NULL, UNPLUG))
					MessageBox(hWnd, TEXT("Request Failed"), TEXT("Error"),
					           MB_OK);  									

				free(pResult);
				}
			break;

		case IDM_EJECT:
			pResult = (PDIALOG_RESULT)DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1),
									  	        hWnd, (DLGPROC)DlgProc);
			if (pResult && IsValid(pResult->SerialNo))
				{
				if (!OpenBusInterface(pResult->SerialNo, NULL, EJECT))
					MessageBox(hWnd, TEXT("Request Failed"), TEXT("Error"), MB_OK);  									
				free(pResult);
				}
			break;

		case IDM_EXIT:
			PostQuitMessage(0);
			break;

		default:
			break;
		}

	return TRUE;
}


//*
INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)

{
BOOL success;
PDIALOG_RESULT dialogResult = NULL;

	switch (message)
		{
		case WM_INITDIALOG:
			SetDlgItemText(hDlg, IDC_DEVICEID, (LPCTSTR)MADSIM_HARDWARE_ID1);   	 
			return TRUE;

		case WM_COMMAND:
			switch (wParam)
				{
				case ID_OK:
					dialogResult = malloc(sizeof(DIALOG_RESULT) + MAX_DEVICENAME);
					if (dialogResult == NULL)
						return FALSE;

					dialogResult->DeviceId = (PWCHAR)((PCHAR)dialogResult + sizeof(DIALOG_RESULT));
					dialogResult->SerialNo = GetDlgItemInt(hDlg, IDC_SERIALNO,
											 	           &success, FALSE);
					GetDlgItemText(hDlg, IDC_DEVICEID, (PUCHAR)dialogResult->DeviceId,
					               MAX_DEVICENAME - 1); 				   
					EndDialog(hDlg, (UINT_PTR)dialogResult);
					return TRUE;

				case ID_CANCEL:
					EndDialog(hDlg, 0);
					return TRUE;
				} //* end sw wParam

			break;
		} //* end switch message 

	return FALSE;
}


//* OpenBusInterface *
//*
BOOLEAN OpenBusInterface(IN ULONG SerialNo, IN PWCHAR pDeviceId,
                         IN USER_ACTION_TYPE Action)

{
HANDLE hDevice = INVALID_HANDLE_VALUE;
PSP_INTERFACE_DEVICE_DETAIL_DATA pDeviceInterfaceDetailData = NULL;
ULONG ulPredictedLen = 0;
ULONG ulRequiredLen = 0;
ULONG ulBytes;
DWORD dwLastErrVal;
MADBUS_UNPLUG_HARDWARE unplug;
MADBUS_EJECT_HARDWARE eject;
PMADBUS_PLUGIN_HARDWARE pHardware;
HDEVINFO hHardwareDevInfo;
SP_INTERFACE_DEVICE_DATA DeviceInterfaceData;
BOOLEAN bStatus = FALSE;
BOOLEAN bRC; 

//* Open a handle to the device interface information set of all 
//* present abstract bus enumerator interfaces.
//*
	hHardwareDevInfo = SetupDiGetClassDevs((LPGUID)&GUID_DEVINTERFACE_MADBUS_ONE,
					 	                    NULL, // Define no enumerator (global)
						 	                NULL, // Define no parent window
						 	                (DIGCF_PRESENT | DIGCF_INTERFACEDEVICE)); // Function class devices.
	if (hHardwareDevInfo == INVALID_HANDLE_VALUE)
		{
		dwLastErrVal = GetLastError(); //* ???
		return FALSE;
		}

	DeviceInterfaceData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);
    bRC = (BOOLEAN)SetupDiEnumDeviceInterfaces(hHardwareDevInfo,
                                               0, // No care about specific PDOs
		                                       (LPGUID)&GUID_DEVINTERFACE_MADBUS_ONE,
	 	                                       0, &DeviceInterfaceData);
	if (!bRC)
		{ 
		dwLastErrVal = GetLastError();
		if (dwLastErrVal == ERROR_NO_MORE_ITEMS)
			; //* ??? return FALSE; {
		}

//* Allocate a function class device data structure to receive the
//* information about this particular device.
//*
        bRC = (BOOLEAN)SetupDiGetInterfaceDeviceDetail(hHardwareDevInfo, &DeviceInterfaceData,
                                                       NULL, // probing so no output buffer yet
                                                       0,    // probing so output buffer length of zero
                                                       &ulRequiredLen, NULL); // not interested in the specific dev-node
	dwLastErrVal = GetLastError(); //* Should equal ERROR_INSUFFICIENT_BUFFER 

	ulPredictedLen = ulRequiredLen;
	pDeviceInterfaceDetailData = malloc(ulPredictedLen);
	if (pDeviceInterfaceDetailData == NULL)
		goto Error;

	pDeviceInterfaceDetailData->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
        bRC = (BOOLEAN)SetupDiGetInterfaceDeviceDetail(hHardwareDevInfo, &DeviceInterfaceData,
                                                       pDeviceInterfaceDetailData,
                                                       ulPredictedLen, &ulRequiredLen, NULL);
   	if (!bRC)
		{
		dwLastErrVal = GetLastError();
		goto Error;
		}

//* Use device path returned above to open w. CreateFile
//*
	hDevice = CreateFile(pDeviceInterfaceDetailData->DevicePath,
			  	         (GENERIC_READ | GENERIC_WRITE),
            			 0, // FILE_SHARE_READ | FILE_SHARE_WRITE
			  	         NULL, // no SECURITY_ATTRIBUTES structure
			  	         OPEN_EXISTING, // No special create flags
			  	         0, // No special attributes
			  	         NULL); // No template file
	if (hDevice == INVALID_HANDLE_VALUE)
		goto Error;

//* Enumerate Devices
//*
	if (Action == PLUGIN)
		{
		ulBytes = (ULONG)(sizeof(MADBUS_PLUGIN_HARDWARE)  + (wcslen(pDeviceId) + 2) * sizeof(WCHAR));

		pHardware = malloc(ulBytes); 
		if (pHardware == NULL)
			goto Error;

		memset(pHardware, 0, ulBytes);
		pHardware->Size = sizeof(MADBUS_PLUGIN_HARDWARE);
		pHardware->SerialNo = SerialNo;
 	    wcscpy(pHardware->HardwareIDs, pDeviceId); //* Copy the Device ID
        bRC = (BOOLEAN)DeviceIoControl(hDevice, IOCTL_MADBUS_PLUGIN_HARDWARE,
                                       pHardware, ulBytes,
                                       pHardware, ulBytes, &ulBytes, NULL);
		free(pHardware);
		if (!bRC)
			goto Error;
		}

//* Removes a device if given the specific Id of the device. Otherwise this
//* ioctls removes all the devices that are enumerated so far.
//*
	if (Action == UNPLUG)
		{
		unplug.Size = ulBytes = sizeof(unplug);
		unplug.SerialNo = SerialNo;

                bRC = (BOOLEAN)DeviceIoControl(hDevice, IOCTL_MADBUS_UNPLUG_HARDWARE,
                                               &unplug, ulBytes, &unplug, ulBytes, &ulBytes, NULL);
		if (!bRC)
			goto Error;
		}

//* Ejects a device if given the specific Id of the device. Otherwise this
//* ioctls ejects all the devices that are enumerated so far.
//*
	if (Action == EJECT)
		{
		eject.Size = ulBytes = sizeof(eject);
		eject.SerialNo = SerialNo;
                bRC = (BOOLEAN)DeviceIoControl(hDevice, IOCTL_MADBUS_EJECT_HARDWARE,
                                               &eject, ulBytes, &eject, ulBytes, &ulBytes, NULL);
		if (!bRC)
			goto Error;
		}

	bStatus = TRUE;

Error:;
  	if (hDevice != INVALID_HANDLE_VALUE)
        CloseHandle(hDevice);

	if (pDeviceInterfaceDetailData == NULL)
            free(pDeviceInterfaceDetailData);

    SetupDiDestroyDeviceInfoList(hHardwareDevInfo);

    return bStatus;
}


//* HandleInterfaceChange
//*
BOOL HandleDeviceInterfaceChange(HWND hWnd, DWORD dwEvtType,
								 PDEV_BROADCAST_DEVICEINTERFACE pDIP)

{
DEV_BROADCAST_HANDLE hDevBrdcast;
PDEVICE_INFO pDevInfo = NULL;
BOOLEAN bRC; 
//UINT uRC;
HANDLE hDevNotify;
PCHAR pChar;
char szInfoMsg[200]; 

	pDevInfo = malloc(sizeof(DEVICE_INFO));
	if (!pDevInfo)
		return FALSE;

	memset(pDevInfo, 0, sizeof(DEVICE_INFO));
        bRC = (BOOLEAN)GetDeviceDescription(pDIP->dbcc_name, pDevInfo->DeviceName,
                                            &pDevInfo->ulSerialNo);
	if (!bRC)
		{
		MessageBox(hWnd,
			       "HandleDeviceInterfaceChange - GetDeviceDescription failed!",
			       "Error!", MB_ICONEXCLAMATION);
        return FALSE;
		}

	switch (dwEvtType)
		{
		case DBT_DEVICEARRIVAL:
			// New device arrived. Open handle to the device 
			// and register notification of type DBT_DEVTYP_HANDLE
			//
			DevInfoPntrs[gnDevInfoNum] = pDevInfo;
			gnDevInfoNum++;

			//LB_Insert((PUSHORT)TEXT("New device arrived: %ws"), (PUSHORT)pDevName);
            Log_Event("Interface change-New device: ",
				      pDevInfo->DeviceName, pDevInfo->ulSerialNo);
			gbPlugged[pDevInfo->ulSerialNo] = TRUE;

			pChar = strcpy(pDevInfo->DevicePath, pDIP->dbcc_name);
			sprintf(szInfoMsg,
				   "Strlen = %ld, Device path = \n%s", (long)strlen(pChar),
				   pDevInfo->DevicePath);
			//uRC = MessageBox(NULL, szInfoMsg, NULL, MB_ICONEXCLAMATION);
			pDevInfo->hDevice = CreateFile(pDevInfo->DevicePath, GENERIC_READ,
					  	                   0, NULL, OPEN_EXISTING, 0, NULL);
			if (pDevInfo->hDevice == INVALID_HANDLE_VALUE)
				break; 

			pDevInfo->bActive = TRUE;
			Init_DevBrdcast_Hndl(&hDevBrdcast,
				                 DBT_DEVTYP_HANDLE, pDevInfo->hDevice);  
			hDevNotify = RegisterDeviceNotification(hWnd, &hDevBrdcast, 0);
            pDevInfo->hHandleNotification = hDevNotify;
			break;

		case DBT_DEVICEREMOVECOMPLETE:
			//LB_Insert(TEXT((PUSHORT)"Device removal complete"), NULL);
			Log_Event("Interface change-device removed: ",
				      pDevInfo->DeviceName, pDevInfo->ulSerialNo);
			gbPlugged[pDevInfo->ulSerialNo] = FALSE;

			free(pDevInfo); //* Temporary use
			break;    
     		
		default:
			//LB_Insert(TEXT((PUSHORT)"Unknown event"), NULL);
			sprintf(szInfoMsg,
				    "Interface change-unknown event (%04x): ", dwEvtType);
			Log_Event(szInfoMsg, pDevInfo->DeviceName, pDevInfo->ulSerialNo);

			free(pDevInfo); //* Temporary use
			break;
		} //* end switch

	return TRUE;
}


//*** HandleDeviceChange
//*
BOOL HandleDeviceChange(HWND hWnd, DWORD evtype, PDEV_BROADCAST_HANDLE dhp)

{
register short int i;
DEV_BROADCAST_HANDLE hDevBrdcast;
PDEVICE_INFO pDevInfo = NULL;
//PLIST_ENTRY thisEntry;
char szInfoMsg[100] = "";

//* Sequentially search the array of pntrs to get DeviceInfo for this device
//* by matching the handle given in the notification.
//*
	for (i = 0; i < gnDevInfoNum; i++)
		{
		pDevInfo = DevInfoPntrs[i];
		if (pDevInfo != NULL)
		    if (pDevInfo->bActive)
				if (pDevInfo->hDevice == dhp->dbch_handle) //* Found it
			        break;
	
		pDevInfo = NULL;
		} 

	if (pDevInfo == NULL)
		{
		sprintf(szInfoMsg, "Device change-unknown device: event = %d", evtype);
		MessageBox(hWnd, szInfoMsg, "Error", MB_ICONEXCLAMATION); 
	    SendMessage(ghWndList, LB_INSERTSTRING, -1, (LPARAM)szInfoMsg);

		return FALSE;
		}

	switch (evtype)
		{
		case DBT_DEVICEQUERYREMOVE:
			//LB_Insert(TEXT((PUSHORT)"Query Remove (Handle Notification)"),
			//          (PUSHORT)pDevInfo->DeviceName);
			Log_Event("Device change-query removal: ",
				      pDevInfo->DeviceName, pDevInfo->ulSerialNo);

			// Close the handle so that target device can be removed.
			// Do not unregister the notification at this point,
			// because we still need to know whether it's successfully removed
			//
			if (pDevInfo->hDevice != INVALID_HANDLE_VALUE)
				{
				CloseHandle(pDevInfo->hDevice);
				//LB_Insert(TEXT((PUSHORT)"Closed handle to device %ws"),
				//	      (PUSHORT)pDevInfo->DeviceName);
                Log_Event("Closed handle to device: ",
				          pDevInfo->DeviceName, pDevInfo->ulSerialNo);

				// since we use the handle to locate the deviceinfo, we
				// will defer the following to the remove_pending.
				//deviceInfo->hDevice = INVALID_HANDLE_VALUE;
				}
			break;

		case DBT_DEVICEREMOVECOMPLETE:
			//LB_Insert(TEXT((PUSHORT)"Remove Complete: %ws"),
			//	      (PUSHORT)deviceInfo->DeviceName);
			Log_Event("Device change-remove complete: ",
			          pDevInfo->DeviceName, pDevInfo->ulSerialNo);
			gbPlugged[pDevInfo->ulSerialNo] = FALSE;

			// Device is removed so close the handle if it's there
			// and unregister the notification
			//
			if (pDevInfo->hHandleNotification)
				{
				UnregisterDeviceNotification(pDevInfo->hHandleNotification);
				pDevInfo->hHandleNotification = NULL;
				}

			if (pDevInfo->hDevice != INVALID_HANDLE_VALUE)
				{
				CloseHandle(pDevInfo->hDevice);
				pDevInfo->hDevice = INVALID_HANDLE_VALUE;

				//LB_Insert(TEXT((PUSHORT)"Closed handle to device %ws"),
				//	      (PUSHORT)pDevInfo->DeviceName);
			    Log_Event("Closed handle to device: ",
			              pDevInfo->DeviceName, pDevInfo->ulSerialNo);
				}
			
			// Unlink this deviceInfo from the list and free the memory
			//
			//RemoveEntryList(&pDevInfo->ListEntry);
			//free(pDevInfo);
			pDevInfo->bActive = FALSE;
			break;

		case DBT_DEVICEREMOVEPENDING:
			//LB_Insert(TEXT((PUSHORT)"Remove Pending: %ws"),
     		//		  (PUSHORT)pDevInfo->DeviceName);
			Log_Event("Device change-remove pending: ",
			          pDevInfo->DeviceName, pDevInfo->ulSerialNo);

			// Device is removed so close the handle if it's there
			// and unregister the notification
			//
			if (pDevInfo->hHandleNotification)
				{
				UnregisterDeviceNotification(pDevInfo->hHandleNotification);
				pDevInfo->hHandleNotification = NULL;
				pDevInfo->hDevice = INVALID_HANDLE_VALUE;
				}

			// Unlink this deviceInfo from the list and free the memory
			//
			//RemoveEntryList(&pDevInfo->ListEntry);
			//free(pDevInfo);
			pDevInfo->bActive = FALSE;
			break;

		case DBT_DEVICEQUERYREMOVEFAILED :
			//LB_Insert(TEXT((PUSHORT)"Remove failed: %ws"),
     		//		  (PUSHORT)pDevInfo->DeviceName);
			Log_Event("Device change-remove pending: ",
			          pDevInfo->DeviceName, pDevInfo->ulSerialNo);

			// Remove failed. So reopen the device and register for
			// notification on the new handle. But first we should unregister
			// the previous notification.
			//
			if (pDevInfo->hHandleNotification)
				{
				UnregisterDeviceNotification(pDevInfo->hHandleNotification);
				pDevInfo->hHandleNotification = NULL;
				}

			pDevInfo->hDevice = CreateFile(pDevInfo->DevicePath, GENERIC_READ,
				                           0, NULL, OPEN_EXISTING, 0, NULL);
			if (pDevInfo->hDevice == INVALID_HANDLE_VALUE)
				{
				//free(pDevInfo);
                pDevInfo->bActive = FALSE;
				break;
				}

			// Register handle based notification to receive pnp 
			// device change notification on the handle.
			//
			Init_DevBrdcast_Hndl(&hDevBrdcast,
				                 DBT_DEVTYP_HANDLE, pDevInfo->hDevice);  
			pDevInfo->hHandleNotification =
				RegisterDeviceNotification(hWnd, &hDevBrdcast, 0);
			//LB_Insert(TEXT((PUSHORT)"Reopened device %ws"), 
			//	      (PUSHORT)pDevInfo->DeviceName);  
			Log_Event("Device change-reopened device: ",
			          pDevInfo->DeviceName, pDevInfo->ulSerialNo);
			break;

		default:
			//LB_Insert(TEXT((PUSHORT)"Unrecognized device notification: %ws"), 
			//          (PUSHORT)pDevInfo->DeviceName);
			sprintf(szInfoMsg,
				    "Device change-unrecognized event notification (%04x): ", 
				    evtype);
		    //SendMessage(ghWndList, LB_INSERTSTRING, -1, (LPARAM)gOutText);
		    Log_Event(szInfoMsg, pDevInfo->DeviceName, pDevInfo->ulSerialNo);
	
			break;
		} //* end switch

	return TRUE;
}


//*** EnumExistingDevices
//*
BOOLEAN EnumExistingDevices(HWND hWnd)

{
//register UINT i = 0;
UINT uRC;
BOOLEAN bRC;
HDEVINFO hHardwareDevInfo;
HANDLE hNotify;
SP_INTERFACE_DEVICE_DATA DeviceInterfaceData;
PSP_INTERFACE_DEVICE_DETAIL_DATA pDevInterfaceDetail = NULL;
ULONG ulPredictedLen = 0;
ULONG ulRequiredLen = 0, ulBytes = 0;
DWORD dwGLE = 0;
DEV_BROADCAST_HANDLE hDevBrdcast;
PDEVICE_INFO pDevInfo = NULL;
char szInfoMsg[100] = "";

	hHardwareDevInfo = SetupDiGetClassDevs((LPGUID)&gInterfaceGuid,
					 	                   NULL, // Define no enumerator (global)
                      					   NULL, // Define no
										   // Devices present & Function class devices
						 	               (DIGCF_PRESENT | DIGCF_INTERFACEDEVICE));
	if (hHardwareDevInfo == INVALID_HANDLE_VALUE)
		goto Error;

//* Enumerate devices of Model-Abstract-Demo device class
//*
	DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);
    bRC = (BOOLEAN)SetupDiEnumDeviceInterfaces(hHardwareDevInfo, NULL, // Not concerned with specific PDOs
                                               (LPGUID)&gInterfaceGuid, 0, &DeviceInterfaceData);
    while (bRC)
		{
		// Allocate a function class device data structure to 
		// receive the information about this particular device.
		//
     	// First find out required length of the buffer
		//
		if (pDevInterfaceDetail != NULL)
			free(pDevInterfaceDetail); //* Release previous

                bRC = (BOOLEAN)SetupDiGetInterfaceDeviceDetail(hHardwareDevInfo,
                                                               &DeviceInterfaceData,
                                                               NULL, // probing so no output buffer yet
                                                               0,    // probing so output buffer length of zero
                                                               &ulRequiredLen, NULL);
		if (!bRC)
			{
			dwGLE = GetLastError();
			if (dwGLE != ERROR_INSUFFICIENT_BUFFER)
			    break;

			dwGLE = 0;
			}
			
		ulPredictedLen = ulRequiredLen;
		pDevInterfaceDetail = malloc(ulPredictedLen);
		if (pDevInterfaceDetail == NULL)
			{
			dwGLE = ERROR_OUTOFMEMORY; 
		    break;
			}

		pDevInterfaceDetail->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
                bRC = (BOOLEAN)SetupDiGetInterfaceDeviceDetail(hHardwareDevInfo, &DeviceInterfaceData,
                                                               pDevInterfaceDetail, ulPredictedLen, &ulRequiredLen, NULL);
		if (!bRC)
			{
			dwGLE = GetLastError();
			break;
			}

		pDevInfo = malloc(sizeof(DEVICE_INFO));
	    if (pDevInfo == NULL)
			{
			dwGLE = ERROR_OUTOFMEMORY; 
		    break;
			}

		// Get the device details such as friendly name and SerialNo
		//
	    memset(pDevInfo, 0, sizeof(DEVICE_INFO));
                bRC = (BOOLEAN)GetDeviceDescription(pDevInterfaceDetail->DevicePath,
                                                    pDevInfo->DeviceName, &(pDevInfo->ulSerialNo));
		if (!bRC)
			break;

		//Log_Event("Device found at startup - Path: ",
		//	      pDevInterfaceDetail->DevicePath, pDevInfo->ulSerialNo);
		Log_Event("Device found at startup: ",
			      pDevInfo->DeviceName, pDevInfo->ulSerialNo);
		gbPlugged[pDevInfo->ulSerialNo] = TRUE;

		DevInfoPntrs[gnDevInfoNum] = pDevInfo;
		gnDevInfoNum++;

		// Open an handle to the device.
		//
		strcpy(pDevInfo->DevicePath, pDevInterfaceDetail->DevicePath);
		pDevInfo->hDevice = CreateFile(pDevInfo->DevicePath, (GENERIC_READ | GENERIC_WRITE),
							  	       0, NULL, OPEN_EXISTING, 0, NULL);
		if (pDevInfo->hDevice == INVALID_HANDLE_VALUE)
			{
			dwGLE = GetLastError();
			break;
			}

		// Register handle based notification to receive pnp 
		// device change notification on the handle.
		//
		hDevBrdcast.dbch_handle = pDevInfo->hDevice;
		Init_DevBrdcast_Hndl(&hDevBrdcast, DBT_DEVTYP_HANDLE,pDevInfo->hDevice);  
		hNotify = RegisterDeviceNotification(hWnd, &hDevBrdcast, 0);
		pDevInfo->hHandleNotification = hNotify;
		pDevInfo->bActive = TRUE;

        bRC = (BOOLEAN)SetupDiEnumDeviceInterfaces(hHardwareDevInfo, NULL, //No specific PDOs
	                                      (LPGUID)&gInterfaceGuid, gnDevInfoNum,
										  &DeviceInterfaceData);
		} //* end while  

	//CleanupPdevList(hWnd);
	if (pDevInterfaceDetail != NULL)
		free(pDevInterfaceDetail);

	if (hHardwareDevInfo != INVALID_HANDLE_VALUE)
	    SetupDiDestroyDeviceInfoList(hHardwareDevInfo);

	sprintf(szInfoMsg, "EnumExistingDevices found %d device(s)", gnDevInfoNum);
	//bRC = MessageBox(hWnd, szInfoMsg, "AbsDevMon.exe", MB_ICONINFORMATION);  

	if (gnDevInfoNum > 0) //* We must have found one active  
		return TRUE;
		
Error:;
	//dwGLE = GetLastError();
    if (dwGLE != 0) //* From above - but it ain't heaven sent
		{
                sprintf(szInfoMsg,
		        "EnumExistingDevices failed.\nGetLastError returned %d", dwGLE);
                uRC = MessageBox(hWnd, szInfoMsg, "Error!", MB_ICONEXCLAMATION);  
		}
	
	return FALSE;
}



//*****
//*
BOOL HandlePowerBroadcast(HWND hWnd, WPARAM wParam, LPARAM lParam)

{
BOOL bRet = TRUE;

    switch (wParam)
	    {
	    case PBT_APMQUERYSTANDBY:
	        LB_Insert(TEXT((PUSHORT)L"PBT_APMQUERYSTANDBY"), NULL);
		    break;

		case PBT_APMQUERYSUSPEND:
			gbSysDownActv = TRUE;
			LB_Insert(TEXT((PUSHORT)L"PBT_APMQUERYSUSPEND"), NULL);
			break;

		case PBT_APMSTANDBY:
			LB_Insert(TEXT((PUSHORT)L"PBT_APMSTANDBY"), NULL);
			break;

		case PBT_APMSUSPEND:
			gbSysDownActv = TRUE;
			LB_Insert(TEXT((PUSHORT)L"PBT_APMSUSPEND"), NULL);
			break;
		case PBT_APMQUERYSTANDBYFAILED:
			LB_Insert(TEXT((PUSHORT)L"PBT_APMQUERYSTANDBYFAILED"), NULL);
			break;

		case PBT_APMRESUMESTANDBY:
			LB_Insert(TEXT((PUSHORT)L"PBT_APMRESUMESTANDBY"), NULL);
			break;

		case PBT_APMQUERYSUSPENDFAILED:
			LB_Insert(TEXT((PUSHORT)L"PBT_APMQUERYSUSPENDFAILED"), NULL);
			break;

		case PBT_APMRESUMESUSPEND:
			gbSysDownActv = FALSE;
			LB_Insert(TEXT((PUSHORT)L"PBT_APMRESUMESUSPEND"), NULL);
			break;

		case PBT_APMBATTERYLOW:
			LB_Insert(TEXT((PUSHORT)L"PBT_APMBATTERYLOW"), NULL);
			break;

		case PBT_APMOEMEVENT:
			LB_Insert(TEXT((PUSHORT)L"PBT_APMOEMEVENT"), NULL);
			break;

		case PBT_APMRESUMEAUTOMATIC:
			gbSysDownActv = FALSE;
			LB_Insert(TEXT((PUSHORT)L"PBT_APMRESUMEAUTOMATIC"), NULL);
			break;

		case PBT_APMRESUMECRITICAL:
			gbSysDownActv = FALSE;
			LB_Insert(TEXT((PUSHORT)L"PBT_APMRESUMECRITICAL"), NULL);
			break;

		case PBT_APMPOWERSTATUSCHANGE:
			//gbSysDownActv = FALSE;
			LB_Insert(TEXT((PUSHORT)L"PBT_APMPOWERSTATUSCHANGE"), NULL);
			break;

		default:
			LB_Insert(TEXT((PUSHORT)L"Default"), NULL);
			break;
	} //* end sw

	return bRet;
}


//***
//*
BOOLEAN CleanupPdevList(HWND hWnd)

{
register int i;
PDEVICE_INFO pDevInfo = NULL;
//PLIST_ENTRY thisEntry;

	for (i = 0; i < gnDevInfoNum; i++)
		{
		if (pDevInfo == NULL)
			continue;

		if (pDevInfo->hHandleNotification)
			{
			UnregisterDeviceNotification(pDevInfo->hHandleNotification);
			pDevInfo->hHandleNotification = NULL;
			}

		if (pDevInfo->hDevice != INVALID_HANDLE_VALUE)
			{
			CloseHandle(pDevInfo->hDevice);
			pDevInfo->hDevice = INVALID_HANDLE_VALUE;
			//LB_Insert(TEXT((PUSHORT)"Closed handle to device: %ws"),
			//	      (PUSHORT)pDevInfo->DeviceName);
			Log_Event("Cleanup - Closed handle to device: ",
				      pDevInfo->DeviceName, pDevInfo->ulSerialNo);
			}

		free(pDevInfo);
		} //* end for loop

	return TRUE;
}


//*******
//*
_inline BOOLEAN IsValid(ULONG nNum)

{
register int i;
//PLIST_ENTRY thisEntry;
PDEVICE_INFO pDevInfo;

    if (nNum == 0)
	    return TRUE; //special case

	for (i = 0; i < gnDevInfoNum; i++)
		{
	    pDevInfo = DevInfoPntrs[i];
		if (pDevInfo != NULL)
			if (pDevInfo->bActive)
	            if (pDevInfo->ulSerialNo == nNum)
		            return TRUE;
		} 	

    return FALSE;
}


//* Analyze power activity for each unit display info to the client area
//* 
void Analyze_Power_Action(POWERNOTIFY_TYPE *pPrevPowerAxn,
                          POWERNOTIFY_TYPE *pCurPowerAxn) 

{
static char szPowerAxnMesg[] = "Power Activity - Unit ";
static char szPowerAxns[7][10] = {"None", "Sleep", "Hibernate",
								  "Shutdown", "Reset", "Off", "Unknown"};
static char szSysStates[8][12] = {"Working", "Sleep1", "Sleep2", "Sleep3",
								  "Hibernate", "Shutdown", "Maximum",
								  "Unknown"};
static char szDevStates[5][12] = {"D0", "D1", "D2", "D3", "Unknown"};

register int j;
char szInfoMsg[100];
char szAnalysis[100];
short int nPowerNotice;
short int nPowerAxn, nDevState, nSysState;

    for (j = 1; j <= 9; j++) //* Skip array[0]
		{
		if (gbPlugged[j] == FALSE)
			continue; //* Unit not plugged in - skip this unit

                nPowerNotice = (SHORT)pCurPowerAxn->PowerNotices[j];
		if (nPowerNotice == pPrevPowerAxn->PowerNotices[j])
			continue; //* No change - skip this unit

//* We have a unit-specific update - let's analyse & display
//*
        sprintf(szInfoMsg, "%s %d %d: ", szPowerAxnMesg, j, nPowerNotice);

        nPowerAxn = (nPowerNotice >> 8);
		nPowerAxn = min(nPowerAxn, 6);

        nSysState = (nPowerNotice & 0x00FF) >> 4;
		nSysState = min(nSysState, 7);

        nDevState = (nPowerNotice & 0x000F);
		nDevState = min(nDevState, 4);
		
		sprintf(szAnalysis, "Sys-Action = %s, Sys-State = %s, Dev-State = %s",
				szPowerAxns[nPowerAxn], szSysStates[nSysState],
				szDevStates[nDevState]);
		strcat(szInfoMsg, szAnalysis);
        LB_Insert(TEXT((PUSHORT)szInfoMsg), NULL);
		} //* next j

	return;
}


//**
_inline void LB_Insert(PWCHAR pFormatStr, PWCHAR pArg)

{
int rc;

	if (pArg != NULL)
		rc = wsprintf(gOutText, (PUCHAR)pFormatStr, pArg);
	else
		rc = wsprintf(gOutText, (PUCHAR)pFormatStr);
	
	//SendMessage(ghWndList, LB_ADDSTRING, 0, (LPARAM)gOutText);
	SendMessage(ghWndList, LB_INSERTSTRING, -1, (LPARAM)gOutText);
	gListBoxIndex++;

	return;
}


//*** Log_Event
//*
void Log_Event(char* szEventText, char* szDevName, ULONG ulSerialNo)

{
int rc;

	rc = sprintf(gOutText, "%s %s%1x", szEventText, szDevName, ulSerialNo);
//	rc = sprintf(gOutText, "%s %s%", szEventText, szDevName);
	
	//SendMessage(ghWndList, LB_ADDSTRING, 0, (LPARAM)gOutText);
	SendMessage(ghWndList, LB_INSERTSTRING, -1, (LPARAM)gOutText);

	return;
}


//*** Init_DevBrdcast_Hndl
//*
void Init_DevBrdcast_Hndl(PDEV_BROADCAST_HANDLE pDevBrdcastHndl,
						  USHORT uDevType, HANDLE hDevice)  

{
	memset(pDevBrdcastHndl, 0x00, sizeof(DEV_BROADCAST_HANDLE)); 
	pDevBrdcastHndl->dbch_size       = sizeof(DEV_BROADCAST_HANDLE);
	pDevBrdcastHndl->dbch_devicetype = uDevType; //DBT_DEVTYP_HANDLE;
	pDevBrdcastHndl->dbch_handle     = hDevice;

    return;
}


//* Check to see if a file has an update
//*
BOOLEAN Chek_File_Update(char szFilename[], POWERNOTIFY_TYPE *pPowerAxn, short int nLen,
						 time_t *ptLastChek)

{
int hFile;
int wrc;
double dElapsedTime;
time_t CurrTime;
struct _stat StatBufr;
//char szBuffer[50]; 

    time(&CurrTime);

    hFile = _open(szFilename, _O_RDONLY);
    if (hFile < 1)
        {
        *ptLastChek = CurrTime;   

        return FALSE;
        }

    wrc = _read(hFile, (PVOID)pPowerAxn, nLen+1);
    if (wrc < nLen)
        {
        _close(hFile);
        *ptLastChek = CurrTime;   

        return FALSE;
        }

    wrc = _fstat(hFile, &StatBufr); 
    _close(hFile);
    if (wrc != 0)
        {
        *ptLastChek = CurrTime;   

        return FALSE;
        }

    dElapsedTime = difftime(StatBufr.st_mtime, *ptLastChek); 
    *ptLastChek = CurrTime;   

    if (dElapsedTime < 0.01f)
        return FALSE;

    return TRUE; //* Indicates an updated file	
}  

