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
/*  Exe file ID  : MadSimUI.exe                                                */
/*                                                                             */
/*  Module  NAME : MadDevSimOpen.c                                             */
/*                                                                             */
/*  DESCRIPTION  : Function definition(s)                                      */
/*                                                                             */
/*******************************************************************************/

#include <basetyps.h>
#include <wtypes.h>
#include <setupapi.h>
#include <initguid.h>
#include <winioctl.h>
#include "..\Includes\MadGUIDs.h"
#include "string.h"
#include "stdio.h"

HDEVINFO MadDevSimOpen(void)

{
HDEVINFO hHdwrDevInfo;
HANDLE hDevice = INVALID_HANDLE_VALUE;
SP_INTERFACE_DEVICE_DATA         DeviceInterfaceData;
PSP_INTERFACE_DEVICE_DETAIL_DATA pDeviceInterfaceDetailData = NULL;
ULONG LenXpectd = 0;
ULONG LenRequired = 0;
ULONG Len;
//ULONG bytes;
BOOL bRC;
char szObjectName[250];
char szInfoMsg[350];

    hHdwrDevInfo = SetupDiGetClassDevs((LPGUID)&GUID_DEVINTERFACE_MADBUS_ONE,
			 	                        NULL, NULL, (DIGCF_PRESENT|DIGCF_INTERFACEDEVICE)); 
    if (hHdwrDevInfo == INVALID_HANDLE_VALUE)
	    {
		sprintf_s(szInfoMsg, 350, "SetupDiGetClassDevs returned INVALID_HANDLE_VALUE");
	    bRC = MessageBox(NULL, szInfoMsg, NULL, MB_ICONINFORMATION);
        return INVALID_HANDLE_VALUE;
	    }
	
	sprintf_s(szInfoMsg, 350, "SetupDiGetClassDevs returned HDEVINFO=%p", hHdwrDevInfo);
	//bRC = MessageBox(NULL, szInfoMsg, NULL, MB_ICONINFORMATION);

    DeviceInterfaceData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);
    bRC = SetupDiEnumDeviceInterfaces(hHdwrDevInfo, 0, 
		                             (LPGUID)&GUID_DEVINTERFACE_MADBUS_ONE,
		                              0, &DeviceInterfaceData);
    if (!bRC)
	    {
		sprintf_s(szInfoMsg, 350, "SetupDiEnumDeviceInterfaces failed!");
	    bRC = MessageBox(NULL, szInfoMsg, NULL, MB_ICONINFORMATION);
        return INVALID_HANDLE_VALUE;
	    }
	else
	    {
	    sprintf_s(szInfoMsg, 350, "SetupDiEnumDeviceInterfaces passed");
	    //bRC = MessageBox(NULL, szInfoMsg, NULL, MB_ICONINFORMATION);

	    bRC = SetupDiGetInterfaceDeviceDetail(hHdwrDevInfo, &DeviceInterfaceData,
	                                          NULL, 0, &LenRequired, NULL); 
		if (bRC)
	        sprintf_s(szInfoMsg, 350, "SetupDiGetInterfaceDeviceDetail returned reqlen=%d", LenRequired);
	    //bRC = MessageBox(NULL, szInfoMsg, NULL, MB_ICONINFORMATION);

#pragma warning(suppress: 6102)
        LenXpectd = LenRequired;
        pDeviceInterfaceDetailData = (PSP_INTERFACE_DEVICE_DETAIL_DATA)malloc(LenXpectd);
		if (pDeviceInterfaceDetailData == NULL)
			return INVALID_HANDLE_VALUE;

	    pDeviceInterfaceDetailData->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
        bRC = SetupDiGetInterfaceDeviceDetail(hHdwrDevInfo, &DeviceInterfaceData,
	     	                                  pDeviceInterfaceDetailData,
		     	                              LenXpectd, &LenRequired, NULL);

#pragma warning(suppress: 6102)
		Len = (ULONG)strlen(pDeviceInterfaceDetailData->DevicePath);
	    sprintf_s(szInfoMsg, 350, "strlen(DevicePath) = %d", Len);
	    //bRC = MessageBox(NULL, szInfoMsg, NULL, MB_ICONINFORMATION);

		memset(szObjectName, 0x00, 200);
	    strcpy_s(szObjectName, 250, pDeviceInterfaceDetailData->DevicePath); 
	    sprintf_s(szInfoMsg, 350, "Object Name = %s", szObjectName);
	    //bRC = MessageBox(NULL, szInfoMsg, NULL, MB_ICONINFORMATION);
        hDevice = CreateFile(szObjectName,
	   	                     (GENERIC_READ|GENERIC_WRITE),
							 (FILE_SHARE_READ|FILE_SHARE_WRITE),
							 NULL, OPEN_EXISTING, 0, NULL); 
	    sprintf_s(szInfoMsg, 300,
			      "CreateFile device handle: %lld", (__int64)hDevice);
	    //bRC = MessageBox(NULL, szInfoMsg, NULL, MB_ICONINFORMATION);
	    }

    SetupDiDestroyDeviceInfoList(hHdwrDevInfo);

    return hDevice;
}