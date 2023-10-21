//#include "..\MadPnpPmUtils.c"
/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : Model-Abstract-Demo Device Simulation Subsystem             */
/*  COPYRIGHT    : (c) 2014 HTF CONSULTING                                     */
/*                                                                             */
/*******************************************************************************/
/*                                                                             */
/*  Exe file ID  : MadEnum.exe, MadMonitor.exe                                 */
/*                                                                             */
/*  Module  NAME : MadPnpPmUtils.c                                             */
/*  DESCRIPTION  : Utility function definitions for the above programs         */
/*                                                                             */
/*******************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setupapi.h>
#include <dbt.h>
#include <winioctl.h>
#include "..\Includes\MadGUIDs.h"
#include "..\Includes\MadMonitor.h"
#define DEVINFO
#include "..\Includes\MadPnpPmUtils.h"


//*** EnumExistingDevices
//*
BOOLEAN GetPnpDevice(LPGUID pInterfaceGuid, ULONG ulSerNum, PDEVICE_INFO pDevInfo)

{
	//register UINT i = 0;
	BOOLEAN bRC = FALSE;
	HDEVINFO hHardwareDevInfo;
	//HANDLE hNotify;
	SP_INTERFACE_DEVICE_DATA DeviceInterfaceData;
	PSP_INTERFACE_DEVICE_DETAIL_DATA pDevInterfaceDetail = NULL;
	ULONG ulSerNo = 0;
	ULONG ulPredictedLen = 0;
	ULONG ulRequiredLen = 0, ulBytes = 0;
	UINT nDevInfoNum = 0, uRC;
	DWORD dwGLE = 0;
	//DEV_BROADCAST_HANDLE hDevBrdcast;
	char szInfoMsg[150] = "";

	hHardwareDevInfo = SetupDiGetClassDevs((LPGUID)pInterfaceGuid, NULL, NULL, // Define no
		(DIGCF_PRESENT | DIGCF_INTERFACEDEVICE));
	if (hHardwareDevInfo == INVALID_HANDLE_VALUE)
		return FALSE;

	//* Enumerate devices of our device class
	//*
	DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);
	bRC = (BOOLEAN)SetupDiEnumDeviceInterfaces(hHardwareDevInfo, NULL, // Not concerned with specific PDOs
		pInterfaceGuid, 0, &DeviceInterfaceData);
	if (!bRC)
	{
		dwGLE = GetLastError();
		sprintf_s(szInfoMsg, 150,
			"SetupDiEnumDeviceInterfaces failed.\nGetLastError returned %d", dwGLE);
		uRC = MessageBox(NULL, szInfoMsg, "Error!", MB_ICONEXCLAMATION);
	}

	while ((bRC) && (pDevInfo->ulSerialNo != ulSerNum))
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
			//dwGLE = GetLastError();
			//if (dwGLE == ERROR_INSUFFICIENT_BUFFER)
			//    dwGLE = 0;
			//else 
			break;
		}

		ulPredictedLen = ulRequiredLen;
		pDevInterfaceDetail = (PSP_INTERFACE_DEVICE_DETAIL_DATA)malloc(ulPredictedLen);
		if (pDevInterfaceDetail == NULL)
			break;

		pDevInterfaceDetail->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
		bRC = (BOOLEAN)SetupDiGetInterfaceDeviceDetail(hHardwareDevInfo,
			&DeviceInterfaceData,
			pDevInterfaceDetail,
			ulPredictedLen, &ulRequiredLen, NULL);
		if (!bRC)
			break;

		nDevInfoNum++;

		// Get the device details such as friendly name and SerialNo
		//
		memset(pDevInfo, 0, sizeof(DEVICE_INFO));
		bRC = (BOOLEAN)GetDeviceDescription(pDevInterfaceDetail->DevicePath,
			pDevInfo->DeviceName, &pDevInfo->ulSerialNo);
		if (!bRC)
			break;

		if (pDevInfo->ulSerialNo != ulSerNum) //* Still not found - continue
			bRC = (BOOLEAN)SetupDiEnumDeviceInterfaces(hHardwareDevInfo, NULL, //No specific PDOs
				(LPGUID)pInterfaceGuid, nDevInfoNum,
				&DeviceInterfaceData);
	} //* end while  

	if (bRC)
	{
#pragma warning(suppress: 6011)
		strcpy_s(pDevInfo->DevicePath, 150, pDevInterfaceDetail->DevicePath);
		//*pulSerNum = pDevInfo->ulSerialNo;
	}

	if (pDevInterfaceDetail != NULL)
		free(pDevInterfaceDetail);

	if (hHardwareDevInfo != INVALID_HANDLE_VALUE)
		SetupDiDestroyDeviceInfoList(hHardwareDevInfo);

	return bRC;
}


