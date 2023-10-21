/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : Model-Abstract-Demo Device Simulation Subsystem             */
/*  COPYRIGHT    : (c) 2014 HTF CONSULTING                                     */
/*                                                                             */
/*******************************************************************************/
/*                                                                             */
/*  Exe file ID  : MadEnum.exe                                                 */
/*                                                                             */
/*  Module  NAME : MadEnum.c                                                   */
/*  DESCRIPTION  : This application interacts w. the MAdBus driver to          */
/*                 simulate the plugin, unplug & eject, & Power Mngt.          */                                  
/*                                                                             */
/*                                                                             */
/*******************************************************************************/

#include <basetyps.h>
#include <stdlib.h>
#include <wtypes.h>
#include <setupapi.h>
#include <initguid.h>
#include <stdio.h>
#include <string.h>
#include <winioctl.h>
#define PHYSICAL_ADDRESS LARGE_INTEGER
#include "..\Includes\MadBusIoctls.h"
#include "..\Includes\MadGUIDs.h"

#pragma warning(suppress: 6102) 
//#pragma warning(suppress: 6387) 

// Prototypes
BOOLEAN OpenBusInterface(IN HDEVINFO HardwareDeviceInfo,
                         IN PSP_INTERFACE_DEVICE_DATA pDevInterfaceData,
	                     ULONG BusNum);

#define USAGE  "/?" 
/*
"Usage: MAD_Enum [-p SerialNo] Plugs in a device. SerialNo must be greater than zero.\n\
			 [-u SerialNo or 0] Unplugs device(s) - specify 0 to unplug all \
								the devices enumerated so far.\n\
			 [-e SerialNo or 0] Ejects device(s) - specify 0 to eject all \
								the devices enumerated so far.\n"
*/

BOOLEAN gbPlugIn = FALSE, gbUnplug = FALSE, gbEject = FALSE;
LONG gnPowerState = -1;
ULONG gulSerialNo;

int _cdecl main(int argc, char* argv[])

{
BOOLEAN bRC = TRUE;
ULONG BusNum = 1;
GUID BusIntfGuid; //
HDEVINFO HardwareDevInfo;
SP_INTERFACE_DEVICE_DATA InterfaceDeviceData;
char cArgv1_0;
char cArgv1_1;
//char cArgv2   = argv[2][0];
char x;

	if (argc < 3)
		goto usage;

	cArgv1_0 = argv[1][0];
    cArgv1_1 = (char)tolower(argv[1][1]);

    if (cArgv1_0 != '-')
	    goto usage;

	memcpy(&BusIntfGuid, (LPGUID)&GUID_DEVINTERFACE_MADBUS_ONE, sizeof(GUID));
    gulSerialNo = (USHORT)atoi(argv[2]);

    switch (cArgv1_1)
        {
	    case 'p':  //* Plugin
	        if (gulSerialNo == 0)
		        goto usage;

		    gbPlugIn = TRUE;
		    break;

        case 'u': //* Unplug
			gbUnplug = TRUE;
            break;

        case 'e': //* Eject
			gbEject = TRUE;
            break;			

        case 's': //* Set Power State
			gnPowerState = atoi(&(argv[1][2])); 
            if ((gnPowerState < 0) || (gnPowerState > 3))
				goto usage;
     		break;			

		case 'z': //ScZi device - 2nd device class
			memcpy(&BusIntfGuid, (LPGUID)&GUID_DEVINTERFACE_MADBUS_TWO, sizeof(GUID));
			BusNum = 2;
			gulSerialNo = (USHORT)atoi(argv[2]);
			char cArgv1_2 = (char)tolower(argv[1][2]);

			switch (cArgv1_2)
			    {
			    case 'p':  //* Plugin
				    if (gulSerialNo == 0)
				    	goto usage;

				    gbPlugIn = TRUE;
				    break;

			    case 'u': //* Unplug
				    gbUnplug = TRUE;
				    break;

			    case 'e': //* Eject
				    gbEject = TRUE;
				    break;

			    case 's': //* Set Power State
				    gnPowerState = atoi(&(argv[1][3]));
				    if ((gnPowerState < 0) || (gnPowerState > 3))
					    goto usage;
				    break;
			    }
			break;

        default:
			goto usage;
		} //* end sw


// Open a handle to the device interface information set of all 
// present abstract bus enumerator interfaces.
	HardwareDevInfo = SetupDiGetClassDevs(&BusIntfGuid, //(LPGUID)&GUID_DEVINTERFACE_MADBUS_DEVICE,
						 	               NULL, // Define no enumerator (global)
						 	               NULL, // Define no
						 	               // Only Devices present, Function class devices.
						 	               (DIGCF_PRESENT | DIGCF_INTERFACEDEVICE));
	if (HardwareDevInfo == INVALID_HANDLE_VALUE)
		{
		printf("SetupDiGetClassDevs failed: %x\n", GetLastError());
		scanf_s("%c", &x, 1);
		exit(0);
		}

	InterfaceDeviceData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);
	bRC = SetupDiEnumDeviceInterfaces(HardwareDevInfo,
		                              0, // No care about specific PDOs
		                              &BusIntfGuid, //(LPGUID)&GUID_DEVINTERFACE_MADBUS_DEVICE,
		                              0, &InterfaceDeviceData);
	if (!bRC)
	    {
		printf("SetupDiEnumDeviceInterfaces failed!");
		scanf_s("%c", &x, 1);
        exit(0);
 	    }
   
	//* Here we finally do the work
   	OpenBusInterface(HardwareDevInfo, &InterfaceDeviceData, BusNum);

//* Normal exit
	SetupDiDestroyDeviceInfoList(HardwareDevInfo);
	return 0;

usage:
   printf(USAGE);
   exit(0);
}

BOOLEAN OpenBusInterface(IN HDEVINFO HardwareDeviceInfo,
	                     IN PSP_INTERFACE_DEVICE_DATA pDevInterfaceData,
	                     ULONG BusNum)
{
HANDLE hFile;
PSP_INTERFACE_DEVICE_DETAIL_DATA pDevInterfaceDetailData = NULL;
ULONG LenXpectd = 0;
ULONG LenRequired = 0;
ULONG bufrsize;
ULONG bytes_returned;
MADBUS_UNPLUG_HARDWARE sUnplug;
MADBUS_EJECT_HARDWARE sEject;
MADBUS_SET_POWER_STATE sSetPowrState;
PMADBUS_PLUGIN_HARDWARE pPluginHW;
char x;
BOOLEAN bRC = TRUE;

// Allocate a function class device data structure to receive the
// information about this particular device.
	SetupDiGetInterfaceDeviceDetail(HardwareDeviceInfo,
		                            pDevInterfaceData,
		                            NULL, // probing so no output buffer yet
		                            0, // probing so output buffer length of zero
		                            &LenRequired,
                             		NULL); // not interested in the specific dev-node

#pragma warning(suppress: 6102)
	LenXpectd = LenRequired;
	pDevInterfaceDetailData = malloc(LenXpectd);
	if (pDevInterfaceDetailData == NULL)
	    {
		printf("malloc for Device Interface Detail failed!\n");
		scanf_s("%c", &x, 1);
		return FALSE;
	    }

	pDevInterfaceDetailData->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
	if (!SetupDiGetInterfaceDeviceDetail(HardwareDeviceInfo,
		                               	 pDevInterfaceData,
		  	                             pDevInterfaceDetailData,
		  	                             LenXpectd,
		  	                             &LenRequired,
                              		  	 NULL))
		{
		printf("Error in SetupDiGetInterfaceDeviceDetail\n");
		free(pDevInterfaceDetailData);
		scanf_s("%c", &x, 1);
		return FALSE;
		}

	printf("Opening %ls\n", pDevInterfaceDetailData->DevicePath);

	hFile = CreateFile(pDevInterfaceDetailData->DevicePath,
                 	   GENERIC_READ | GENERIC_WRITE,
		               0, // FILE_SHARE_READ | FILE_SHARE_WRITE
		   	           NULL, // no SECURITY_ATTRIBUTES structure
		     	       OPEN_EXISTING, // No special create flags
		               0, // No special attributes
		   	           NULL); // No template file
	if (INVALID_HANDLE_VALUE == hFile)
		{
		printf("Device not ready: %x", GetLastError());
		free(pDevInterfaceDetailData);
		scanf_s("%c", &x, 1);
		return FALSE;
		}

	printf("Bus interface opened!!!\n");

// Enumerate Devices
	if (gbPlugIn)
		{
		printf("SerialNo. of the device to be enumerated: %d\n", gulSerialNo);

		bufrsize = (sizeof(MADBUS_PLUGIN_HARDWARE) + 20);
		pPluginHW = malloc(bufrsize); 
		if (pPluginHW == NULL)
		    {
			printf("malloc for PlugIn failed: \n");
			scanf_s("%c", &x, 1);
			goto End;
		    }

		memset(pPluginHW, 0x00, bufrsize); // sizeof(MADBUS_PLUGIN_HARDWARE));
		pPluginHW->Size = bufrsize; // (sizeof(MADBUS_PLUGIN_HARDWARE) + 2);
		pPluginHW->SerialNo = gulSerialNo;

// Allocate storage for the Device ID
		if (BusNum == 1)
			memcpy(pPluginHW->HardwareIDs, MADSIM_HARDWARE_ID1, 
				   (MADSIM_HARDWARE_ID_LENGTH1 * sizeof(WCHAR)));
		else
			memcpy(pPluginHW->HardwareIDs, MADSIM_HARDWARE_ID2, 
				   (MADSIM_HARDWARE_ID_LENGTH2 * sizeof(WCHAR)));

		if (!DeviceIoControl(hFile, IOCTL_MADBUS_PLUGIN_HARDWARE,
			 	             pPluginHW, bufrsize, pPluginHW, bufrsize,
			                 &bytes_returned, NULL))
			{
			printf("PlugIn failed:0x%x\n", GetLastError());
			scanf_s("%c", &x, 1); //Wait for input so that the user sees the failure
			}

		free(pPluginHW);
		goto End;
		}

// Removes a device if given the specific Id of the device. Otherwise this
// ioctls removes all the devices that are enumerated so far.
	if (gbUnplug)
		{
		printf("Unplugging device(s)....\n");

		sUnplug.Size = bufrsize = sizeof(sUnplug);
		sUnplug.SerialNo = gulSerialNo;
		if (!DeviceIoControl(hFile, IOCTL_MADBUS_UNPLUG_HARDWARE,
			                 &sUnplug, bufrsize, &sUnplug, bufrsize,
			                 &bytes_returned, NULL))
		    {
			printf("Unplug failed: 0x%x\n", GetLastError());
			scanf_s("%c", &x, 1);
		    }

		goto End;
		}

// Ejects a device if given the specific Id of the device. Otherwise this
// ioctls ejects all the devices that are enumerated so far.
	if (gbEject)
		{
		printf("Ejecting Device(s)\n");

		sEject.Size     = bufrsize = sizeof(sEject);
		sEject.SerialNo = gulSerialNo;
		if (!DeviceIoControl(hFile, IOCTL_MADBUS_EJECT_HARDWARE,
			                 &sEject, bufrsize, &sEject, bufrsize,
			                 &bytes_returned, NULL))
		    {
			printf("Eject failed: 0x%x\n", GetLastError());
			scanf_s("%c", &x, 1);
		    }

		goto End;
		}

// Issue a Power State change if given the specific Id of the device. Otherwise this
// ioctls sets power state for all devices enumerated so far.
	if (gnPowerState > -1)
		{
		printf("Setting Power State to %d\n", gnPowerState);

		sSetPowrState.Size           = bufrsize = sizeof(sSetPowrState);
		sSetPowrState.SerialNo       = gulSerialNo;
		sSetPowrState.nDevPowerState = gnPowerState;
		if (!DeviceIoControl(hFile, IOCTL_MADBUS_SET_POWER_STATE,
			                 &sSetPowrState, bufrsize, &sSetPowrState, bufrsize,
			                &bytes_returned, NULL))
	    	{
			printf("Set Power State failed: 0x%x\n", GetLastError());
			scanf_s("%c", &x, 1);
	    	}

		goto End;
		}

	printf("Success!!!\n");

End:;
    CloseHandle(hFile);
	free(pDevInterfaceDetailData);

	return TRUE;
}
