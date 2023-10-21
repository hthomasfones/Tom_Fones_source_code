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
/*  Exe file ID  : MadDevice.sys                                               */
/*                                                                             */
/*  Module  NAME : MadDevice.cpp                                               */
/*                                                                             */
/*  DESCRIPTION  : Main source module for the device-function driver           */
/*                 DriverEntry, DriverUnload, AddDevice,StartHardware          */
/*                 Derived from WDK-Toaster\func\func.c                        */
/*                 This is a featured version of the toaster function driver.  */
/*                 This version shows how to register for PNP and Power events,*/
/*                 handle create & close file requests,                        */
/*                 handle WMI set & query events, fire WMI notification events.*/
/*                                                                             */
/*******************************************************************************/

#include "../inc/MadDevice.h"
#ifdef WPP_TRACING
    // #pragma message("WPP_TRACING defined")
    #include "trace.h"
    #include "MadDevice.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, MadEvtAddDevice)
#pragma alloc_text (PAGE, MadEvtFileCreate)
#pragma alloc_text (PAGE, MadEvtFileClose)
#pragma alloc_text (PAGE, MadEvtPrepareHardware)
#pragma alloc_text (PAGE, MadEvtReleaseHardware)
#pragma alloc_text (PAGE, MadEvtContextCleanup)
#pragma alloc_text (PAGE, MadEvtIoDeviceControl)
#pragma alloc_text (PAGE, MadEvtIoBufrdRead)
#pragma alloc_text (PAGE, MadEvtIoBufrdWrite)
#endif

//Global variables
ULONG                    DebugLevel = 3; 
DRIVER_DATA              gDriverData;
PDRIVER_OBJECT           gpDriverObj;

#ifdef MAD_DEVICE_CONTROL_POWER_MNGT
LONG  gPowrChekTimrMillisecs = MAD_DFLT_POWER_CHECK_DELAY;
#endif

/************************************************************************//**
 * DriverEntry
 *
 * DESCRIPTION:
 *    DriverEntry initializes the driver and is the first routine called by the
 *    system after the driver is loaded. DriverEntry specifies the other entry
 *    points in the function driver, such as AddDevice and Unload.
 *    
 * PARAMETERS: 
 *     @param[in]  pDriverObj     pointer to the driver object 
 *     @param[in]  pRegistryPath pointer to a unicode string representing
 *                               the path, to driver-specific registry key
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS DriverEntry(IN PDRIVER_OBJECT  pDriverObj,
                     IN PUNICODE_STRING pRegistryPath)
{
NTSTATUS            NtStatus = STATUS_SUCCESS;
WDF_DRIVER_CONFIG   DriverConfig;
UNICODE_STRING      ParmSubkey;
UNICODE_STRING      ParmSubkeyPath;

#ifdef WPP_TRACING
	WPP_INIT_TRACING(pDriverObj, pRegistryPath);
#endif

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDevice:DriverEntry build timestamp ... %s|%s\n", BUILD_TIME, BUILD_DATE);

    // Initialize driver config to control the attributes that are global to the driver.
    // Note that framework by default provides a driver unload routine.
    // If you create any resources in the DriverEntry and want to be cleaned in driver unload,
    // you can override that by manually setting the EvtDriverUnload in the config structure.
    // In general xxx_CONFIG_INIT macros are provided to initialize most commonly used members.
    //
    WDF_DRIVER_CONFIG_INIT(&DriverConfig, MadEvtAddDevice); 
    DriverConfig.EvtDriverUnload = MadEvtDriverUnload;
    DriverConfig.DriverPoolTag   = MAD_DEV_POOL_TAG;

    // Create a framework driver object to represent our driver.
    //
    NtStatus = WdfDriverCreate(pDriverObj,
                               pRegistryPath,
                               WDF_NO_OBJECT_ATTRIBUTES, // Driver Attributes
                               &DriverConfig,            // Driver Config Info
                               &gDriverData.hDriver);
    if (!NT_SUCCESS(NtStatus)) 
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadDevice:DriverEntry:WdfDriverCreate...failed with NtStatus 0x%x\n",
                    NtStatus);

		MadWriteEventLogMesg(pDriverObj, MADDEVICE_DRIVER_LOAD_ERROR, 1, 
                             sizeof(NTSTATUS), (PWSTR)&NtStatus); //Minimal payload
		return NtStatus;
        }

    gpDriverObj = pDriverObj;
    PDRIVER_DATA pDriverData = &gDriverData; // MadDevDriverGetContextData(ghDriver);
    pDriverData->pDriverObj = pDriverObj;
    pDriverData->bDmaEnabled = MAD_DFLT_DMA_ENABLED;

	//Save the WDM pointer to our driver-obj & it's service key registry path because it isn't static
	//gpDriverObj = pDriverObj;
    pDriverData->RegistryPath.Buffer = NULL;
    NtStatus = MadSaveUnicodeString(&gDriverData.RegistryPath, pRegistryPath);
    ASSERT(NtStatus == STATUS_SUCCESS);
    
	//Build our parameter subkey registry name and fetch our registry parameters
	ParmSubkeyPath.Buffer = NULL;
	RtlInitUnicodeString(&ParmSubkey, PARMS_SUBKEY);
    NtStatus = 
    MadInitSubkeyPath(pDriverObj, pRegistryPath, &ParmSubkey, &ParmSubkeyPath);
    ASSERT(NtStatus == STATUS_SUCCESS);

	NtStatus = MadDev_ReadRegParms(pDriverObj, &ParmSubkeyPath, &gDriverData.bDmaEnabled);
	ASSERT(NtStatus == STATUS_SUCCESS);

	if (ParmSubkeyPath.Buffer != NULL)
	    ExFreePool(ParmSubkeyPath.Buffer);

	MadWriteEventLogMesg(pDriverObj, MADDEVICE_DRIVER_LOAD, 0, 0, NULL); //Hard-coded message text. No payload

	return NtStatus;
}

/************************************************************************//**
 * MadEvtDriverUnload
 *
 * DESCRIPTION:
 *    This function is the framework call-back when we need to close up,
 *    and/or free up resources 
 *    
 * PARAMETERS: 
 *     @param[in]  hDriver     handle to the driver object 
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
VOID MadEvtDriverUnload (IN WDFDRIVER hDriver)

{
    PDRIVER_OBJECT pDriverObj = WdfDriverWdmGetDriverObject(hDriver);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtDriverUnload...just saying adios\n");
    ASSERT(hDriver == gDriverData.hDriver);
	UNREFERENCED_PARAMETER(hDriver);

	if (gDriverData.RegistryPath.Buffer != NULL)
	    ExFreePool(gDriverData.RegistryPath.Buffer);

#ifdef WPP_TRACING
  	WPP_CLEANUP(pDriverObj);
#endif

	MadWriteEventLogMesg(pDriverObj, MADDEVICE_DRIVER_UNLOAD,
                         0, 0, NULL); //Hard-coded message text. No payload

    return;
}

/************************************************************************//**
 * MadEvtAddDevice 
 *
 * DESCRIPTION:
 *    MadEvtAddDevice is called by the framework in response to AddDevice
 *    call from the PnP manager. We create and initialize a device object to
 *    represent a new instance of Model-Abstract-Demo Device.
 *
 * PARAMETERS: 
 *     @param[in]  hDriver     handle to the driver object  
 *     @param[in]  pDeviceInit pointer to the framework device init object
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 *    
 ***************************************************************************/
NTSTATUS MadEvtAddDevice(IN WDFDRIVER  hDriver, IN PWDFDEVICE_INIT pDeviceInit)
{
static ULONG                          PluginNum = 0;
static WCHAR                          wcDigits[] = MAD_OBJECTNAME_UNITNUM_WSTR;
//
PDRIVER_OBJECT pDriverObj = WdfDriverWdmGetDriverObject(hDriver);
NTSTATUS                              NtStatus = STATUS_SUCCESS;
WDF_PNPPOWER_EVENT_CALLBACKS          PnpPowerCallbacks;
WDF_OBJECT_ATTRIBUTES                 FdoAttrs;
WDFDEVICE                             hDevice;
WDF_FILEOBJECT_CONFIG                 fileConfig;
WDF_POWER_POLICY_EVENT_CALLBACKS      powerPolicyCallbacks;
WDF_IO_QUEUE_CONFIG                   queueConfig;
WDFQUEUE                              hQueue;
PFDO_DATA                             pFdoData;
PDEVICE_OBJECT                        pDevObj;
PDEVICE_OBJECT                        pLowrDev;
WCHAR                                 wcNtMadDeviceName[] = MADDEV_NT_DEVICE_NAME_WSTR;
UNICODE_STRING                        NtDeviceName;

    PAGED_CODE();	
    UNREFERENCED_PARAMETER(hDriver);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadEvtAddDevice enter\n");

    // Initialize the PnpPowerCallbacks structure.  
    // Callback events for PNP and Power are specified here.
    // If we don't supply any callbacks, the Framework will take appropriate default actions
	// based on whether pDeviceInit is initialized to be an FDO, a PDO or a filter device object.
    MadDeviceSetPnpPowerCallbacks(&PnpPowerCallbacks);
    WdfDeviceInitSetPnpPowerEventCallbacks(pDeviceInit, &PnpPowerCallbacks);

    // Register power policy event callbacks so that we would know when to arm/disarm 
	// the hardware to handle wait-wake and when the wake event is triggered by the hardware.
    MadDeviceSetPowerPolicyCallbacks(&powerPolicyCallbacks);
    WdfDeviceInitSetPowerPolicyEventCallbacks(pDeviceInit, 
                                              &powerPolicyCallbacks);

 	//Let's name our device object so we can find it in the object data base
	//Using Sysinternals:WinObj.exe / OSR:DeviceTree.exe
	PluginNum++; //Not necessarily the SerialNo
	wcNtMadDeviceName[MADDEV_NT_DEVICE_NAME_UNITID_INDX] = wcDigits[PluginNum];
	RtlInitUnicodeString(&NtDeviceName, wcNtMadDeviceName);

	//When we do this we prevent the test app from opening the device by symbolic name / or interface_guid *!*
	//NtStatus = WdfDeviceInitAssignName(pDeviceInit, &NtDeviceName);
	if (!NT_SUCCESS(NtStatus))
	    {
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		            "MadEvtAddDevice:WdfDeviceInitAssignName returned x%X; the device object will be unnamed\n",
					NtStatus);
	    }

    //  WDF_FILEOBJECT_CONFIG_INIT struct to tell the framework whether we are interested
	// in handling Create, Close and Cleanup requests that gets genereate when an app or 
	// kernel component opens a handle to the device.
    // If we don't register, the framework default behaviour is to complete these requests with STATUS_SUCCESS.
    // A driver might be interested in registering these events if it wants to do security validation
	// and also wants to maintain per handle (fileobject) context.
    WDF_FILEOBJECT_CONFIG_INIT(&fileConfig,
                               MadEvtFileCreate,
                               MadEvtFileClose,
                               WDF_NO_EVENT_CALLBACK); // not interested in Cleanup
 
    WdfDeviceInitSetFileObjectConfig(pDeviceInit,
                                     &fileConfig, WDF_NO_OBJECT_ATTRIBUTES);

    //More device init parms
	//Will default to WdfDeviceIoBuffered unless ...
    if (gDriverData.bDmaEnabled) //Direct I/O, no buffering sector-aligned, random-access 
        WdfDeviceInitSetIoType(pDeviceInit, WdfDeviceIoDirect); //DMA w. MDLs
	
    // When we are ready to define a known setup class ...
    //WdfDeviceInitSetDeviceClass(pDeviceInit, &GUID_DEVCLASS_DISKDRIVE);

    // Now specify the size of device extension where we track per device context.
    // Along with setting the context type as shown below, you should also specify the WDF_DECLARE_CONTEXT_TYPE_WITH_NAME 
    // in header to specify the accessor function name.
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&FdoAttrs, FDO_DATA);

    // Set a context cleanup routine to cleanup any resources that are not parent to this device.
    // This cleanup will be called in the context of pnp remove-device when the framework deletes the device object.
    #pragma warning(suppress: 28023)
    FdoAttrs.EvtCleanupCallback = (PFN_WDF_OBJECT_CONTEXT_CLEANUP)MadEvtContextCleanup; 

    // DeviceInit is completely initialized. 
    // Call the framework to create the device and attach it to the lower stack.
    NtStatus = WdfDeviceCreate(&pDeviceInit, &FdoAttrs, &hDevice);
    if (!NT_SUCCESS(NtStatus)) 
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadEvtAddDevice:WdfDeviceCreate failed...NtStatus x%X\n",
                    NtStatus);

		MadWriteEventLogMesg(pDriverObj, MADDEVICE_CREATE_DEVICE_ERROR, 1, 
                             sizeof(NTSTATUS), (PWSTR)&NtStatus); //Minimal payload
        return NtStatus;
        }

	ASSERT(pDeviceInit == NULL); //Because WdfDeviceCreate is supposed to set this

    // Get the device context by using accessor function specified in the 
	// WDF_DECLARE_CONTEXT_TYPE_WITH_NAME macro for FDO_DATA.
    pFdoData = MadDeviceFdoGetData(hDevice);
	pFdoData->hDevice = hDevice;
    pFdoData->pDriverObj = pDriverObj;
    pFdoData->pDriverData = &gDriverData;

    //Get the MadBus configuration space parameters
    pDevObj  = WdfDeviceWdmGetDeviceObject(hDevice);
    pLowrDev = WdfDeviceWdmGetAttachedDevice(hDevice);
	pFdoData->pPhysDevObj = WdfDeviceWdmGetPhysicalDevice(hDevice);
    NtStatus = MadDev_ProcessConfig(pLowrDev, pDevObj, pFdoData);
    if (!NT_SUCCESS(NtStatus)) 
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadEvtAddDevice:MadDev_ProcessConfig returned NtStatus=x%X\n",
                    NtStatus);

		MadWriteEventLogMesg(pDriverObj, MADDEVICE_CREATE_DEVICE_ERROR, 1, 
                             sizeof(NTSTATUS), (PWSTR)&NtStatus); //Minimal payload
        return NtStatus;
        }

    //One-shot use to create a registry subkey:Value with config data
    //NtStatus = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, gpRegistryPath->Buffer, 
    //                                 ValueName, REG_NONE, &pFdoData->DevCnfgData, sizeof(MadBus_DEVICE_CONFIG_DATA));
    //ASSERT(NtStatus == STATUS_SUCCESS);

    // Tell the Framework that this device will need an interface so that an
    // application can find our device and talk to it.
    NtStatus =
    WdfDeviceCreateDeviceInterface(hDevice, 
                                   (LPGUID)&GUID_DEVINTERFACE_MADDEVICE, NULL);
    if (!NT_SUCCESS (NtStatus)) 
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadEvtAddDevice:WdfDeviceCreateDeviceInterface failed! ntstatus=x%x\n",
                    NtStatus);

		MadWriteEventLogMesg(pDriverObj, MADDEVICE_CREATE_DEVICE_ERROR, 1, 
                             sizeof(NTSTATUS), (PWSTR)&NtStatus); //Minimal payload
        return NtStatus;
        }

    // Register I/O callbacks; tell the framework that we are interested in handling 
    // IRP_MJ_READ, IRP_MJ_WRITE, and IRP_MJ_DEVICE_CONTROL requests.
    // In case a specific handler is not specified for one of these,
	// the request will be dispatched to the EvtIoDefault handler, if any.
    // If there is no EvtIoDefault handler, the request will be failed with STATUS_INVALID_DEVICE_REQUEST.
    // WdfIoQueueDispatchParallel means that we are capable of handling all the I/O request simultaneously 
    // and we are responsible for protecting data that could be accessed by these callbacks simultaneously.
    // A default Q gets all requests not configure-forwarded using WdfDeviceConfigureRequestDispatching.
    //Prototype caveat: serialized i/o keeping the prototype simple
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig,
                                           WdfIoQueueDispatchSequential);
    
	if (!pFdoData->pDriverData->bDmaEnabled) // Buffered I/O, (16)byte-aligned, sequential access                 
		{ 
        queueConfig.EvtIoRead  = MadEvtIoBufrdRead;
        queueConfig.EvtIoWrite = MadEvtIoBufrdWrite;
	    }
	else  //Direct I/O, no buffering, sector-aligned, random-access 
        {
		queueConfig.EvtIoRead  = MadEvtIoDmaRead;
        queueConfig.EvtIoWrite = MadEvtIoDmaWrite;
        }

    queueConfig.EvtIoDeviceControl = MadEvtIoDeviceControl;

    NtStatus = 
    WdfIoQueueCreate(hDevice, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &hQueue);
    if (!NT_SUCCESS (NtStatus)) 
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadEvtAddDevice:WdfIoQueueCreate failed...0x%x\n",
                    NtStatus);

		MadWriteEventLogMesg(pDriverObj, MADDEVICE_CREATE_DEVICE_ERROR, 1, 
                             sizeof(NTSTATUS), (PWSTR)&NtStatus); //Minimal payload
        return NtStatus;
        }

    if (pFdoData->pDriverData->bDmaEnabled) // Create a dma enabler
        {
		WdfDeviceSetAlignmentRequirement(hDevice, MAD_DMA_ALIGN_REQ); 

		WDF_DMA_ENABLER_CONFIG  DmaConfig; 
        WDF_DMA_ENABLER_CONFIG_INIT(&DmaConfig, MAD_DMA_PROFILE,
			                        (MAD_SECTOR_SIZE * MAD_DMA_MAX_SECTORS));
  		NtStatus = 
		MadDmaEnablerCreate(hDevice, &DmaConfig, WDF_NO_OBJECT_ATTRIBUTES, 
                            &pFdoData->hDmaEnabler); 
        if (!NT_SUCCESS(NtStatus)) 
	        {
            TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		  	            "MadEvtAddDevice:WdfDmaEnablerCreate failed...status=x%X\n",
                        NtStatus);

			MadWriteEventLogMesg(pDriverObj, MADDEVICE_CREATE_DEVICE_ERROR, 1, 
                                 sizeof(NTSTATUS), (PWSTR)&NtStatus); //Minimal payload
            return NtStatus;
            }

		MadDmaEnablerSetMaximumScatterGatherElements(pFdoData->hDmaEnabler,
                                                     MAD_DMA_MAX_SECTORS);

        //Prototype caveat: 
        //We do this once if we only have one static Chained-Dma List. (The hardware Scatter-Gather list)
        pFdoData->liCDPP = MmGetPhysicalAddress(pFdoData->HdwSgDmaList);
        }

#ifdef MAD_HOST_CONTROL_POWER_MNGT  //========================================================================
	// Taken straight from the WDK sample for the toaster device
    // Set the idle power policy to put the device to Dx if the device is not used for the specified time.
    // Since this is a virtual device we tell the framework that we cannot wake ourself if we sleep in S0. 
    // The only way the device can be brought to D0 is if the device receives an I/O from the system.
    //
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS IdleParms;
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&IdleParms, IdleCannotWakeFromS0);
    IdleParms.IdleTimeout = MAD_IDLE_TIMEOUT; 
    NtStatus = WdfDeviceAssignS0IdleSettings(hDevice, &IdleParms);
    if (!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadEvtAddDevice:WdfDeviceAssignS0IdleSettings failed...status=x%X\n", NtStatus);
        //return NtStatus;
        }

	// Set the wait-wake policy.
    //
	WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS WakeSetngs;
    WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_INIT(&WakeSetngs);
    NtStatus = WdfDeviceAssignSxWakeSettings(hDevice, &WakeSetngs);
    if (!NT_SUCCESS(NtStatus)) 
        {
        // We are probably enumerated on a bus that doesn't support Sx-wake.
        // Let us not fail the device add just because we aren't able to support wait-wake. 
        // Let the user decide how important it is to support wait-wake
        // for their hardware and return appropriate NtStatus.
        //
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadEvtAddDevice:WdfDeviceAssignSxWakeSettings failed...status=x%X\n", NtStatus);
        NtStatus = STATUS_SUCCESS;
        } 
#endif //MAD_HOST_CONTROL_POWER_MNGT ...................................................................
//
#ifdef MAD_DEVICE_CONTROL_POWER_MNGT //========================================================================
    // Set up the power mngt timer
    // We will get device power state by polling config space with the cooperation of the bus driver
    //
	BOOLEAN                bRC = FALSE;
	NTSTATUS               NtStatTimr = STATUS_SUCCESS;   
	WDF_TIMER_CONFIG       PowrChekTimrCnfg;
    WDF_OBJECT_ATTRIBUTES  PowrChekTimrObjAttrs;
    WDF_TIMER_CONFIG_INIT(&PowrChekTimrCnfg, (PFN_WDF_TIMER)MadEvtPowerCheckTimer);
    PowrChekTimrCnfg.Period                 = 0;    //Not a periodic timer - no repitition
    PowrChekTimrCnfg.AutomaticSerialization = TRUE;
    PowrChekTimrCnfg.TolerableDelay         = 10;   //milliseconds - arbitrary;

    WDF_OBJECT_ATTRIBUTES_INIT(&PowrChekTimrObjAttrs);
    PowrChekTimrObjAttrs.ExecutionLevel = WdfExecutionLevelPassive; //Our timer function will be a work-item, not a DPC
    PowrChekTimrObjAttrs.ParentObject   = hDevice; 

    NtStatTimr =
	WdfTimerCreate(&PowrChekTimrCnfg, &PowrChekTimrObjAttrs, &pFdoData->hPowrChekTimr);
    if (NtStatTimr != STATUS_SUCCESS)
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadEvtAddDevice:WdfTimerCreate...returned x%X - we will carry on w/out aggressive power mngt\n",
                    NtStatTimr);
        }
    else
        {
        pFdoData->PowrChekTimrPeriod = -(10 * 1000 * gPowrChekTimrMillisecs);
        bRC = WdfTimerStart(pFdoData->hPowrChekTimr, pFdoData->PowrChekTimrPeriod); 
        ASSERT(!bRC); //Because our timer should not already be in the system timer queue
        }
#endif //MAD_DEVICE_CONTROL_POWER_MNGT .........................................................................

#ifdef CREATE_SYMBOLIC_LINK //Else we force the application to know our interface guid for all devices ======
	                        //This works with the #define MAD_POWER_USER in the test app
    WCHAR          wcMadSymLinkName[] = MADDEV_DOS_DEVICE_NAME_WSTR;
    UNICODE_STRING MadSymLinkName;
	ULONG          UnitNum = pFdoData->SerialNo;

    #ifdef FORCE_TESTAPP_INTERFACE_GUID_AWARE
    //Use a hard-coded suffix & name which will be duplicate and fail to create after the 1st instance.
    //Force the test app to know the Interface GUID to open subsequent units (2..9)
	//This works with the define MAD_POWER_USER in the test app
    UnitNum = 1;
    #endif

	wcMadSymLinkName[MADDEV_DOS_DEVICE_NAME_UNITID_INDX] = wcDigits[UnitNum];
    RtlInitUnicodeString(&MadSymLinkName, wcMadSymLinkName);
    NtStatus = WdfDeviceCreateSymbolicLink(hDevice, &MadSymLinkName);
    if (NtStatus != STATUS_SUCCESS)
	    { //Brackets around null stmt in the case of Free_build & No WPP_TRACING(KdPrint)
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		            "MadEvtAddDevice:WdfDeviceCreateSymbolicLink returned...x%X - no symbolic link/dos name created!\n",
                    NtStatus);
	    }
#endif //CREATE_SYMBOLIC_LINK ...............................................................................

    // Finally register all our WMI datablocks with WMI subsystem.
    NtStatus = MadDeviceWmiRegistration(hDevice);
    if (NtStatus != STATUS_SUCCESS)
	    { //Brackets around null stmt in the case of Free_build & No WPP_TRACING(KdPrint)
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		            "MadEvtAddDevice:MadDeviceWmiRegistration ... returned x%X - continuing\n", NtStatus);
	    }

    MadDevInitFDO(pFdoData);

    // Please note that if this event fails or the device eventually gets removed the framework will automatically
    // take care of deregistering with WMI, detaching and deleting the deviceobject and cleaning up other resources. 
    // Framework does most of the resource cleanup during device remove and driver unload.
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtAddDevice exit...serialNo=%d, NtStatus=x%X\n", 
                pFdoData->SerialNo, NtStatus);

    return NtStatus;
}

/************************************************************************//**
 * MadDevInitFDO 
 *
 * DESCRIPTION:
 *    This function initializes the framework device extension for the device.
 *    
 * PARAMETERS: 
 *     @param[in]  pFdoData    pointer to the framework device extension
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
VOID  MadDevInitFDO(PFDO_DATA pFdoData)

{
register ULONG j;

    for (j = eBufrdRead; j < eMltplIO; j++)
		pFdoData->hPendingReqs[j] = NULL;
 
    return;
}

/**************************************************************************//**
 * MadEvtPrepareHardware
 *
 * DESCRIPTION:
 *   This function is the EvtDevicePrepareHardware event callback which
 *   performs operations necessary to make the driver's device operational.
 *   The framework calls the driver's EvtDevicePrepareHardware callback when
 *   the PnP manager sends an IRP_MN_START_DEVICE irp to the driver stack.
 *
 *   Specifically, most drivers will use this callback to map resources.
 *   USB drivers may use it to get device PartlResrcDescs, 
 *   config PartlResrcDescs and to select configs.
 *
 *   Some drivers may choose to download firmware to a device in this callback,
 *   but that is usually only a good choice if the device firmware won't be
 *   destroyed by a D0 to D3 transition.  If firmware will be gone after D3,
 *   then firmware downloads should be done in EvtDeviceD0Entry, not here.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice      handle to our device 
 *     @param[in]  hResrcsRaw   Handle to a collection of framework resource 
 *                              objects. This collection identifies the raw 
 *                              (bus-relative) hardware resources that have 
 *                              been assigned to the device.
 *     @param[in]  hResrcsXlatd Handle to a collection of framework resource
 *                              objects. This collection identifies the
 *                              translated (system-physical) hardware 
 *                              resources assigned to the device.
 *                              The resources are from the CPU's view-point.
 *                              Use these resources to map I/O space & device-
 *                              accessible memory into virtual addr space
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 *****************************************************************************/
NTSTATUS MadEvtPrepareHardware(WDFDEVICE hDevice, WDFCMRESLIST hResrcsRaw,
                               WDFCMRESLIST hResrcsXlatd)
{
register LONG i;
PFDO_DATA   pFdoData  = MadDeviceFdoGetData(hDevice);
ULONG       SerialNo  = pFdoData->SerialNo;
PCM_PARTIAL_RESOURCE_DESCRIPTOR pPartlResrcDesc;
PCM_PARTIAL_RESOURCE_DESCRIPTOR pPartlResrcDescRaw   = 
                                WdfCmResourceListGetDescriptor(hResrcsRaw, 0);
PCM_PARTIAL_RESOURCE_DESCRIPTOR pPartlResrcDescXlatd = 
                                WdfCmResourceListGetDescriptor(hResrcsXlatd, 0);
LONG ResrcCount   = WdfCmResourceListGetCount(hResrcsXlatd);
NTSTATUS NtStatus = STATUS_SUCCESS;
LONG MemCount     = 0;

    PAGED_CODE();
    UNREFERENCED_PARAMETER(hDevice);
    UNREFERENCED_PARAMETER(pPartlResrcDescRaw);
    UNREFERENCED_PARAMETER(pPartlResrcDescXlatd);
	UNREFERENCED_PARAMETER(SerialNo);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtPrepareHardware enter...SerialNo=%d\n", SerialNo);

    SIMULATION_ASSERT(ResrcCount==MADDEV_TOTAL_CM_RESOURCES);//Simulation-aware sanity check  
    SIMULATION_ASSERT(WdfCmResourceListGetCount(hResrcsRaw)==MADDEV_TOTAL_CM_RESOURCES);//Simulation-aware sanity check  

    // Get the number of items that are currently in Resources collection and
    // iterate thru as many times to get more information about each item
    for (i=0; i < ResrcCount; i++) 
        {
        pPartlResrcDesc = WdfCmResourceListGetDescriptor(hResrcsXlatd, i);

        switch(pPartlResrcDesc->Type) 
            {
            case CmResourceTypePort: //Where the registers are
                TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
					        "MadEvtPrepareHardware...I/O Port (%x), Length (%d), SerialNo=%d\n",
                            pPartlResrcDesc->u.Port.Start.LowPart, 
                            pPartlResrcDesc->u.Port.Length, SerialNo);

                SIMULATION_ASSERT(pPartlResrcDesc->u.Port.Start.LowPart==pFdoData->BARs[0]);  
                pFdoData->pMadRegs = 
                (PMADREGS)MmMapIoSpaceEx(pPartlResrcDesc->u.Port.Start,
					                     pPartlResrcDesc->u.Port.Length, MAD_MEM_MAPIO_FLAGS);
                break;

            case CmResourceTypeInterrupt:
                TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
					        "MadEvtPrepareHardware...Interrupt level x%X, Vector x%X, Affinity x%X, SerialNo=%d\n", 
                            pPartlResrcDesc->u.Interrupt.Level,
                            pPartlResrcDesc->u.Interrupt.Vector, 
                            (ULONG)pPartlResrcDesc->u.Interrupt.Affinity,
                            SerialNo);

                //SIMULATION_ASSERT(pPartlResrcDesc->u.Interrupt.Level==pFdoData->Irql);
                {
                KAFFINITY DevAffin = 
                          pPartlResrcDesc->u.Interrupt.Affinity &
                          (KAFFINITY)((ULONG_PTR)0x01 << (ULONG_PTR)pFdoData->SerialNo);
                UNREFERENCED_PARAMETER(DevAffin);
                SIMULATION_ASSERT(DevAffin == (KAFFINITY)((ULONG_PTR)0x01 << (ULONG_PTR)pFdoData->SerialNo));
                }
                pFdoData->Irql        = (KIRQL)pPartlResrcDesc->u.Interrupt.Level;
                pFdoData->IDTindx     = pPartlResrcDesc->u.Interrupt.Vector;
                pFdoData->IntAffinity = pPartlResrcDesc->u.Interrupt.Affinity;

                //Connect the Interrupt below after collecting all Config Mngt resources
                break;

            case CmResourceTypeMemory: //Auxiliary device memory
                TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
					        "MadEvtPrepareHardware...Memory (%x), Length (%d), SerialNo=%d\n",
                            pPartlResrcDesc->u.Memory.Start.LowPart, 
                            pPartlResrcDesc->u.Memory.Length, SerialNo);

                MemCount++;
                SIMULATION_ASSERT(pPartlResrcDesc->u.Memory.Start.LowPart==pFdoData->BARs[MemCount]);  
                if (MemCount == 1)
                    pFdoData->pMadPioRead = 
                    MmMapIoSpaceEx(pPartlResrcDesc->u.Memory.Start,
					               pPartlResrcDesc->u.Memory.Length, 
                                   MAD_MEM_MAPIO_FLAGS);
                else
                    pFdoData->pMadPioWrite = 
                    MmMapIoSpaceEx(pPartlResrcDesc->u.Memory.Start,
					               pPartlResrcDesc->u.Memory.Length,
                                   MAD_MEM_MAPIO_FLAGS);
                break;

            default:
                TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				"MadEvtPrepareHardware... SerialNo=%d Unknown resource type (%d)\n",
                            SerialNo, pPartlResrcDesc->Type);
                break;
            } //end switch
        } //next i

    SIMULATION_ASSERT(MemCount==2);  
	SIMULATION_ASSERT((ULONG_PTR)pFdoData->pMadPioWrite > (ULONG_PTR)pFdoData->pMadPioRead);
	//
	if ((pFdoData->pMadPioRead == NULL) || (pFdoData->pMadPioWrite == NULL)) 
	    {//Driver-Verifier might force fail an allocation.
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
        "MadEvtPrepareHardware failing because of insufficient resources... SerialNo=%d\n",
                    SerialNo);
		return STATUS_INSUFFICIENT_RESOURCES;
	    }

    // Connect the interrupt
    // Connecting in PrepareHardware (in Start_Device) requires KMDF 1.11
     NtStatus = MadDevWdfCreateInt(hDevice, hResrcsRaw, hResrcsXlatd);
    if (!NT_SUCCESS(NtStatus))
		return NtStatus;
 
	// Deliver the arrival event to all WMI data consumers
    NtStatus =  MadDevWmiFireArrivalEvent(hDevice);
	if (!NT_SUCCESS(NtStatus)) 
	    { //Brackets around null stmt in the case of Free_build & No WPP_TRACING(KdPrint)
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
					"MadEvtPrepareHardware:MadDevWmiFireArrivalEvent... ntstatus=x%X - continuing, SerialNo=%d\n",
					NtStatus, SerialNo);
	    }

    #ifdef _MAD_SIMULATION_MODE_ //Prime the simulator's interrupt thread with needed parameters
	PMAD_SIMULATION_INT_PARMS pMadSimIntParms = pFdoData->pMadSimIntParms;
    pMadSimIntParms->u.WdfIntParms.hInterrupt       = pFdoData->hInterrupt;
	pMadSimIntParms->u.WdfIntParms.hSpinlock        = pFdoData->hIntSpinlock;
	pMadSimIntParms->u.WdfIntParms.Irql             = pFdoData->Irql;
	pMadSimIntParms->u.WdfIntParms.IntAffinity      = pFdoData->IntAffinity;
	pMadSimIntParms->u.WdfIntParms.pMadEvtIsrFunxn  = MadEvtISR;
    #endif //_MAD_SIMULATION_MODE_

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
    "MadEvtPrepareHardware... SerialNo=%d pMadRegs=%p pPioRd=%p pPioWr=%p\n",
                SerialNo, pFdoData->pMadRegs, 
                pFdoData->pMadPioRead, pFdoData->pMadPioWrite);

    return STATUS_SUCCESS;
}

/************************************************************************//**
 * MadDevWdfCreateInt
 *
 * DESCRIPTION:
 *    This function creates the WDF interrupt object based on configuration
 *    management parameters retrieved in the PrepareHardware function above.
 *    It is a helper function for the above function. 
 *    This function is pure vanilla/white-bread.  No simulation issues.
 *    
 * PARAMETERS: 
 *      @param[in]  hDevice      handle to our device 
 *      @param[in]  hResrcsRaw   see the description above
 *      @param[in]  hResrcsXlatd see the description above
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadDevWdfCreateInt(WDFDEVICE hDevice, 
                            WDFCMRESLIST hResrcsRaw, WDFCMRESLIST hResrcsXlatd)

{
PFDO_DATA             pFdoData = MadDeviceFdoGetData(hDevice);
NTSTATUS              NtStatus;
WDF_OBJECT_ATTRIBUTES SpinLockAttrs;
WDF_INTERRUPT_CONFIG  IntConfig;
WDF_WORKITEM_CONFIG   WiConfig;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			    "MadDevWdfCreateInt SerialNo=%d\n", pFdoData->SerialNo);

    WDF_OBJECT_ATTRIBUTES_INIT(&SpinLockAttrs);
    SpinLockAttrs.ParentObject = hDevice;

    NtStatus = WdfSpinLockCreate(&SpinLockAttrs, &pFdoData->hIntSpinlock);
    if (!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadDevWdfCreateInt:WdfSpinLockCreate... ntstatus=x%X\n",
                    NtStatus);
        return NtStatus;
        }

    WDF_INTERRUPT_CONFIG_INIT(&IntConfig, MadEvtISR, MadEvtDPC);
    IntConfig.SpinLock            = pFdoData->hIntSpinlock;
    IntConfig.ShareVector         = WdfUseDefault; 
    //IntConfig.AutomaticSerialization = FALSE; 
    IntConfig.EvtInterruptEnable  = MadEvtIntEnable;
    IntConfig.EvtInterruptDisable = MadEvtIntDisable;
    IntConfig.InterruptRaw        = WdfCmResourceListGetDescriptor(hResrcsRaw, 0);   //The head of the list
    IntConfig.InterruptTranslated = WdfCmResourceListGetDescriptor(hResrcsXlatd, 0); //The head of the list

	WDF_OBJECT_ATTRIBUTES IntObjAttrs;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&IntObjAttrs, ISR_DPC_WI_CONTEXT);

    #ifdef _MAD_SIMULATION_MODE_ //Beware - these comments apply ... ==================================== ***
	//WdfInterruptCreate will create a KMDF interrupt object handle and return STATUS_SUCCESS even
	//when we plug in arbitrary Config Mngt parameters not blessed by the PnP Mngr.
	//KMDF does not call IoConnectInterrupt(Ex) here.
	//The underlying interrupt object won't be created nor will the IDT vector be reserved or used!
	//However an interrupt handle with the associated spinlock will be created which is sufficient
	//for our purpose.
	//
	//*** We cannot endure WDF driver-verifier here! It should be set to OFF(0) in the registry.    ***
	//
	//*** We must have WDfCoinstaller 1.11(+).dll installed to enable connecting the interrupt      ***
	//*** here in the Start_device irp!                                                            	***
	//*** In Vs201x - in the Driver Model Settings - Type of Driver must be KMDF of course,         ***
	//*** Driver Major Version should be 1 & Driver Minor Version should be 11                      ***
	//
    #endif //_MAD_SIMULATION_MODE_ ==========================================================================

    NtStatus = 
    WdfInterruptCreate(hDevice, &IntConfig, &IntObjAttrs, &pFdoData->hInterrupt);
	
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDevWdfCreateInt:WdfInterruptCreate... ntstatus=x%X\n",
                NtStatus);
    if (!NT_SUCCESS(NtStatus))
        return NtStatus;

	PISR_DPC_WI_CONTEXT pIsrDpcWiData = 
    MadGetIsrDpcWiContext(pFdoData->hInterrupt);
	pIsrDpcWiData->IsrDpcWiTag = eIsrDpc;
	
	//Set up the work item(s)
 	WDF_OBJECT_ATTRIBUTES WiObjAttrs;
    WDF_OBJECT_ATTRIBUTES_INIT(&WiObjAttrs);
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&WiObjAttrs, ISR_DPC_WI_CONTEXT);
    WiObjAttrs.ParentObject = pFdoData->hInterrupt; 
	
	if (pFdoData->pDriverData->bDmaEnabled) //We need a DMA work item
	    {
	    WDF_WORKITEM_CONFIG_INIT(&WiConfig, (PFN_WDF_WORKITEM)MadEvtDmaWorkItem);
		NtStatus = WdfWorkItemCreate(&WiConfig, &WiObjAttrs, &pFdoData->hDmaWI);
		}
	else //We need a Bufrd-Io work item
	    {
		WDF_WORKITEM_CONFIG_INIT(&WiConfig, (PFN_WDF_WORKITEM)MadEvtBufrdIoWorkItem);
		NtStatus = WdfWorkItemCreate(&WiConfig, &WiObjAttrs, &pFdoData->hBufrdIoWI);
	    }

	if (!NT_SUCCESS(NtStatus)) //We can't continue w/out an I/O work item
	    {
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadDevWdfCreateInt:WdfWorkItemCreate (BufrdIo / DMA) returned...x%X\n",
                    NtStatus);
		return NtStatus;
	    }

	if (pFdoData->pDriverData->bDmaEnabled)
	    {
		pIsrDpcWiData = MadGetIsrDpcWiContext(pFdoData->hDmaWI);
		pIsrDpcWiData->IsrDpcWiTag = eDmaWI;
	    }
	else
	    {
		pIsrDpcWiData = MadGetIsrDpcWiContext(pFdoData->hBufrdIoWI);
		pIsrDpcWiData->IsrDpcWiTag = eBufrdIoWI;
	    }

    WDF_WORKITEM_CONFIG_INIT(&WiConfig, (PFN_WDF_WORKITEM)MadEvtErrorWorkItem);
    NtStatus = WdfWorkItemCreate(&WiConfig, &WiObjAttrs, &pFdoData->hDevErrorWI); 
	if (NT_SUCCESS(NtStatus))
	    {
		pIsrDpcWiData = MadGetIsrDpcWiContext(pFdoData->hDevErrorWI);
		pIsrDpcWiData->IsrDpcWiTag = eDevErrWI;
	    }
	else  //Not a show-stopper ... but please no device failures
	    { //Brackets around null stmt in the case of Free_build & No WPP_TRACING(KdPrint)
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		            "MadDevWdfCreateInt:WdfWorkItemCreate (DevError)...returns x%X, continuing\n",
                    NtStatus);
	    }

    return STATUS_SUCCESS;
}

/************************************************************************//**
 * MadEvtIntEnable
 *
 * DESCRIPTION:
 *    This callback function is invoked by the framework to enable interrupts
 *    at power-up. Called at device irql w/ the int spinlock acquired
 *    
 * PARAMETERS: 
 *     @param[in]  hInterrupt  handle to the interrupt object 
 *     @param[in]  hDevice     handle to our device 
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadEvtIntEnable(IN WDFINTERRUPT hInterrupt, IN WDFDEVICE hDevice)

{  
static ULONG64 Zero64 = 0;
PFDO_DATA pFdoData = MadDeviceFdoGetData(hDevice);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtIntEnable enter...pFdoData=%p SerialNo=%d\n", 
                pFdoData, pFdoData->SerialNo);
    ASSERT(hInterrupt == pFdoData->hInterrupt);
    UNREFERENCED_PARAMETER(hInterrupt);
    ASSERT(KeGetCurrentIrql() == pFdoData->Irql);

    WRITE_REGISTER_ULONG(&pFdoData->pMadRegs->IntEnable, MAD_ALL_INTS_ENABLED_BITS);
	WRITE_REGISTER_BUFFER_UCHAR((PUCHAR)&pFdoData->pMadRegs->DmaChainItem0.HostAddr,
		                        (PUCHAR)&Zero64, sizeof(ULONG64));

    return STATUS_SUCCESS;
}

/**************************************************************************//**
 * MadEvtIntDisable
 *
 * DESCRIPTION:
 *    This callback function is invoked by the framework to disable interrupts
 *    at power-down. Called at device irql w/ the int spinlock acquired
 *    
 * PARAMETERS: 
 *     @param[in]  hInterrupt  handle to the interrupt object 
 *     @param[in]  hDevice     handle to our device 
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 *****************************************************************************/
NTSTATUS MadEvtIntDisable(IN WDFINTERRUPT hInterrupt, IN WDFDEVICE hDevice)
{
PFDO_DATA pFdoData = MadDeviceFdoGetData(hDevice);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtIntDisable enter...pFdoData=%p SerialNo=%d\n",
                pFdoData, pFdoData->SerialNo);
    ASSERT(hInterrupt == pFdoData->hInterrupt);
    UNREFERENCED_PARAMETER(hInterrupt);
    ASSERT(KeGetCurrentIrql() == pFdoData->Irql);
 
    WRITE_REGISTER_ULONG(&pFdoData->pMadRegs->IntEnable, MAD_ALL_INTS_DISABLED);

    return STATUS_SUCCESS;
}

/**************************************************************************//**
 * MadEvtReleaseHardware
 *
 * DESCRIPTION:
 *      This function is called by the framework whenever the PnP manager is 
 *      revoking ownership of our resources. This may be in response to either
 *      IRP_MN_STOP_DEVICE or IRP_MN_REMOVE_DEVICE.  This callback is invoked 
 *      before passing down the IRP to the lower driver.    
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice      handle to our device 
 *     @param[in]  hResrcsXlatd see the description above
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 *****************************************************************************/
NTSTATUS MadEvtReleaseHardware(IN WDFDEVICE hDevice, IN WDFCMRESLIST hResrcsXlatd)
{
PFDO_DATA pFdoData = MadDeviceFdoGetData(hDevice);;

    UNREFERENCED_PARAMETER(hResrcsXlatd);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtReleaseHardware enter...SerialNo=%d\n", 
                pFdoData->SerialNo);

    PAGED_CODE();

    // Unmap all virtual memory chunks that we mapped in PrepareHardware.
    // Disconnecting from the interrupt will be done automatically by the framework.
    //Make sure we got our device mapped before unmapping
    if (pFdoData->pMadPioWrite != NULL)
	    MmUnmapIoSpace(pFdoData->pMadPioWrite, MAD_MAPD_WRITE_SIZE);

    if (pFdoData->pMadPioRead != NULL)
	    MmUnmapIoSpace(pFdoData->pMadPioRead, MAD_MAPD_READ_SIZE);

    if (pFdoData->pMadRegs != NULL)
        MmUnmapIoSpace(pFdoData->pMadRegs,  MAD_REGISTER_BLOCK_SIZE);

    return STATUS_SUCCESS;
}

/************************************************************************//**
 * MadEvtContextCleanup
 *
 * DESCRIPTION:
 *    This EvtDeviceContextCleanup event callback function must perform any 
 *    operations that are necessary before the specified device is removed. 
 *    The framework calls the driver's EvtDeviceContextCleanup callback when
 *    the device is deleted in response to IRP_MN_REMOVE_DEVICE request.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice     handle to our device 
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
VOID MadEvtContextCleanup(IN WDFDEVICE hDevice)
{
PFDO_DATA   pFdoData  = MadDeviceFdoGetData(hDevice);

PAGED_CODE();

    UNREFERENCED_PARAMETER(pFdoData);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtContextCleanup enter... SerialNo=%d\n",
                pFdoData->SerialNo);

    return;
}

/**************************************************************************//**
 * MadEvtFileCreate
 * DESCRIPTION:
 *      The framework calls a driver's EvtDeviceFileCreate callback when
 *      the framework receives an IRP_MJ_CREATE request. The system sends this
 *      request when a user application opens the device to perform an I/O
 *      operation, such as reading or writing to a device. This callback is 
 *      called in the context of the thread that created the IRP_MJ_CREATE irp.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice   handle to our device 
 *     @param[in]  hRequest  handle to this I/O request
 *     @param[in]  hFileObj  handle to the fileobject representing the open.
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 *****************************************************************************/
VOID MadEvtFileCreate(IN WDFDEVICE hDevice, IN WDFREQUEST hRequest,
                      IN WDFFILEOBJECT hFileObj)
{
PFDO_DATA pFdoData = MadDeviceFdoGetData(hDevice);

    UNREFERENCED_PARAMETER(hFileObj);
	UNREFERENCED_PARAMETER(pFdoData);
    PAGED_CODE ();

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtFileCreate... pFdoData=%p SerialNo=%d\n",
                pFdoData, pFdoData->SerialNo);

    WdfRequestComplete(hRequest, STATUS_SUCCESS);

    return;
}

/************************************************************************//**
 * MadEvtFileClose
 *
 * DESCRIPTION:
 *      EvtFileClose is called when all the handles represented by the 
 *      FileObject is closed and all the references to FileObject is removed.
 *      This callback may get called in an arbitrary thread context instead of
 *      the thread that called CloseHandle. If we want to delete any per
 *      FileObject context that must be done in the context of the user thread
 *      that made the Create call, we do that in the EvtDeviceCleanp callback.
 *    
 * PARAMETERS: 
 *     @param[in]  hFileObj  handle to the fileobject representing the open.
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
VOID MadEvtFileClose(IN WDFFILEOBJECT hFileObj)
{
PFDO_DATA pFdoData = MadDeviceFdoGetData(WdfFileObjectGetDevice(hFileObj));

    PAGED_CODE();
	UNREFERENCED_PARAMETER(pFdoData);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtFileClose... pFdoData=%p SerialNo=%d\n",
                pFdoData, pFdoData->SerialNo);

    return;
}

/************************************************************************//**
 * MadDev_ProcessConfig
 *
 * DESCRIPTION:
 *    This function reads the device configuration space parameters from
 *    the bus driver; verifies id; processes & saves the data.
 *    
 * PARAMETERS: 
 *     @param[in]  pLowrDev    pointer to the lower device object.
 *     @param[in]  pDevObj     pointer  handle to our device object
 *     @param[in]  pFdoData    pointer to the framework device extension
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadDev_ProcessConfig(PDEVICE_OBJECT pLowrDev, 
                              PDEVICE_OBJECT pDevObj, PFDO_DATA pFdoData)

{
register LONG            j;
NTSTATUS                 NtStatus;
USHORT CnfgVendorId;
USHORT CnfgDeviceId;
USHORT CnfgSubVndrId;
USHORT CnfgSubDevId;
MADBUS_DEVICE_CONFIG_DATA  MadDevCnfg;

    RtlFillMemory(&MadDevCnfg, sizeof(MADBUS_DEVICE_CONFIG_DATA), 0x00);
    NtStatus = MadDev_GetCnfgDataFromPdo(pLowrDev, pDevObj, &MadDevCnfg);
    if (!NT_SUCCESS(NtStatus))
        return NtStatus;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDevice_ProcessConfig...pLowrDev=%p pDevObj=%p\n",
				pLowrDev, pDevObj); 

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MADDEVICE config data: VendorID=x%X DeviceID=x%X InterruptLine=x%X\n",
                MadDevCnfg.LegacyPci.VendorID, MadDevCnfg.LegacyPci.DeviceID,
	            MadDevCnfg.LegacyPci.u.type1.InterruptLine); 

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MADDEVICE config data: BARs [0..5] = {x%X x%X x%X x%X x%X x%X}\n",
                MadDevCnfg.LegacyPci.u.type0.BaseAddresses[0], 
                MadDevCnfg.LegacyPci.u.type0.BaseAddresses[1],
                MadDevCnfg.LegacyPci.u.type0.BaseAddresses[2], 
                MadDevCnfg.LegacyPci.u.type0.BaseAddresses[3],
	            MadDevCnfg.LegacyPci.u.type0.BaseAddresses[4],
                MadDevCnfg.LegacyPci.u.type0.BaseAddresses[5]); 
	
//*** Verify our vendor & Device-ID or it's  not our device 
    CnfgVendorId  = MadDevCnfg.LegacyPci.VendorID;
    CnfgDeviceId  = MadDevCnfg.LegacyPci.DeviceID;
    CnfgSubVndrId = MadDevCnfg.LegacyPci.u.type0.SubVendorID;
    CnfgSubDevId  = MadDevCnfg.LegacyPci.u.type0.SubSystemID; //Odd variable name in PCI_COMN_CNFG

    if ((CnfgVendorId != MAD_VENDOR_ID)  || 
        (CnfgDeviceId != MAD_DEVICE_ID_GENERIC)) 
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "Unknown Vendor/Device-ID in the MadBus configuration space!\n"); 
        return STATUS_INVALID_PARAMETER;
    	}

//* Gather the MadBus config-space data 
    pFdoData->SerialNo          = MadDevCnfg.VndrSpecData.BusSlotNum;
	pFdoData->Irql              = MadDevCnfg.VndrSpecData.Irql;
    pFdoData->CurrDevPowerState = MadDevCnfg.VndrSpecData.DevPowerState;

	//Save the BARs
    for (j = 0; j < 6; j++)
        pFdoData->BARs[j] = MadDevCnfg.LegacyPci.u.type0.BaseAddresses[j];   
         	    
	//Save the whole config-space 
    RtlCopyMemory(&pFdoData->DevCnfgData, &MadDevCnfg, sizeof(MADBUS_DEVICE_CONFIG_DATA));

	#ifdef _MAD_SIMULATION_MODE_ //We need the pointer to the simulator's interrupt thread parm area
	pFdoData->pMadSimIntParms = 
    (PMAD_SIMULATION_INT_PARMS)MadDevCnfg.VndrSpecData.pMadSimIntParms;

    #ifdef MAD_KERNEL_WONT_CREATE_DMA_ADAPTER
    //We need the pointer to the table of Wdf Dma replacement function pointers provided by the simulator
	pFdoData->pMadSimDmaFunxns = 
    (PMADSIM_KMDF_DMA_FUNXNS)MadDevCnfg.VndrSpecData.pMadSimDmaFunxns;
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadDev_ProcessConfig... Sernum=%d Irql=%d BARs[0..5] = {x%X x%X x%X x%X x%X x%X} pSimDmaFunxns=%p\n",
                pFdoData->SerialNo, pFdoData->Irql,
                pFdoData->BARs[0], pFdoData->BARs[1], pFdoData->BARs[2],
                pFdoData->BARs[3], pFdoData->BARs[4], pFdoData->BARs[5],
                pFdoData->pMadSimDmaFunxns);
    ASSERTMSG("SimDmaFunxnsPntr invalid!", (pFdoData->pMadSimDmaFunxns != NULL));
    ASSERTMSG("SimDmaEnablerCreatePntr invalid!", 
              (pFdoData->pMadSimDmaFunxns->pDmaEnablerCreate != NULL));
    #endif // MAD_KERNEL_WONT_CREATE_DMA_ADAPTER
    #endif //_MAD_SIMULATION_MODE_

    return STATUS_SUCCESS;
}

/**************************************************************************//**
 * MadDev_GetCnfgDataFromPdo
 *
 * DESCRIPTION:
 *    This function builds the irp to read configuration space from the bus
 *    driver. It is a helper function 
 *    
 * PARAMETERS: 
 *     @param[in]  pPrntDev    pointer to the parent device object.
 *     @param[in]  pDevObj     pointer to our device object
 *     @param[in]  pFdoData    pointer to the framework device extension
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 *****************************************************************************/
NTSTATUS MadDev_GetCnfgDataFromPdo(PDEVICE_OBJECT pPrntDev, PDEVICE_OBJECT pDevObj,
                                   PMADBUS_DEVICE_CONFIG_DATA pMadDevCnfg)

{
NTSTATUS                 NtStatus;
IO_STACK_LOCATION        IoStackLoc;

    //TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
    //"MadDevice_GetCnfgFromBus pPrntDev=%p, pDevObj=%p\n", pPrntDev, pDevObj)); 

//*** Setup read-config IRP Parameters
    IoStackLoc.MajorFunction                         = IRP_MJ_PNP;
    IoStackLoc.MinorFunction                         = IRP_MN_READ_CONFIG;
    IoStackLoc.Parameters.ReadWriteConfig.WhichSpace = MADBUS_WHICHSPACE_CONFIG; 
    IoStackLoc.Parameters.ReadWriteConfig.Buffer     = (PVOID)pMadDevCnfg;  
    IoStackLoc.Parameters.ReadWriteConfig.Offset     = 0; 
    IoStackLoc.Parameters.ReadWriteConfig.Length     = sizeof(MADBUS_DEVICE_CONFIG_DATA); 

//*** Send IRP and wait for results
    NtStatus = MadDev_AllocIrpSend2Parent((PVOID)pPrntDev, 
                                          (PVOID)pDevObj, (PVOID)&IoStackLoc); 
    return NtStatus;
}

#if 0
/************************************************************************//**
 * MadDev_AllocIrpSend2Parent
 *
 * DESCRIPTION:
 *    This function builds and sends an irp to the lower device object.
 *    It waits for the IRP to complete
 *    
 * PARAMETERS: 
 *     @param[in]  pPrntDev    pointer to the parent device object.
 *     @param[in]  pDevObj     pointer to our device object
 *     @param[in]  pIoStackLoc pointer to the current stack location
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadDev_AllocIrpSend2Parent(PDEVICE_OBJECT pPrntDev,
                                    PDEVICE_OBJECT pDevObj, 
                                    PIO_STACK_LOCATION pIoStackLoc)
{
NTSTATUS           NtStatus;
KEVENT             Kevent;
PIRP               pIRP;
PIO_STACK_LOCATION pIrpIoStackLoc;
CCHAR              StackSize       = pDevObj->StackSize;

    //TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
    //"MadDevice_AllocIrpSend2Parent pPrntDev=%p, pDevObj=%p\n", pPrntDev, pDevObj)); 

//*** Build the IRP and pass it down to the parent (PDO)
    pIRP = IoAllocateIrp(StackSize, FALSE);
    if (pIRP == NULL)  // LOG it & return false
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadDev_AllocIrpSend2Parent...allocate IRP failed!\n"); 
        return STATUS_NO_MEMORY;
        }

    KeInitializeEvent(&Kevent, NotificationEvent, FALSE);
    #pragma warning(suppress: 28023)
    IoSetCompletionRoutine(pIRP, MadDev_CnfgCmpltnRtn, &Kevent, TRUE, TRUE, TRUE);

//*** Setup IRP Parameters
	pIRP->IoStatus.Status = STATUS_UNSUCCESSFUL; //Throw Driver-verifier a cookie
    pIrpIoStackLoc = IoGetNextIrpStackLocation(pIRP);  
    pIrpIoStackLoc->MajorFunction = pIoStackLoc->MajorFunction;
    pIrpIoStackLoc->MinorFunction = pIoStackLoc->MinorFunction;
    RtlCopyMemory(&pIrpIoStackLoc->Parameters,
                  &pIoStackLoc->Parameters, sizeof(pIoStackLoc->Parameters)); 

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadDev_AllocIrpSend2Parent... Sending PnpIrp Irp_Major:Minor=x%X:%X\n",
                pIrpIoStackLoc->MajorFunction, pIrpIoStackLoc->MinorFunction);

//*** Send the IRP and wait for results
    NtStatus = IoCallDriver(pPrntDev, pIRP);
    if (NtStatus == STATUS_PENDING)
        {
        KeWaitForSingleObject(&Kevent, Executive, KernelMode, FALSE, NULL);
        NtStatus = pIRP->IoStatus.Status;
        }
    IoFreeIrp(pIRP); 

    if (!NT_SUCCESS(NtStatus)) // LOG it & return 
	    { //Brackets around null stmt in the case of Free_build & No WPP_TRACING(KdPrint)
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		            "MadDev_AllocIrpSend2Parent...SendIrpSync failed! ntstatus=x%X\n", 
                    NtStatus); 
	    }

    return NtStatus;
}

/************************************************************************//**
 * MadDev_CnfgCmpltnRtn
 *
 * DESCRIPTION:
 *    This function is the irp completion routine for the read configuration
 *    irp
 *    
 * PARAMETERS: 
 *     @param[in]  pDevObj     pointer to our device object
 *     @param[in]  pIRP        pointer to the completing irp
 *     @param[in]  pContext    pointer to the event object to be waited on 
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadDev_CnfgCmpltnRtn(PDEVICE_OBJECT pDevObj, PIRP pIRP, PVOID pContext)

{
    UNREFERENCED_PARAMETER(pDevObj);
    UNREFERENCED_PARAMETER(pIRP);

    KeSetEvent((PKEVENT)pContext, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED; // Keep this IRP
}
#endif //0

/************************************************************************//**
 * MadDev_ReadRegParms
 *
 * DESCRIPTION:
 *    This function is the irp completion routine for the read configuration
 *    irp
 *    
 * PARAMETERS: 
 *     @param[in]   pDriverObj    pointer to our driver object
 *     @param[in]   pParmPath     pointer to the registry path
 *     @param[out]  pbDmaEnabled pointer to the boolean DmaEnabled 
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadDev_ReadRegParms(IN PDRIVER_OBJECT pDriverObj, IN PUNICODE_STRING pParmPath,
							 OUT PBOOLEAN pbDmaEnabled)

{
RTL_QUERY_REGISTRY_TABLE RegParmTable[2];  // Parameter table -- parameters key
NTSTATUS NtStatus = STATUS_SUCCESS;   // assume success

// We set these values to their defaults in case there are any failures
HANDLE  hKey;
OBJECT_ATTRIBUTES  ObjAttrs;

    UNREFERENCED_PARAMETER(pDriverObj);
    ASSERT(pParmPath != NULL);
 
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDev_ReadRegParms...RegPath = %S\n", pParmPath->Buffer);

// We set these values to their defaults in case there are any failures
    *pbDmaEnabled = FALSE;

	InitializeObjectAttributes(&ObjAttrs, pParmPath,
                               OBJ_CASE_INSENSITIVE, NULL, (PSECURITY_DESCRIPTOR)NULL);
    NtStatus = ZwOpenKey(&hKey, KEY_QUERY_VALUE, &ObjAttrs);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDev_ReadRegParms:ZwOpenKey...NtStatus = 0x%x\n", NtStatus);
	if (!NT_SUCCESS(NtStatus))
		return NtStatus;

    //Old-fashioned non-WDF registry read of multiple values by setting up a table
    RtlZeroMemory(&RegParmTable, sizeof(RegParmTable)); // mandatory
    RegParmTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    RegParmTable[0].Name          = REG_DMA_ENABLED;
    RegParmTable[0].EntryContext  = pbDmaEnabled;
    RegParmTable[0].DefaultType   = REG_DWORD;
    RegParmTable[0].DefaultData   = pbDmaEnabled;
    RegParmTable[0].DefaultLength = sizeof(ULONG);

    NtStatus = 
    RtlQueryRegistryValues((RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL | RTL_REGISTRY_HANDLE),
                            /*pParmPath->Buffer*/(PCWSTR)hKey, RegParmTable, NULL, NULL);
    if (!NT_SUCCESS(NtStatus))
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadDev_ReadRegParms:RtlQueryRegistryValues failed...NtStatus=x%X\n",
                    NtStatus);
    else
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			         "MadDev_ReadRegParms...DmaEnabled = %d\n", *pbDmaEnabled);
 
	NTSTATUS NtStatZw = ZwClose(hKey);
	ASSERT(NtStatZw == STATUS_SUCCESS);
	UNREFERENCED_PARAMETER(NtStatZw);

    return NtStatus;
}

/////////////////////////////////////////////////////////////////////////////////////////////
#ifdef TTIOD2PNP
NTSTATUS MadEvtAddDeviceResrcReqs(IN WDFDEVICE hDevice, IN WDFIORESREQLIST hIoResrcReqList)
{
PFDO_DATA pFdoData = MadDeviceFdoGetData(hDevice);
ULONG ResrcCount = WdfIoResourceRequirementsListGetCount(hIoResrcReqList);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtAddDeviceResrcReqs pFdoData=%p, SerialNo=%d\n, ResrcCount=%d\n", 
                pFdoData, pFdoData->SerialNo, ResrcCount));
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtAddDeviceResrcReqs...don't add - just be glad  :)\n"); 

    return STATUS_SUCCESS;
}

NTSTATUS MadEvtRemoveResrcReqs(IN WDFDEVICE hDevice, IN WDFIORESREQLIST hIoResrcReqList)
{
PFDO_DATA pFdoData = MadDeviceFdoGetData(hDevice);
ULONG ResrcCount = WdfIoResourceRequirementsListGetCount(hIoResrcReqList);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtRemoveResrcReqs pFdoData=%p, SerialNo=%d\n, ResrcCount=%d\n", 
                pFdoData, pFdoData->SerialNo, ResrcCount));
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtRemoveResrcReqs...don't remove - just approve :)\n"); 

    return STATUS_SUCCESS;
}
#endif
