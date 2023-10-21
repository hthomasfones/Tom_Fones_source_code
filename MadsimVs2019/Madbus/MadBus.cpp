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
/*  Exe file ID  : MadBus.sys                                                  */
/*                                                                             */
/*  Module  NAME : MadBus.cpp                                                  */
/*                                                                             */
/*  DESCRIPTION  : Main source module for the simulator-bus driver             */
/*                 DriverEntry, UnloadDriver, AddDevice, PnP ioctl procesing   */
/*                 Derived from WDK-Toaster\bus\bus.c                          */
/*                                                                             */
/*******************************************************************************/

#define _MADBUS_MAIN
#include "MadBus.h"

#ifdef WPP_TRACING
    //#pragma message("WPP_TRACING defined")
    #include "trace.h"
    #include "MadBus.tmh"
#endif

//Globals
ULONG                        gDevIRQL          = MAD_DFLT_DEVICE_IRQL;
ULONG                        gIdtBaseDX        = MAD_DFLT_IDT_BASE_INDX;
BOOLEAN                      gbAffinityOn      = MAD_DFLT_AFFINITY_ON;
//WDFDEVICE                    ghDevFDO[eMaxBusType];
//
DRIVER_DATA                  gDriverData;
PDRIVER_OBJECT               gpDriverObj;

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, MadBusEvtDeviceAdd)
#pragma alloc_text (PAGE, MadBusEvtIoDeviceControl)
#pragma alloc_text (PAGE, MadBus_PlugInDevice)
#pragma alloc_text (PAGE, MadBus_UnPlugDevice)
#pragma alloc_text (PAGE, MadBus_EjectDevice)
#endif

/************************************************************************//**
 * DriverEntry
 *
 * DESCRIPTION:
 *     DriverEntry initializes the driver and is the first routine called by 
 *     the system after the driver is loaded. DriverEntry specifies the other
 *     entry points in the function driver, such as AddDevice and Unload.
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
NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObj, IN PUNICODE_STRING pRegistryPath)

{
static WCHAR ValueName[] = CONFIG_DATA_VALUE_NAME;
WDF_DRIVER_CONFIG     DriverConfig;
//WDF_OBJECT_ATTRIBUTES DdoAttrs; //driver object
NTSTATUS              NtStatus;
WDFDRIVER             hDriver;
UNICODE_STRING        ParmSubkey;
UNICODE_STRING        ParmSubkeyPath;
UNICODE_STRING        ConfigSubkey;
UNICODE_STRING        ConfigSubkeyPath;
ULONG                 RegDataXtent = 0;
PDRIVER_DATA          pDriverData;

#ifdef WPP_TRACING
	WPP_INIT_TRACING(pDriverObj, pRegistryPath);
#endif

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBus.sys: bus driver for the Model-Abstract-Demo-Device - build timestamp ... %s|%s\n",
                BUILD_TIME, BUILD_DATE);

    // Initialize driver config to control the attributes that are global to the driver. 
    // Note that framework by default provides a driver unload routine. 
    // If you create any resources in the DriverEntry and want to be cleaned in driver unload,
    // you can override that by specifing one in the Config structure.
    WDF_DRIVER_CONFIG_INIT(&DriverConfig, MadBusEvtDeviceAdd);
    DriverConfig.EvtDriverUnload = MadBusEvtDriverUnload;
    DriverConfig.DriverPoolTag   = MADBUS_POOL_TAG;

    // Initialize attributes structure to specify size and accessor function for storing device context.
    //WDF_OBJECT_ATTRIBUTES_INIT(&DdoAttrs);
   // WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&DdoAttrs, DRIVER_DATA);
    
    // Create a framework driver object to represent our driver.
     NtStatus = WdfDriverCreate(pDriverObj, pRegistryPath, 
                                WDF_NO_OBJECT_ATTRIBUTES,
                                &DriverConfig, &hDriver);
    if (!NT_SUCCESS(NtStatus)) 
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "WdfDriverCreate failed with NtStatus x%x\n", NtStatus);
		MadWriteEventLogMesg(pDriverObj, MADBUS_DRIVER_LOAD_ERROR, 
                             1, sizeof(NTSTATUS), (PWSTR)&NtStatus); 
        return NtStatus;
        } 

	gDriverData.RegistryPath.Buffer = NULL;
    NtStatus = MadSaveUnicodeString(&gDriverData.RegistryPath, pRegistryPath);
    ASSERT(NtStatus == STATUS_SUCCESS);
 
    gDriverData.pDriverObj = pDriverObj;
    gpDriverObj = pDriverObj;
    pDriverData = &gDriverData; //MadBusDriverGetContextData(hDriver);
    MadBus_InitDriverContextAndGlobals(pDriverData, hDriver,
                                       pDriverObj, pRegistryPath);
    MadBus_InitConfigSpaceDflts(&pDriverData->MadDevDfltCnfg);

    //One-shot use to create a registry subkey:Value with config data to be edited
    //NtStatus = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, gRegistryPath.Buffer, 
    //                                 ValueName, REG_NONE, &gMadDevDfltCnfg, sizeof(MadBus_DEVICE_CONFIG_DATA));
    //ASSERT(NtStatus == STATUS_SUCCESS);

    //Fetch globals parameters from the registry
	ConfigSubkeyPath.Buffer = NULL;
    RtlInitUnicodeString(&ConfigSubkey, CONFIG_DATA_SUBKEY);

    NtStatus = 
    MadInitSubkeyPath(pDriverObj, pRegistryPath, &ConfigSubkey, &ConfigSubkeyPath);
    ASSERT(NtStatus == STATUS_SUCCESS);

    NtStatus = 
    MadBusReadRegConfig((WDFDEVICE)hDriver, &ConfigSubkeyPath, &pDriverData->MadDevDfltCnfg);
	if (ConfigSubkeyPath.Buffer != NULL)
	    ExFreePool(ConfigSubkeyPath.Buffer);

	ParmSubkeyPath.Buffer = NULL;
    RtlInitUnicodeString(&ParmSubkey, PARMS_SUBKEY);
    NtStatus = 
    MadInitSubkeyPath(pDriverObj, pRegistryPath, &ParmSubkey, &ParmSubkeyPath);
    ASSERT(NtStatus == STATUS_SUCCESS);

    NtStatus = MadBusReadRegParms(pDriverObj, &ParmSubkeyPath,
                                  &gDevIRQL, &gIdtBaseDX, &gbAffinityOn,
								  &pDriverData->NumFilters, 
                                  &pDriverData->NumAllocDevices, &RegDataXtent);
	if (ParmSubkeyPath.Buffer != NULL)
	    ExFreePool(ParmSubkeyPath.Buffer);

	//Update the device data size if the registry value passes this filter
	// ... else report the default unchanged
	if ((RegDataXtent % MAD_SECTOR_SIZE) == 0)
		if (RegDataXtent >= MAD_MIN_DATA_EXTENT)
			if (RegDataXtent <= MAD_MAX_DATA_EXTENT)
                pDriverData->MadDataXtent = RegDataXtent;
    //
	if (pDriverData->MadDataXtent == MAD_DEFAULT_DATA_EXTENT)
	    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		            "MadBus:WdfDriverCreate gMadDataXtent default unchanged\n");

	//Get a big defined chunk of memory for the entire set of devices
    pDriverData->pDeviceSetBase = 
    MadBus_AllocDeviceSet(pDriverObj, &pDriverData->MadDevDfltCnfg,
                          &pDriverData->NumAllocDevices);
	if (pDriverData->pDeviceSetBase == NULL)
	    {
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadBus:WdfDriverCreate failed (insufficient memory)\n");

		MadWriteEventLogMesg(pDriverObj, MADBUS_DRIVER_LOAD_ERROR, 1, 
                             sizeof(NTSTATUS), (PWSTR)&NtStatus); //Minimal payload
		return STATUS_NO_MEMORY;
	    }

	MadWriteEventLogMesg(pDriverObj, MADBUS_DRIVER_LOAD, 0, 0, NULL); //Hard-coded message text. No payload

    return STATUS_SUCCESS;  
}

/************************************************************************//**
 * MadBusEvtDriverUnload
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
VOID MadBusEvtDriverUnload(IN WDFDRIVER  hDriver)

{
    PDRIVER_DATA  pDriverData = &gDriverData; // MadBusDriverGetContextData(hDriver);
    PDRIVER_OBJECT pDriverObj = pDriverData->pDriverObj;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBusEvtDriverUnload... the root-enumerated Madbus driver is exiting *!*\n");
    ASSERT(hDriver == gDriverData.hDriver);

#ifdef WPP_TRACING
	PDRIVER_OBJECT pDriverObj = WdfDriverWdmGetDriverObject(hDriver);
  	WPP_CLEANUP(pDriverObj);
#endif

	if (pDriverData->pDeviceSetBase != NULL)
        MmFreeContiguousMemory(pDriverData->pDeviceSetBase);

	if (pDriverData->RegistryPath.Buffer != NULL)
		ExFreePool(pDriverData->RegistryPath.Buffer);
     
    pDriverData->NumBusFDOs = 0;
	MadWriteEventLogMesg(pDriverObj, MADBUS_DRIVER_LOAD, 0, 0, NULL); //Hard-coded message text. No payload

    return;
}

/************************************************************************//**
 * MadBus_InitDriverContextAndGlobals
 *
 * DESCRIPTION:
 *    This function initializes the driver-wide context data.
 *    This data replaces/minimizes the need for global data
 *
 * PARAMETERS:
 *    @param[in]  pDriverData    pointer to the driver context data
 *    @param[in]  hDriver       handle to the driver object
 *    @param[in]  pDriverObj    driver object pointer passed to DriverEntry
 *    @param[in]  pRegistryPath registry path pointer passed to DriverEntry
 *
 * RETURNS:
 *    @return      void        nothing returned
 *
 ***************************************************************************/
void MadBus_InitDriverContextAndGlobals(PDRIVER_DATA pDriverData,
                                        WDFDRIVER hDriver, 
                                        PDRIVER_OBJECT pDriverObj,
                                        PUNICODE_STRING pRegistryPath)
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    RtlFillMemory(pDriverData, sizeof(DRIVER_DATA), 0x00);
    pDriverData->hDriver = hDriver;
    pDriverData->pDriverObj = pDriverObj;

    pDriverData->RegistryPath.Buffer = NULL;
    NtStatus = MadSaveUnicodeString(&pDriverData->RegistryPath, pRegistryPath);
    ASSERT(NtStatus == STATUS_SUCCESS);
    //
    pDriverData->DevIRQL = MAD_DFLT_DEVICE_IRQL;
    pDriverData->IdtBaseDX = MAD_DFLT_IDT_BASE_INDX;
    pDriverData->bAffinityOn = MAD_DFLT_AFFINITY_ON;
    pDriverData->NumFilters = MAD_DFLT_NUM_FILTERS;
    pDriverData->MadDataXtent = MAD_DEFAULT_DATA_EXTENT;
    pDriverData->NumBusFDOs = 0; 

#ifdef _MAD_SIMULATION_MODE_ //MAD_KERNEL_WONT_CREATE_DMA_ADAPTER 
    //Populate a table of function pointers for KMDF-DMA simulation functions to be passed to the device driver 
    MadSimInitDmaFunxnTable(&pDriverData->MadSimDmaFunxns);

    //Populate a table of Storport simulation function pointers   
    MadSimInitStorPortFunxnTable(&pDriverData->MadSimSpIoFunxns);
    //gDriverData.MadSimSpIoFunxns.pMadsimSpGetDeviceBase = MadSimGetDeviceBase;
#endif   

    //Initialize all of the interface_query id GUIDs
    gDriverData.QueryIntfGuids[eNullIntfQ]       = GUID_NULL;
    gDriverData.QueryIntfGuids[eBusIntfQ]       = GUID_BUS_INTERFACE_STANDARD;
    gDriverData.QueryIntfGuids[ePciBusIntfQ]    = GUID_PCI_BUS_INTERFACE_STANDARD;
    gDriverData.QueryIntfGuids[ePciBusIntfQ2]   = GUID_PCI_BUS_INTERFACE_STANDARD2;
    gDriverData.QueryIntfGuids[eArbtrIntfQ]     = GUID_ARBITER_INTERFACE_STANDARD;
    gDriverData.QueryIntfGuids[eXlateIntfQ]     = GUID_TRANSLATOR_INTERFACE_STANDARD;
    gDriverData.QueryIntfGuids[eAcpiIntfQ]      = GUID_ACPI_INTERFACE_STANDARD;
    gDriverData.QueryIntfGuids[eIntRouteIntfQ]  = GUID_INT_ROUTE_INTERFACE_STANDARD;
    gDriverData.QueryIntfGuids[ePcmciaBusIntfQ] = GUID_PCMCIA_BUS_INTERFACE_STANDARD;
    gDriverData.QueryIntfGuids[eAcpiRegsIntfQ]  = GUID_ACPI_REGS_INTERFACE_STANDARD;
    gDriverData.QueryIntfGuids[eLegacyDevIntfQ] = GUID_LEGACY_DEVICE_DETECTION_STANDARD;
    gDriverData.QueryIntfGuids[ePciDevIntfQ]    = GUID_PCI_DEVICE_PRESENT_INTERFACE;
    gDriverData.QueryIntfGuids[eMfEnumIntfQ]    = GUID_MF_ENUMERATION_INTERFACE;
    gDriverData.QueryIntfGuids[eRenumSelfIntfQ] = GUID_REENUMERATE_SELF_INTERFACE_STANDARD;
    gDriverData.QueryIntfGuids[ePnpLocIntfQ]    = GUID_PNP_LOCATION_INTERFACE;
    gDriverData.QueryIntfGuids[eXAddrIntfQ]     = GUID_PNP_EXTENDED_ADDRESS_INTERFACE;
    gDriverData.QueryIntfGuids[eIommuBusIntfQ]  = GUID_IOMMU_BUS_INTERFACE;

    gDriverData.NumAllocDevices = MAD_MAX_DEVICES;
    gDriverData.MadDataXtent = MAD_DEFAULT_DATA_EXTENT;
    return;
}

/************************************************************************//**
 * Ras_Init_ConfigSpaceDflts
 *
 * DESCRIPTION:
 *    This function initializes the device configuration space
 *
 * PARAMETERS:
 *    @param[in]  pMadDevCnfg    pointer to the configuration space data
 *
 * RETURNS:
 *    @return      void        nothing returned
 *
 ***************************************************************************/
void MadBus_InitConfigSpaceDflts(PMADBUS_DEVICE_CONFIG_DATA pMadDevCnfg)

{
    RtlFillMemory((PVOID)pMadDevCnfg, sizeof(MADBUS_DEVICE_CONFIG_DATA), 0x00);

    pMadDevCnfg->LegacyPci.VendorID = MAD_VENDOR_ID;
    pMadDevCnfg->LegacyPci.DeviceID = MAD_DEVICE_ID_GENERIC;
    //USHORT  Command;                    
    //USHORT  Status;
    //UCHAR   RevisionID;                 
    //UCHAR   ProgIf;                     
    //UCHAR   SubClass;                   
    //UCHAR   BaseClass;                  
    //UCHAR   CacheLineSize;              
    //UCHAR   LatencyTimer;               
    //UCHAR   HeaderType;                 
    //UCHAR   BIST;                       

    pMadDevCnfg->LegacyPci.u.type0.BaseAddresses[0] = 0xFFF00000;
    pMadDevCnfg->LegacyPci.u.type0.BaseAddresses[1] = 0xFFF10000;
    pMadDevCnfg->LegacyPci.u.type0.BaseAddresses[2] = 0xFFF20000;
    pMadDevCnfg->LegacyPci.u.type0.BaseAddresses[3] = 0xFFF30000;
    pMadDevCnfg->LegacyPci.u.type0.BaseAddresses[4] = 0xFFF40000;
    pMadDevCnfg->LegacyPci.u.type0.BaseAddresses[5] = 0xFFF50000;
    //ULONG   CIS;
    pMadDevCnfg->LegacyPci.u.type0.SubVendorID = MAD_SUBVENDOR_ID;
    pMadDevCnfg->LegacyPci.u.type0.SubSystemID = MAD_SUBSYSTEM_ID;
    //        ULONG   ROMBaseAddress;
    //        UCHAR   CapabilitiesPtr;
    //        UCHAR   Reserved1[3];
    //        ULONG   Reserved2;
    pMadDevCnfg->LegacyPci.u.type0.InterruptLine = 0x0A;
    pMadDevCnfg->LegacyPci.u.type0.InterruptPin = 0x01;
    //       UCHAR   MinimumGrant;       
    //       UCHAR   MaximumLatency;     

    return;
}

/************************************************************************//**
 * MadBus_AllocDeviceSet
 *
 * DESCRIPTION:
 *    This function determines how many devices to support; provide memory for
 *    One device per INT-simulating thread; one thread per processor - 
 *    hence 1:1:1initiates a read to the Bob device.
 *    
 * PARAMETERS: 
 *     @param[in]   pDriverObj   pointer to the driver object 
 *     @param[in]   pCnfgData   pointer to the configuration space 
 *     @param[out]  pNumDevices pointer to the number of devices supported 
 *     @param[in]   pFdoData    pointer to the framework device extension
 *     
 * RETURNS:
 *    @return      pMemBase      pointer to the allocated memory 
 * 
 ***************************************************************************/
PVOID MadBus_AllocDeviceSet(IN PDRIVER_OBJECT pDriverObj,
                            IN PMADBUS_DEVICE_CONFIG_DATA pCnfgData,
                            PULONG pNumDevices)
{
static  LARGE_INTEGER    liZERO = {0, 0};
//static  PHYSICAL_ADDRESS liBoundAddr = {MAD_64KB_ALIGNMENT, 0}; 
//static  LARGE_INTEGER    liAddrMult = {MAD_DEVICE_MAPD_MEM_SIZE, 0};
PHYSICAL_ADDRESS liLoAddr   = MADDEV_LOW_PHYS_ADDR;          
PHYSICAL_ADDRESS liHiAddr   = MADDEV_HIGH_PHYS_ADDR; 
ULONG   NumDevices = *pNumDevices; 
SIZE_T  TotlDevSize = MAD_DEVICE_MEM_SIZE_NODATA + 
                      (SIZE_T)gDriverData.MadDataXtent; //The registry adjusted data extent
SIZE_T  NumBytes   = (NumDevices * TotlDevSize);
PVOID   pMemBaseAlloc   = NULL;
PHYSICAL_ADDRESS liMemBase;

    UNREFERENCED_PARAMETER(pDriverObj);
	UNREFERENCED_PARAMETER(liLoAddr);

	if (gbAffinityOn) //Max number of devices constrained to one per processor 
	    {
		ULONG NumCPUs = KeQueryActiveProcessorCount((PKAFFINITY)NULL); //Static# - unaware of processor hot-plug
		NumDevices    = min(NumDevices, (NumCPUs-1)); //Serial#s 1..n - so don't count processor 0
		NumBytes      = (NumDevices * TotlDevSize); 
	    }

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBus:MadBus_AllocDeviceSet...NumDevices=%d NumBytes=%d\n",
                NumDevices, (ULONG)NumBytes);

	if (NumBytes == 0) //As if a VM has only one processor
	    {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, 
			        "MadBus:MadBus_AllocDeviceSet...(computed) NumBytes=0. Can't proceed!\n");
		*pNumDevices = 0;
		return NULL;
	    }

    //Override device physical address range with vendor specific data - if valid (non-zero)
    //Only at the high end - can't squeeze the low end w/ MmAllocateContiguousMemory(NumBytes, liHiAddr);
    if (!RtlEqualMemory(&pCnfgData->VndrSpecData.liHiAddr, &liZERO, sizeof(PHYSICAL_ADDRESS))) 
        if (!RtlEqualMemory(&pCnfgData->VndrSpecData.liHiAddr, &liHiAddr, sizeof(PHYSICAL_ADDRESS))) 
            liHiAddr = pCnfgData->VndrSpecData.liHiAddr;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBus_AllocDeviceSet memory range: x%X:%X...x%X:%X\n",
                liLoAddr.HighPart,liLoAddr.LowPart, liHiAddr.HighPart,liHiAddr.LowPart);

    //Alloc as much as possible through trial and error
	NumBytes = (NumDevices * TotlDevSize);
    pMemBaseAlloc = MmAllocateContiguousMemory(NumBytes, liHiAddr);
	while ((pMemBaseAlloc == NULL) && (NumDevices > 1))
        {
        NumDevices--;
		NumBytes = (NumDevices * TotlDevSize);
        pMemBaseAlloc = MmAllocateContiguousMemory(NumBytes, liHiAddr);
        }

    ASSERT(pMemBaseAlloc != NULL);
	liMemBase = MmGetPhysicalAddress(pMemBaseAlloc);
	RtlZeroMemory(pMemBaseAlloc, NumBytes);

    *pNumDevices = NumDevices;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBus:MadBus_AllocDeviceSet...returning NumDevices=%d NumBytes=%d pMemBaseAlloc=%p liMemBase.LowPart=x%X\n",
                NumDevices, (ULONG)NumBytes, pMemBaseAlloc, liMemBase.LowPart);

	return pMemBaseAlloc;
}

/************************************************************************//**
 * MadBusEvtDeviceAdd
 *
 * DESCRIPTION:
 *    MadBusEvtDeviceAdd is called by the framework in response to the
 *    AddDevice call from the PnP manager. We create and initialize a 
 *    device object to represent a new instance of Madbus.                                                  
 *    
 * PARAMETERS: 
 *     @param[in]  hDriver     handle to the driver object  
 *     @param[in]  pDeviceInit pointer to the framework device init object
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS 
MadBusEvtDeviceAdd(IN WDFDRIVER hDriver, IN PWDFDEVICE_INIT pDeviceInit)
{
PDRIVER_DATA  pDriverData = &gDriverData; // MadBusDriverGetContextData(hDriver);
PDRIVER_OBJECT pDriverObj = pDriverData->pDriverObj;

WDF_CHILD_LIST_CONFIG      hConfig;
WDF_OBJECT_ATTRIBUTES      FdoAttrs;
NTSTATUS                   NtStatus;
WDFDEVICE                  hDevice;
WDF_IO_QUEUE_CONFIG        hQueueConfig;
PNP_BUS_INFORMATION        busInfo;
PFDO_DEVICE_DATA           pFdoData;
WDFQUEUE                   hQueue;
WDF_FILEOBJECT_CONFIG      fileConfig;
//WDF_FDO_EVENT_CALLBACKS    FdoEventCallbacks;
//WDF_OBJECT_ATTRIBUTES      SpinLockAttrs;
ULONG j;

    PAGED_CODE ();
    UNREFERENCED_PARAMETER(hDriver);

    pDriverData->NumBusFDOs++; 
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBusEvtDeviceAdd...pDeviceInit=%p BusNum=%d\n",
                pDeviceInit, pDriverData->NumBusFDOs);

    if (pDriverData->NumBusFDOs >= eMaxBusType)
        {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                    "MadBusEvtDeviceAdd... ntstatus=x%X\n", NtStatus);
        return NtStatus;
        }

    // Initialize all the properties specific to the device.
    // Framework has default values for the one that are not set explicitly here. 
    // So please read the doc and make sure you are okay with the defaults.
    WdfDeviceInitSetDeviceType(pDeviceInit, FILE_DEVICE_BUS_EXTENDER);

    // Artifact of toaster. It must have been a crude synchronization technique.
    // We definitely don't need this if we need to have multiple sharing openers
    //WdfDeviceInitSetExclusive(pDeviceInit, TRUE); ////////////////////////////
 
    // Since this is pure software bus enumerator, we don't have to register for
    // any PNP/Power callbacks. Framework will take the default action for
    // all the PNP and Power IRPs.
    //
    // WDF_ DEVICE_LIST_CONFIG describes how the framework should handle
    // dynamic child enumeration on behalf of the driver writer.
    // Since we are a bus driver, we need to specify identification description for our child devices.
    // This description will serve as the identity of our child device. 
    // Since the description is opaque to the framework, we have to provide bunch of callbacks to 
    // compare, copy, or free any other resources associated with the description.
    WDF_CHILD_LIST_CONFIG_INIT(&hConfig, sizeof(PDO_IDENTIFICATION_DESCRIPTION),
                               MadBusEvtDeviceListCreatePdo);

    // This function pointer will be called when the framework needs to copy a identification description
    // from one location to another.
    // An implementation of this function is only necessary if the description contains description
    // relative pointer values (like LIST_ENTRY for instance).
    // If set to NULL, the framework will use RtlCopyMemory to copy an identification description. 
    // In this sample, it's not required to provide these callbacks. they are added just for illustration.
    hConfig.EvtChildListIdentificationDescriptionDuplicate = 
            MadBusEvtChildListIdentificationDescriptionDuplicate;

    // This function pointer will be called when the framework needs to compare two identificaiton descriptions. 
    // If left NULL a call to RtlCompareMemory will be used to compare two identificaiton descriptions.
    hConfig.EvtChildListIdentificationDescriptionCompare = 
            MadBusEvtChildListIdentificationDescriptionCompare;

    // This function pointer is called when the framework needs to free a identification description.
    // An implementation of this function is only necessary if 
    // the description contains dynamically allocated memory that needs to be freed. 
    // The actual identification description pointer itself will be freed by the framework.
    hConfig.EvtChildListIdentificationDescriptionCleanup = 
            MadBusEvtChildListIdentificationDescriptionCleanup;

    // Tell the framework to use the built-in childlist to track the state of the device 
    // based on the configuration we just created.
    WdfFdoInitSetDefaultChildListConfig(pDeviceInit, 
                                        &hConfig, WDF_NO_OBJECT_ATTRIBUTES);

    // WDF_FILEOBJECT_CONFIG_INIT struct to tell the framework whether we are interested in handling 
    // Create, Close and Cleanup requests generated when an app or kernel component opens the device.
    // If we don't register, the framework default behaviour is to complete these requests with STATUS_SUCCESS.
    // A driver might be interested in registering these events if it wants to 
    // do security validation and also wants to maintain per handle (fileobject) context.
    WDF_FILEOBJECT_CONFIG_INIT(&fileConfig,
                               MadBusEvtFileCreate,
                               MadBusEvtFileClose,
                               WDF_NO_EVENT_CALLBACK); // not interested in Cleanup
 
    WdfDeviceInitSetFileObjectConfig(pDeviceInit,
                                     &fileConfig,
                                     WDF_NO_OBJECT_ATTRIBUTES);

    // Initialize attributes structure to specify size and accessor function for storing device context.
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&FdoAttrs, FDO_DEVICE_DATA);

    // Create a framework device object. 
    // In response to this call, framework creates a WDM deviceobject and attaches to the PDO.
     NtStatus = WdfDeviceCreate(&pDeviceInit, &FdoAttrs, &hDevice);
    if (!NT_SUCCESS(NtStatus)) 
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadBusEvtDeviceAdd:WdfDeviceCreate failed!...NtStatus=x%X\n",
                    NtStatus);

		MadWriteEventLogMesg(pDriverObj, MADBUS_CREATE_DEVICE_ERROR, 1, 
                             sizeof(NTSTATUS), (PWSTR)&NtStatus); //Minimal payload
        return NtStatus;
        }

	ASSERT(pDeviceInit == NULL); //Because WdfDeviceCreate is supposed to  set this

    // Configure a default queue so that requests that are not configure-fowarded using 
    // WdfDeviceConfigureRequestDispatching to goto other queues get dispatched here.
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&hQueueConfig, WdfIoQueueDispatchParallel);
    hQueueConfig.EvtIoDeviceControl = MadBusEvtIoDeviceControl;

    NtStatus =
    WdfIoQueueCreate(hDevice, &hQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &hQueue);
    if (!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadBusEvtDeviceAdd:WdfIoQueueCreate failed!...NtStatus=x%X\n",
                    NtStatus);
		MadWriteEventLogMesg(pDriverObj, MADBUS_CREATE_DEVICE_ERROR, 1, 
                             sizeof(NTSTATUS), (PWSTR)&NtStatus); //Minimal payload
        return NtStatus;
        }

    // Get the device context.
    //ASSERT(pDriverData->NumBusFDOs < eMaxBusType);
    pDriverData->hDevFDO[pDriverData->NumBusFDOs] = hDevice;
    pFdoData = MadBusFdoGetData(hDevice);
    pFdoData->SerialNo = pDriverData->NumBusFDOs;
    pFdoData->pDriverData = pDriverData;

    //Declare all device #s to be available
    for (j = 0; j < MAD_MAX_DEVICES; j++)
        {
        pFdoData->PdoDevOidsList[j].pPhysDevObj = NULL;
        pFdoData->PdoDevOidsList[j].hPhysDevice = (WDFDEVICE)NULL;
        }

    // Create device interface for this device. 
    // The interface will be enabled by the framework when we return from StartDevice successfully.
    // Clients of this driver will open this interface and send ioctls.
    if (pDriverData->NumBusFDOs == eGenericBus)
        NtStatus = 
        WdfDeviceCreateDeviceInterface(hDevice, 
	                                   &GUID_DEVINTERFACE_MADBUS_ONE,
                                       // A reference string could be be appended to the symbolic link.
                                       // Some drivers register multiple interfaces for the same device
                                       // and use the reference string to distinguish between them
                                       NULL); 
    else
        NtStatus = 
        WdfDeviceCreateDeviceInterface(hDevice,
                                       &GUID_DEVINTERFACE_MADBUS_TWO,
                                       NULL); // A reference string would be be appended to the symbolic link.
     if (!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadBusEvtDeviceAdd:WdfDeviceCreateDeviceInterface failed!... SerialNo=%d NtStatus=x%X\n",
                    pDriverData->NumBusFDOs, NtStatus);
		MadWriteEventLogMesg(pDriverObj, MADBUS_CREATE_DEVICE_ERROR, 1, 
                             sizeof(NTSTATUS), (PWSTR)&NtStatus); //Minimal payload
        return NtStatus;
        }

    //Assign Int parms to be referenced in Config-Mngt
    pFdoData->OurVector = gIdtBaseDX; 
	pFdoData->OurIRQL   = (KIRQL)gDevIRQL;  

    // This value is used in responding to the IRP_MN_QUERY_MadBus_INFORMATION for the child devices. 
    // This is an optional information provided to uniquely identify the bus the device is connected.
    busInfo.BusNumber = 0;
    if (pDriverData->NumBusFDOs == eGenericBus)
        {
        busInfo.LegacyBusType = MAD_BUS_INTERFACE_TYPE; // PNPBus;
        busInfo.BusTypeGuid   = MAD_GUID_BUS_TYPE;      // GUID_DEVCLASS_MADDEVICE;
        }
    else
        {
        busInfo.LegacyBusType = MAD_BUS_INTERFACE_TYPE_SCSI; //Internal;
        busInfo.BusTypeGuid   = MAD_GUID_BUS_TYPE_SCSI;      //GUID_BUS_TYPE_INTERNAL
        }
    WdfDeviceSetBusInformationForChildren(hDevice, &busInfo);

    NtStatus = MadBus_WmiRegistration(hDevice);
    if (!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadBusEvtDevAdd:MadBus_WmiRegistration failed...NtStatus=x%X - continuing\n",
					NtStatus);
        //return NtStatus;
        }

    //Create, initialize the MAD device environment
    RtlCopyMemory(&pFdoData->BusInstanceConfigData, 
                  &pDriverData->MadDevDfltCnfg, MADBUS_DEV_CNFG_SIZE);
    pFdoData->BusInstanceConfigData.VndrSpecData.Irql = pFdoData->OurIRQL;
	 
	// Retrieve addresses for the allocated device memory
    pFdoData->pDeviceSetBase = pDriverData->pDeviceSetBase;
	pFdoData->liDeviceSetBase = MmGetPhysicalAddress(pFdoData->pDeviceSetBase);

    // Check the registry to see if we need to enumerate child devices .
    //NtStatus = MadBus_DoStaticEnumeration(hDevice);
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBusEvtDevAdd... Busnum=%d pDevsBase=%p, liDevsBase=x%X:%X\n",
                pDriverData->NumBusFDOs, pFdoData->pDeviceSetBase,
                pFdoData->liDeviceSetBase.HighPart, pFdoData->liDeviceSetBase.LowPart);

    return STATUS_SUCCESS;
}

/************************************************************************//**
 * MadBusFdoEvtWdmIrpPreprocess
 *
 * DESCRIPTION:
 *    This function is a callback used for special processing of IRP_MJ_PNP
 *    & IRP_MJ_INTERNAL_DEVICE_CONTROL irps
 *
 * PARAMETERS:
 *     @param[in]  hDevice   handle to our device
 *     @param[in]  pIRP      pointer to the I/o Request Pkt for this request
 *
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 *
 ***************************************************************************/
NTSTATUS MadBusFdoEvtWdmIrpPreprocess(WDFDEVICE hDevice, PIRP pIRP)
{
    PFDO_DEVICE_DATA   pFdoData = MadBusFdoGetData(hDevice);
    PIO_STACK_LOCATION pIoStackLoc = IoGetCurrentIrpStackLocation(pIRP);
    UCHAR              MajrFunxn = pIoStackLoc->MajorFunction;
    UCHAR              MinrFunxn = pIoStackLoc->MinorFunction;
    //
    NTSTATUS NtStatus = STATUS_SUCCESS;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadBusFdoEvtWdmIrpPreprocess... BusNum=%d IrpMajor=x%X IrpMinor=x%X\n",
                pFdoData->SerialNo, MajrFunxn, MinrFunxn);

    switch (MajrFunxn)
    {
    case IRP_MJ_PNP:
        switch (MinrFunxn)
        {
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            NtStatus = 
            MadBusFdo_QueryDeviceRelations(pFdoData, pIRP, pIoStackLoc);
            break;

        default:
            GENERIC_SWITCH_DEFAULTCASE_ASSERT;
        }
        break;

    case IRP_MJ_INTERNAL_DEVICE_CONTROL: //This ioctl is common from other kernel-mode drivers
                                         //but we currently have no ioctls defined so we fall through
    default:
        GENERIC_SWITCH_DEFAULTCASE_ASSERT;
    };

    //Necessary because D-V forces the client driver to init this status to STATUS_NOT_SUPPORTED (0xC00000BB)
    pIRP->IoStatus.Status = NtStatus; 
    IoSkipCurrentIrpStackLocation(pIRP);
    NtStatus = WdfDeviceWdmDispatchPreprocessedIrp(hDevice, pIRP);

    return NtStatus;
}

NTSTATUS MadBusFdo_QueryDeviceRelations(PFDO_DEVICE_DATA pFdoData, PIRP pIRP, 
                                        PIO_STACK_LOCATION pIoStackLoc)

{
    register ULONG j;
    ULONG QueryType = pIoStackLoc->Parameters.QueryDeviceRelations.Type;
    PDEVICE_RELATIONS pDevRelations = NULL;
    size_t PoolSize = 
           (sizeof(DEVICE_RELATIONS) + (sizeof(PDEVICE_OBJECT) * (pFdoData->CurrNumPDOs - 1)));

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadBusFdo_QueryDeviceRelations... SerialNo=%d QueryType=%d\n",
                pFdoData->SerialNo, QueryType);

    switch (QueryType)
        {
        case BusRelations:
            pDevRelations =
            (PDEVICE_RELATIONS)ExAllocatePoolWithTag(NonPagedPoolNx, 
                                                     PoolSize, MADBUS_POOL_TAG);
            pDevRelations->Count = pFdoData->CurrNumPDOs;

            for (j = 0; j < pDevRelations->Count; j++)
                {
                pDevRelations->Objects[j] = pFdoData->pDevObjs[j + 1];
                if (pDevRelations->Objects[j] != NULL)
                    ObReferenceObject(pDevRelations->Objects[j]);
                }
            pIRP->IoStatus.Information = (ULONG_PTR)pDevRelations;
            break;

        default:
            GENERIC_SWITCH_DEFAULTCASE_ASSERT;
        }

    return STATUS_SUCCESS;
}

/************************************************************************//**
 * MadBusEvtFileCreate
 *
 * DESCRIPTION:
 *    The framework calls a driver's EvtDeviceFileCreate callback when the
 *    framework receives an IRP_MJ_CREATE request.  The system sends this 
 *    request when a user application opens the device to perform an I/O 
 *    operation, such as reading or writing to a device. This callback is 
 *    called in the context of the thread that created the IRP_MJ_CREATE request.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice     handle to our device 
 *     @param[in]  hRequest    handle to this I/O request
 *     @param[in]  hFileObj    Handle to the fileobj representing the open.
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
VOID MadBusEvtFileCreate(IN WDFDEVICE      hDevice,
                         IN WDFREQUEST     hRequest,
                         IN WDFFILEOBJECT  hFileObj)
{
PFDO_DEVICE_DATA   pFdoData = MadBusFdoGetData(hDevice);

    //PAGED_CODE ();
    UNREFERENCED_PARAMETER(hFileObj);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBusEvtFileCreate...pFdoData=%p BusNum=%d\n", 
                pFdoData, pFdoData->SerialNo);

    WdfRequestComplete(hRequest, STATUS_SUCCESS);

    return;
}

/************************************************************************//**
 * MadBusEvtFileClose
 *
 * DESCRIPTION:
 *    This function is called when all the handles represented by the 
 *    FileObject is closed and all the references to FileObject is removed. 
 *    This callback may get called in an arbitrary thread context instead 
 *    of the thread that called CloseHandle. If we want to delete any
 *    per FileObject context that must be done in the context of the thread 
 *    that made the Create call, we should do that in EvtDeviceCleanup.
 *    
 * PARAMETERS: 
 *     @param[in]  hFileObj    handle to fileobject that represents the open.
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
VOID MadBusEvtFileClose(IN WDFFILEOBJECT hFileObj)
{
PFDO_DEVICE_DATA pFdoData = MadBusFdoGetData(WdfFileObjectGetDevice(hFileObj));

    //PAGED_CODE ();

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBusEvtFileClose...pFdoData=%p BusNum=%d\n",
				pFdoData, pFdoData->SerialNo);

    return;
}

/************************************************************************//**
 * MadBusEvtIoDeviceControl
 *
 * DESCRIPTION:
 *    This function handles user mode PlugIn, UnPlug, Eject & power-state
 *    change requests
 *    
 * PARAMETERS: 
 *     @param[in]  hQueue        handle to our I/O queue for this device.
 *     @param[in]  hRequest      handle to this I/O request
 *     @param[in]  OutBufrLen    length of the buffer to the device 
 *     @param[in]  InBufrLen     length of the buffer from the device   
 *     @param[in]  IoControlCode driver/system-defined I/O control code
 *                               associated with the request.
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
VOID MadBusEvtIoDeviceControl(IN WDFQUEUE hQueue, IN WDFREQUEST hRequest, 
                              IN size_t OutBufrLen, IN size_t InBufrLen,
                              IN ULONG  IoControlCode)

{
WDFDEVICE                hDevice = WdfIoQueueGetDevice(hQueue);
PFDO_DEVICE_DATA         pFdoData = MadBusFdoGetData(hDevice);
size_t                   InfoLen = 0;
PMADBUS_PLUGIN_HARDWARE pPlugIn = NULL;
PMADBUS_UNPLUG_HARDWARE pUnplug = NULL;
PMADBUS_EJECT_HARDWARE  pEject  = NULL;
PMADBUS_SET_POWER_STATE pSetPower = NULL;
NTSTATUS                 NtStatus = STATUS_INVALID_PARAMETER;

    PAGED_CODE ();

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                 "MadBusEvtIoDeviceControl...hDevice=%p BusNum=%d Ioctl=x%X\n",
                 hDevice, pFdoData->SerialNo, IoControlCode);

    switch (IoControlCode)
        {
        case IOCTL_MADBUS_PLUGIN_HARDWARE:
            NtStatus = WdfRequestRetrieveInputBuffer(hRequest, 
                                                     sizeof(MADBUS_PLUGIN_HARDWARE),
                                                     (PVOID *)&pPlugIn, &InfoLen);
            if(!NT_SUCCESS(NtStatus))
                {
                TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
					        "MadBusEvtIoDeviceControl:WdfRequestRetrieveInputBuffer failed! ntstatus=x%X\n",
							NtStatus);
                break;
                }

            //ASSERT(InfoLen >= sizeof(MADBUS_PLUGIN_HARDWARE));

            // Make sure the IDs is two NULL terminated.
            if ((pPlugIn->HardwareIDs[MADSIM_HARDWARE_ID_MAX_LEN-1] != UNICODE_NULL)  ||
                (pPlugIn->HardwareIDs[MADSIM_HARDWARE_ID_MAX_LEN] != UNICODE_NULL)) 
                {
		        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
					        "MadBusEvtIoDeviceControl...Plugin ioctl improper format\n");
                NtStatus = STATUS_INVALID_PARAMETER;
                //break;
                }

            NtStatus = MadBus_PlugInDevice(hDevice, pPlugIn->HardwareIDs, 
                                           MADSIM_HARDWARE_ID_MAX_LEN,
                                           pPlugIn->SerialNo);
            break;

        case IOCTL_MADBUS_UNPLUG_HARDWARE:
            NtStatus =
            WdfRequestRetrieveInputBuffer(hRequest,
                                          sizeof(MADBUS_UNPLUG_HARDWARE), 
	  	                                 (PVOID *)&pUnplug, &InfoLen);
            if(!NT_SUCCESS(NtStatus)) 
                {
                TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
					        "MadBusEvtIoDeviceControl:WdfRequestRetrieveInputBuffer failed! NtStatus=x%X\n",
							NtStatus);
                break;
                }

            if (pUnplug->Size == InBufrLen)
                NtStatus = MadBus_UnPlugDevice(hDevice, pUnplug->SerialNo);
            break;

        case IOCTL_MADBUS_EJECT_HARDWARE:
            NtStatus = 
		    WdfRequestRetrieveInputBuffer(hRequest, sizeof(MADBUS_EJECT_HARDWARE),
                                          (PVOID *)&pEject, &InfoLen);
            if(!NT_SUCCESS(NtStatus))
                {
                TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
					        "MadBusEvtIoDeviceControl:WdfRequestRetrieveInputBuffer failed! NtStatus=x%X\n",
							NtStatus);
                break;
                }

            if (pEject->Size == InBufrLen)
                NtStatus = MadBus_EjectDevice(hDevice, pEject->SerialNo);
            break;

        case IOCTL_MADBUS_SET_POWER_STATE:
            NtStatus = 
            WdfRequestRetrieveInputBuffer(hRequest, 
                                          sizeof(MADBUS_SET_POWER_STATE),
                                          (PVOID *)&pSetPower, &InfoLen);
            if(!NT_SUCCESS(NtStatus))
                {
                TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
					        "MadBusEvtIoDeviceControl:WdfRequestRetrieveInputBuffer failed! ntstatus=x%X\n",
							NtStatus);
                break;
                }

            NtStatus = MadBus_SetPowerState(hDevice, pSetPower);
            break;

        default:
            NtStatus = MadBusEvtDevCntl_DevSimUsrInt(hQueue, hRequest, 
                                                     OutBufrLen, InBufrLen,
                                                     IoControlCode);
            return;
        } //end switch
	
	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBusEvtIoDeviceControl...NtStatus=x%X\n", NtStatus);

    WdfRequestCompleteWithInformation(hRequest, NtStatus, InfoLen);
	
	return;
}

/************************************************************************//**
 * MadBus_PlugInDevice
 *
 * DESCRIPTION:
 *    This function is called when the enum application tells us that a new
 *    device on the bus has arrived.  We create a description structure in a
 *    stack, fill in information about the child device and call 
 *    WdfChildListAddOrUpdateChildDescriptionAsPresent to add the device.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice     handle to our device 
 *     @param[in]  HardwareIds pointer to the hardware list 
 *     @param[in]  HardwareIds pointer to the compatibility hardware list 
 *     @param[in]  SerialNo    the unit number of the device to plugin
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadBus_PlugInDevice(__in WDFDEVICE hDevice, __in PWCHAR HardwareIds,
                             __in size_t CchHardwareIds, __in ULONG SerialNo)

{
PDO_IDENTIFICATION_DESCRIPTION description;
NTSTATUS                       NtStatus = STATUS_SUCCESS;
PFDO_DEVICE_DATA               pFdoData = MadBusFdoGetData(hDevice);
ULONG                          BusNum = pFdoData->SerialNo;

    PAGED_CODE ();

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadBus_PlugInDevice... Bus#:Ser#=%d:%d HardwareIds=%S HardwareIds=%s\n",
                BusNum, SerialNo, HardwareIds, CchHardwareIds);

	if ((SerialNo < 1) || (SerialNo > pFdoData->pDriverData->NumAllocDevices))
        NtStatus = STATUS_INVALID_PARAMETER;

    if (pFdoData->PdoDevOidsList[SerialNo].pPhysDevObj != NULL)
        NtStatus = STATUS_CONNECTION_IN_USE;

    if (NtStatus != STATUS_SUCCESS)
        goto MadBusPluginExit;

    // Initialize the description with the information about the newly plugged in device.
    WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header, 
                                                     sizeof(description));
    description.SerialNo       = SerialNo;
    description.CchHardwareIds = CchHardwareIds;
    description.HardwareIds    = HardwareIds;

    // Call the framework to add this child to the childlist. 
    // This call will internally call our DescriptionCompare callback to check whether this device is a new device
    //  or existing device.
    // If it's a new device, the framework will call DescriptionDuplicate to create a copy of this description
    // in nonpaged pool.
    // The actual creation of the child device will happen when the framework receives a QUERY_DEVICE_RELATION
	// request from the PNP manager in response to InvalidateDeviceRelations call made as part of adding a new child.
    WDFCHILDLIST hChildList = WdfFdoGetDefaultChildList(hDevice);
    NtStatus = 
    WdfChildListAddOrUpdateChildDescriptionAsPresent(hChildList,
                                                     &description.Header,
                                                     NULL); // AddressDescription
    //if (NtStatus == STATUS_OBJECT_NAME_EXISTS) 
        //The description is already present in the list, the serial number is not unique 
        // NtStatus = STATUS_INVALID_PARAMETER;  //return error.
	if (NtStatus == STATUS_SUCCESS)
	    {
		if (pFdoData->PdoDevOidsList[SerialNo].pPhysDevObj == NULL) 
     	    {
            //Mark this slot as used temporarily before the oids are assigned in CreatePDO
            pFdoData->PdoDevOidsList[SerialNo].pPhysDevObj = (PDEVICE_OBJECT)(PVOID)-1;
            pFdoData->PdoDevOidsList[SerialNo].hPhysDevice = (WDFDEVICE)-1;
		    }

		pFdoData->MadBusWmiData.InfoClassData.CurrDevCount++;
		if (pFdoData->MadBusWmiData.InfoClassData.CurrDevCount > pFdoData->MadBusWmiData.InfoClassData.TotlDevCount)
			pFdoData->MadBusWmiData.InfoClassData.TotlDevCount++;
	    }

MadBusPluginExit:;
    if (NtStatus != STATUS_SUCCESS)
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                    "MadBus_PlugInDevice... plugin/create PDO for busnum:serial# %d:%d fail! ntstatus=x%X\n",
                    BusNum, SerialNo, NtStatus);
 
    return NtStatus;
}

/************************************************************************//**
 * MadBus_UnPlugDevice
 *
 * DESCRIPTION:
 *       The application has told us a device has departed from the bus.
 *       We therefore need to flag the PDO as no longer present.
 *
 * PARAMETERS: 
 *     @param[in]  hDevice     handle to our device 
 *     @param[in]  SerialNo    the unit number of the device to unplug
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadBus_UnPlugDevice(WDFDEVICE hDevice, ULONG SerialNo)
{ 
PFDO_DEVICE_DATA               pFdoData = MadBusFdoGetData(hDevice);
ULONG                          BusNum = pFdoData->SerialNo;
NTSTATUS                       NtStatus = STATUS_SUCCESS;
WDFCHILDLIST                   hList;
WDF_CHILD_RETRIEVE_INFO        childInfo;
WDFDEVICE                      hChild;

    PAGED_CODE ();

    hList = WdfFdoGetDefaultChildList(hDevice);

    if (0 == SerialNo)
        {
        // Unplug everybody.  
        // We do this by starting a scan and then not reporting any children upon its completion
        NtStatus = STATUS_SUCCESS;

        WdfChildListBeginScan(hList);
        // A call to WdfChildListBeginScan indicates to the framework that the driver is about to scan for dynamic children.
        // After this call has returned, all previously reported children associated with this will be marked as potentially missing.
        // A call to either WdfChildListUpdateChildDescriptionAsPresent or WdfChildListMarkAllChildDescriptionsPresent will mark
		// all previuosly reported missing children as present.
        // If any children currently present are not reported present by calling WdfChildListUpdateChildDescriptionAsPresent 
        // at the time of WdfChildListEndScan, they will be reported as missing to the PnP subsystem after WdfChildListEndScan call
		// has returned, the framework will invalidate the device relations for the FDO associated with the list and report the changes
        WdfChildListEndScan(hList);
        }
    else 
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadBus_UnPlugDevice... busnum:unit %d:%d\n", 
                    BusNum, SerialNo);

        if (pFdoData->PdoDevOidsList[SerialNo].pPhysDevObj == NULL)
            {
            NtStatus = STATUS_INVALID_PARAMETER;
            goto MadBusUnplugExit;
            }

        PDO_IDENTIFICATION_DESCRIPTION description;
        WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header,
                                                         sizeof(description));
        description.SerialNo = SerialNo;
        WDF_CHILD_RETRIEVE_INFO_INIT(&childInfo, &description.Header);
        hChild = WdfChildListRetrievePdo(hList, &childInfo);
        if (hChild != NULL) 
			MadBusPdoFreeResrcs(hChild, SerialNo);
          
        // WdfFdoUpdateChildDescriptionAsMissing indicates to the framework that a
        // child device that was previuosly detected is no longe present on the bus.
        // This API can be called by itself or after a call to WdfChildListBeginScan.
        // After this call has returned, the framework will invalidate the device
        // relations for the FDO associated with the list and report the changes.
        NtStatus = WdfChildListUpdateChildDescriptionAsMissing(hList,
                                                               &description.Header);
        /*if (NtStatus == STATUS_NO_SUCH_DEVICE)
            {
            // serial number didn't exist. Remap it to a NtStatus that user
            // application can understand when it gets translated to win32 error code.
            NtStatus = STATUS_INVALID_PARAMETER;
            } */
        }

MadBusUnplugExit:;
    //if (NtStatus != STATUS_SUCCESS)
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                    "MadBus_UnPlugDevice... busnum:serialno %d:%d ntstatus=x%X\n",
                    BusNum, SerialNo, NtStatus);

    return NtStatus;
}

/************************************************************************//**
 * MadBus_EjectDevice
 *
 * DESCRIPTION:
 *       The application is telling us a device is to be removed from the bus.
 *       We therefore need to flag the PDO as no longer present.
 *       In a real situation the driver gets notified by an interrupt when
 *       the user presses the Eject button on the device.
 *
 * PARAMETERS: 
 *     @param[in]  hDevice     handle to our device 
 *     @param[in]  SerialNo    the unit number of the device to unplug
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadBus_EjectDevice(WDFDEVICE hDevice, ULONG SerialNo)
{
PFDO_DEVICE_DATA pFdoData = MadBusFdoGetData(hDevice);
ULONG            BusNum = pFdoData->SerialNo;
WDFDEVICE        hChild;
NTSTATUS         NtStatus = STATUS_INVALID_PARAMETER;
WDFCHILDLIST     hList;

    PAGED_CODE();

    hList = WdfFdoGetDefaultChildList(hDevice);

    // A zero serial number means eject all children
    if (0 == SerialNo) {
        WDF_CHILD_LIST_ITERATOR iterator;

        WDF_CHILD_LIST_ITERATOR_INIT(&iterator, WdfRetrievePresentChildren);

        WdfChildListBeginIteration(hList, &iterator);

        for ( ; ; )
            {
            WDF_CHILD_RETRIEVE_INFO         childInfo;
            PDO_IDENTIFICATION_DESCRIPTION  description;
            BOOLEAN                         ret;

            // Init the structures.
            WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header,
                                                             sizeof(description));
            WDF_CHILD_RETRIEVE_INFO_INIT(&childInfo, &description.Header);

            // Get the device identification description
            NtStatus = WdfChildListRetrieveNextDevice(hList, &iterator, &hChild, &childInfo);
            if (!NT_SUCCESS(NtStatus) || NtStatus == STATUS_NO_MORE_ENTRIES) 
                break;

            ASSERT(childInfo.Status == WdfChildListRetrieveDeviceSuccess);

            // Use that description to request an eject.
            ret = WdfChildListRequestChildEject(hList, &description.Header);
            if (!ret)
                WDFVERIFY(ret);
            }

        WdfChildListEndIteration(hList, &iterator);

        if (NtStatus == STATUS_NO_MORE_ENTRIES) 
            NtStatus = STATUS_SUCCESS;
        }
    else 
        {
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadBus_EjectDevice... busnum:unit %d:%d\n", BusNum, SerialNo);
		if (pFdoData->PdoDevOidsList[SerialNo].pPhysDevObj == NULL)
		    {
            NtStatus = STATUS_INVALID_PARAMETER;
            goto MadBusEjectExit;
		    }
 
		WDF_CHILD_RETRIEVE_INFO  childInfo;
		PDO_IDENTIFICATION_DESCRIPTION description;
        WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header,
                                                         sizeof(description));
		description.SerialNo = SerialNo;
		WDF_CHILD_RETRIEVE_INFO_INIT(&childInfo, &description.Header);
        hChild = WdfChildListRetrievePdo(hList, &childInfo);
        if (hChild != NULL) 
			MadBusPdoFreeResrcs(hChild, SerialNo);

		NtStatus = 
        WdfChildListRequestChildEject(hList, 
                                      &description.Header) ? STATUS_SUCCESS :
                                                             STATUS_DEVICE_DOES_NOT_EXIST;
        }

MadBusEjectExit:;
    if (NtStatus != STATUS_SUCCESS)
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                    "MadBus_EjectDevice... busnum:serialno %d:%d ntstatus=%d\n",
                     BusNum, SerialNo, NtStatus);

    return NtStatus;
}

/************************************************************************//**
 * MadBusPdoFreeResrcs
 *
 * DESCRIPTION:
 *    Shutdown the interrupt thread before releasing memory mapping 
 *    
 * PARAMETERS: 
 *     @param[in]  hChild         handle to the child device to be released 
 *     @param[in]  SerialNo       the device serial #
 *     
 * RETURNS:
 *    @return      void           nothing returned
 * 
 ***************************************************************************/
VOID MadBusPdoFreeResrcs(WDFDEVICE hChild, ULONG SerialNo)
{
static LARGE_INTEGER liZero = {0, 0};  
//
PPDO_DEVICE_DATA     pPdoData = MadBusPdoGetData(hChild);
ULONG                BusNum = pPdoData->pFdoData->SerialNo;
//
ULONG                Signald;
NTSTATUS             NtStatus;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			    "MadBus:MadBusPdoFreeResrcs... busnum:unit %d:%d\n",
                BusNum, SerialNo);

    ASSERT(pPdoData != NULL);
	pPdoData->BusPdoCnfgData.VndrSpecData.DevPowerState = PowerDeviceMaximum;

	NtStatus = KeWaitForSingleObject((PKEVENT)pPdoData->pIntThreadObj,
                                     Executive, KernelMode, FALSE, 
                                     &liZero); //Don't wait - just check 

	if (NtStatus != STATUS_SUCCESS) //If the thread event object is not already signalled
		{                           //indicating that the thread is gone ...
	                                //we need to shut down the thread here before cleanup
	    ASSERT(NtStatus == STATUS_TIMEOUT);

	    //Signal the thread exit event 1st because the PDO int thread will not wait after
	    //the power-down event is signalled before checking the exit event
        Signald = KeSetEvent(&pPdoData->evIntThreadExit, (KPRIORITY)0, FALSE);
        ASSERT(Signald == 0); //Wasn't already signalled

	    Signald = KeSetEvent(&pPdoData->MadSimIntParms.evDevPowerDown, (KPRIORITY)0, FALSE);
        ASSERT(Signald == 0); //Wasn't already signalled

		Signald = KeSetEvent(&pPdoData->MadSimIntParms.evDevPowerUp, (KPRIORITY)0, FALSE);

	    //Wait for the PDO Interrupt thread to exit
	    NtStatus = KeWaitForSingleObject((PKEVENT)pPdoData->pIntThreadObj,
                                         Executive, KernelMode, FALSE, 
                                         NULL); //Wait until signalled
	    ASSERT(NtStatus == STATUS_SUCCESS);
	    }

	//The whole purpose for the synchronization protocol above is 
	//to not free resources until the PDO interrupt thread exits
	MmUnmapIoSpace(pPdoData->pDeviceData, 
                   pPdoData->pFdoData->pDriverData->MadDataXtent);
	MmUnmapIoSpace(pPdoData->pPioWrite,   MAD_MAPD_WRITE_SIZE);
	MmUnmapIoSpace(pPdoData->pPioRead,    MAD_MAPD_READ_SIZE);
	MmUnmapIoSpace(pPdoData->pMadRegs,    MAD_REGISTER_BLOCK_SIZE);

	//Remove device object references from the table of active devices
	//WdfSpinLockAcquire(ghPdoDevOidsLock);
    pPdoData->pFdoData->PdoDevOidsList[SerialNo].pPhysDevObj = NULL;
    pPdoData->pFdoData->PdoDevOidsList[SerialNo].hPhysDevice = (WDFDEVICE)NULL;
	//WdfSpinLockRelease(ghPdoDevOidsLock);

	//Update the WMI information: count of active devices
	pPdoData->pFdoData->MadBusWmiData.InfoClassData.CurrDevCount--;

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			    "MadBus:MadBusPdoFreeResrcs exit...busnum:unit %d:%d\n",
                BusNum, SerialNo);

	return;
}

/************************************************************************//**
 * MadBus_SetPowerState
 *
 * DESCRIPTION:
 *    An ioctl (from MadEnum.exe) is telling us a device is changing
 *    device power state.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice        handle to our device 
 *     @param[in]  pSetPowerState pointer to a structure containing 
 *                                device-id & power-state
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadBus_SetPowerState(WDFDEVICE hDevice, 
                              PMADBUS_SET_POWER_STATE pSetPowerState)
{
NTSTATUS  NtStatus = STATUS_SUCCESS;
PPDO_DEVICE_DATA pPdoData; //* For the device units
PDEVICE_OBJECT   pDevObj;
ULONG SerialNo; 
POWER_STATE PowerState; //* Union
WDFCHILDLIST hList = WdfFdoGetDefaultChildList(hDevice);
PDO_IDENTIFICATION_DESCRIPTION description;
WDF_CHILD_RETRIEVE_INFO        childInfo;
WDFDEVICE                      hChild;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				"MadBus_SetPowerState: hDevice=%p, pSetPowerState=%p\n",
                hDevice, pSetPowerState);

#ifndef MAD_DEVICE_CONTROL_POWER_MNGT
	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				"MadBus_SetPowerState: device controlled power management is not active! Just an FYI - continuing.\n");
#endif

    PowerState.DeviceState =
	(DEVICE_POWER_STATE)(pSetPowerState->nDevPowerState+1);//EnumType starts w/ Unknown=0
    SerialNo = pSetPowerState->SerialNo;
    if (SerialNo > 0)
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadBus_SetPowerState:setting power state for unit %d to %d\n",
			        pSetPowerState->SerialNo, pSetPowerState->nDevPowerState);

        WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header,
                                                         sizeof(description));
        description.SerialNo = SerialNo;
        WDF_CHILD_RETRIEVE_INFO_INIT(&childInfo, &description.Header);
        hChild  = WdfChildListRetrievePdo(hList, &childInfo);
        pDevObj = WdfDeviceWdmGetDeviceObject(hChild); 
        pPdoData = MadBusPdoGetData(hChild); 
        pPdoData->BusPdoCnfgData.VndrSpecData.DevPowerState = PowerState.DeviceState;
        PowerState = PoSetPowerState(pDevObj, DevicePowerState, PowerState);
        }
    else
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadBus_SetPowerState:setting power state for ALL units to %d (not implemented)\n",
			        pSetPowerState->nDevPowerState);
        NtStatus = STATUS_NOT_IMPLEMENTED;
        //TBD
        }

    return NtStatus;
}


/************************************************************************//**
 * MadBusEvtDevCntl_DevSimUsrInt
 *
 * DESCRIPTION:
 *    This function processes ioctls for the simulation 
 *    
 * PARAMETERS: 
 *    @param[in]   hQueue        handle to our I/O queue for this device.
 *     @param[in]  hRequest      handle to this I/O request
 *     @param[in]  OutBufrLen    length of the buffer to the device 
 *     @param[in]  InBufrLen     length of the buffer from the device   
 *     @param[in]  IoControlCode driver-defined I/O control code
                                 associated with the request.
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadBusEvtDevCntl_DevSimUsrInt(IN WDFQUEUE hQueue, IN WDFREQUEST hRequest, 
                                       IN size_t  OutBufrLen, IN size_t InBufrLen,
                                       IN ULONG  IoControlCode)

{
WDFDEVICE            hDevice = WdfIoQueueGetDevice(hQueue);
PFDO_DEVICE_DATA     pFdoData = MadBusFdoGetData(hDevice);
SIZE_T TotlDevSize  = MAD_DEVICE_MEM_SIZE_NODATA +
                      (SIZE_T)gDriverData.MadDataXtent; //The registry adjusted data extent

PMAD_USRINTBUFR       pMadUsrIntBufr;

PVOID                    pDevBase;
LONG                     SerialNo; 
size_t                   XpectLen = sizeof(MAD_USRINTBUFR);       
ULONG                    DevOffset;
PMADBUS_MAP_WHOLE_DEVICE pMapWholeDevice = NULL; 
ULONG j;
size_t               InfoLen = 0;
NTSTATUS                 NtStatus = STATUS_INVALID_PARAMETER;


    UNREFERENCED_PARAMETER(InBufrLen);
    UNREFERENCED_PARAMETER(OutBufrLen);
	UNREFERENCED_PARAMETER(XpectLen);
    ASSERT(pFdoData->SerialNo == eGenericBus);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBusEvtDevCntl_DevSimUsrInt...pFdoData=%p SerialNo=%d Ioctl=x%X\n",
		        pFdoData, pFdoData->SerialNo, ULONG(IoControlCode));

    NtStatus = 
	WdfRequestRetrieveInputBuffer(hRequest, 1/*sizeof(MAD_USRINTBUFR)*/,
                                  (PVOID *)&pMadUsrIntBufr, &InfoLen);
    if(!NT_SUCCESS(NtStatus))
       {
       TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		           "MadBusEvtDevCntl_DevSimUsrInt:WdfRequestRetrieveInputBuffer failed...NtStatus=x%X SerialNo=%d\n",
		           NtStatus, pFdoData->SerialNo);
       WdfRequestComplete(hRequest, NtStatus);
       return NtStatus;
       }

    ASSERT(InfoLen == XpectLen);
    SerialNo = (LONG)pMadUsrIntBufr->SerialNo;

    switch (IoControlCode)
        {
        case IOCTL_MADBUS_INITIALIZE:
            if (SerialNo != -1) //* Initialize a specific device 
                {
                TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
					        "MadBusEvtDevCntl_DevSimUsrInt: initializing device# %d\n",
                            SerialNo);
				DevOffset = (ULONG)((SerialNo - 1) * TotlDevSize);
                pDevBase  = (PVOID)((ULONG_PTR)pFdoData->pDeviceSetBase + DevOffset); 
				RtlFillMemory(pDevBase, TotlDevSize, 0x00);
                InfoLen = 0; //* No data returned
                }        
            else //*It's a request to assign Serial No - determine next available
                {
				InfoLen = 0;                              //Until we complete our business
				NtStatus = STATUS_INSUFFICIENT_RESOURCES; //  "   "     "      "     "

                for (j = 1; j <= pFdoData->pDriverData->NumAllocDevices; j++) //Skip slot zero
					if (pFdoData->PdoDevOidsList[j].pPhysDevObj == NULL)
                        break;  //* Out of for loop

                if (j <= pFdoData->pDriverData->NumAllocDevices)
                    {
                    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
						        "MadBusEvtDevCntl_DevSimUsrInt...assigning device# %d\n", j);
                    pMadUsrIntBufr->SerialNo = (LONG)j;
                    }

                InfoLen = sizeof(MAD_USRINTBUFR);
				NtStatus = STATUS_SUCCESS;
                }
            break;
  
        case IOCTL_MADBUS_MAP_WHOLE_DEVICE:
			if ((SerialNo < 1) || (SerialNo > (LONG)pFdoData->pDriverData->NumAllocDevices))
			    {
				TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
						    "MadBusEvtDevCntl_DevSimUsrInt...Invalid SerialNo=%d for device mapping\n",
                            SerialNo);
				NtStatus = STATUS_INVALID_PARAMETER;
				InfoLen  = 0;
				break;
			    }

			TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				        "MadBusEvtDevCntl_DevSimUsrInt...mapping device# (%d) for the Simulator UI\n",
						SerialNo);
            pMapWholeDevice = (PMADBUS_MAP_WHOLE_DEVICE)pMadUsrIntBufr->ucDataBufr;
            NtStatus = MadBus_MapWholeDevice(pFdoData, pMapWholeDevice, SerialNo);
            InfoLen = sizeof(MAD_USRINTBUFR);
            break;

        case IOCTL_MADBUS_UNMAP_WHOLE_DEVICE:
            pMapWholeDevice = (PMADBUS_MAP_WHOLE_DEVICE)pMadUsrIntBufr->ucDataBufr;
            TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				        "MadBusEvtDevCntl_DevSimUsrInt...unmapping memory (%p) for device# %d from the Simulator UI\n",
                        pMapWholeDevice->pDeviceRegs, SerialNo);
            NtStatus = Mad_UnmapUsrAddr(pMapWholeDevice->pDeviceRegs);
            InfoLen = 0;
            break;

        default:
	        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				        "MadBusEvtDevCntl_DevSimUsrInt...invalid pnp ioctl = x%X\n",
                        IoControlCode);
            break; // default NtStatus is STATUS_INVALID_PARAMETER
        }
	
	if (!(NT_SUCCESS(NtStatus)))
		{ //Brackets around null stmt in the case of Free_build & No WPP_TRACING(KdPrint)
	    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		            "MadBusEvtDevCntl_DevSimUsrInt...NtStatus=x%X\n", NtStatus);
	    }

	WdfRequestCompleteWithInformation(hRequest, NtStatus, InfoLen);
	
	return NtStatus;
}

/************************************************************************//**
 * MadBus_MapWholeDevice
 *
 * DESCRIPTION:
 *    This function maps device memory components into the requesting user 
 *    app's virtual address space. One internal mapping is sufficient because
 *    all components are contiguous
 *    
 * PARAMETERS: 
 *     @param[in]  pFdoData        pointer to the framework device extension
 *     @param[in]  pMapWholeDevice pointer to the structure containing the 
 *                                 device component pointers 
 *     @param[in]  SerialNo        indicating which device 
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadBus_MapWholeDevice(PFDO_DEVICE_DATA pFdoData, 
                               PMADBUS_MAP_WHOLE_DEVICE pMapWholeDevice, ULONG SerialNo)

{
SIZE_T  TotlDevSize = MAD_DEVICE_MEM_SIZE_NODATA + 
                      (SIZE_T)gDriverData.MadDataXtent; //The registry adjusted data extent
ULONG DevOffset = (ULONG)((SerialNo - 1) * TotlDevSize);
PVOID pDevBase  = (PVOID)((ULONG_PTR)pFdoData->pDeviceSetBase + DevOffset); 
//PHYSICAL_ADDRESS liDeviceRegs = {0, 0};
BOOLEAN  bLargeMap = TRUE;
NTSTATUS NtStatus;

    //One mapping for all components of device memory
    //
    pMapWholeDevice->pDeviceRegs = NULL;
	NtStatus = Mad_MapSysAddr2UsrAddr(pDevBase, TotlDevSize,
                                      &pMapWholeDevice->liDeviceRegs, 
                                      &pMapWholeDevice->pDeviceRegs);
	if (!NT_SUCCESS(NtStatus))
	    { //Try to map less, fogetting the data portion of the device
		bLargeMap = FALSE;
		NtStatus = Mad_MapSysAddr2UsrAddr(pDevBase, MAD_DEVICE_MEM_SIZE_NODATA,
                                          &pMapWholeDevice->liDeviceRegs, 
                                          &pMapWholeDevice->pDeviceRegs);
	    }

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBus_MapWholeDevice...NtStatus=x%X pDevBase=%p pDeviceRegs=%p liDeviceRegs=x%X:%X\n",
                NtStatus, pDevBase, pMapWholeDevice->pDeviceRegs,
				pMapWholeDevice->liDeviceRegs.HighPart, 
                pMapWholeDevice->liDeviceRegs.LowPart);

	if (pMapWholeDevice->pDeviceRegs != NULL)
    	{
		//All device memory components are contiguous so using offsets works fine
        pMapWholeDevice->pPioRead  = 
        (PVOID)((ULONG_PTR)pMapWholeDevice->pDeviceRegs + MAD_MAPD_READ_OFFSET); 
	    pMapWholeDevice->pPioWrite = 
        (PVOID)((ULONG_PTR)pMapWholeDevice->pDeviceRegs + MAD_MAPD_WRITE_OFFSET); 
		//
	    pMapWholeDevice->pDeviceData = NULL;
		if (bLargeMap)
			pMapWholeDevice->pDeviceData = 
            (PVOID)((ULONG_PTR)pMapWholeDevice->pDeviceRegs + MAD_DEVICE_DATA_OFFSET); 
	    }

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBus_MapWholeDevice...pDeviceRegs=%p, pPioRead=%p, pPioWrite=%p, pDeviceData=%p\n",
	            pMapWholeDevice->pDeviceRegs, pMapWholeDevice->pPioRead,
			    pMapWholeDevice->pPioWrite,   pMapWholeDevice->pDeviceData);

    return NtStatus;
}

/************************************************************************//**
 * MadBus_DoStaticEnumeration
 *
 * DESCRIPTION:
 *    This function enables us to statically enumerate child devices
 *    during start instead of running the enum.exe/notify.exe to
 *    enumerate maddevice devices. It is an artifact of the toaster sample.
 *    It is working but unused.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice     handle to our device 
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadBus_DoStaticEnumeration(IN WDFDEVICE hDevice)

{
static   ULONG value = 1;
register ULONG i;
WDFKEY      hKey = NULL;
NTSTATUS    NtStatus = STATUS_UNSUCCESSFUL;

DECLARE_CONST_UNICODE_STRING(valueName, L"NumberOfMadDevices");

    // If the registry value doesn't exist, we will use the hardcoded default number.
    //
    //value = DEF_STATICALLY_ENUMERATED_MADDEVICES;

    // Open the device registry and read the "NumberOfMadDevices" value.
    //
    //NtStatus = WdfDeviceOpenRegistryKey(hDevice, PLUGPLAY_REGKEY_DEVICE, STANDARD_RIGHTS_ALL,
    //                                    NULL, // PWDF_OBJECT_ATTRIBUTES
    //                                    &hKey);
    if (NT_SUCCESS (NtStatus))
	    {
        NtStatus = WdfRegistryQueryULong(hKey, &valueName, &value);

        WdfRegistryClose(hKey);
        hKey = NULL; // Set hKey to NULL to catch any accidental subsequent use.

        if (NT_SUCCESS (NtStatus))
            {
            // Make sure it doesn't exceed the max. This is required to prevent
            // denial of service by enumerating large number of child devices.
            //
            //value = min(value, MAX_STATICALLY_ENUMERATED_MADDEVICES);
            }
        else
		    {
            return STATUS_SUCCESS; // This is an optional property.
            }
        }

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBus_DoStaticEnumeration...enumerating %d MadDevice devices\n", value);

    for(i=1; i <= value; i++)
        {
        // Value of i is used as serial number.
        NtStatus = MadBus_PlugInDevice(hDevice, MADSIM_HARDWARE_ID1,
                                       MADSIM_HARDWARE_ID_LENGTH1, i);
        }

    return NtStatus;
}

/**************************************************************************//**
 * MadBusReadRegParms
 *
 * DESCRIPTION:
 *    This function reads parameters from the registry 
 *    
 * PARAMETERS: 
 *     @param[in]  pDriverObj   pointet to the driver object
 *     @param[in]  pParmPath   pointer to the unicode name of the registry key
 *     @param[out] pDevIRQL, pIdtBaseDX, pbAffinityOn pNumFilters,pMaxDevices
 *                 pointers to the parameters to be read
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 *****************************************************************************/
NTSTATUS MadBusReadRegParms(IN PDRIVER_OBJECT pDriverObj,   
                            IN PUNICODE_STRING pParmPath,    
                            OUT PULONG pDevIRQL, OUT PULONG pIdtBaseDX, 
                            PBOOLEAN pbAffinityOn, OUT PULONG pNumFilters,
                            OUT PULONG pMaxDevices, OUT PULONG pMadDataXtent) 
{
RTL_QUERY_REGISTRY_TABLE RegParmTable[10]; //Parameter table -- parameters key
NTSTATUS NtStatus = STATUS_SUCCESS;  
HANDLE  hKey;
OBJECT_ATTRIBUTES  ObjAttrs;

    UNREFERENCED_PARAMETER(pDriverObj);
    ASSERT(pParmPath != NULL);
 
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBusReadRegParms...RegPath = %S\n", pParmPath->Buffer);

// We set these values to their defaults in case there are any failures
//
    *pDevIRQL      = MAD_DFLT_DEVICE_IRQL;
    *pIdtBaseDX    = MAD_DFLT_IDT_BASE_INDX;
	*pbAffinityOn  = MAD_DFLT_AFFINITY_ON;
    *pNumFilters   = MAD_DFLT_NUM_FILTERS; 
    *pMaxDevices   = MAD_MAX_DEVICES;
	*pMadDataXtent = MAD_DEFAULT_DATA_EXTENT;

	InitializeObjectAttributes(&ObjAttrs, pParmPath,
                               OBJ_CASE_INSENSITIVE, NULL, (PSECURITY_DESCRIPTOR)NULL);
    NtStatus = ZwOpenKey(&hKey, KEY_QUERY_VALUE, &ObjAttrs);
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBusReadRegParms:ZwOpenKey...NtStatus = x%X\n", NtStatus);
	if (!NT_SUCCESS(NtStatus))
		return NtStatus;

	//Old-fashioned non-WDF registry read of multiple values by setting up a table
    RtlZeroMemory(RegParmTable, sizeof(RegParmTable)); // mandatory
    RegParmTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    RegParmTable[0].Name          = REG_DEVIRQL;
    RegParmTable[0].EntryContext  = pDevIRQL;
    RegParmTable[0].DefaultType   = REG_DWORD;
    RegParmTable[0].DefaultData   = pDevIRQL;
    RegParmTable[0].DefaultLength = sizeof(ULONG);

    RegParmTable[1].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    RegParmTable[1].Name          = REG_IDT_BASE;
    RegParmTable[1].EntryContext  = pIdtBaseDX;
    RegParmTable[1].DefaultType   = REG_DWORD;
    RegParmTable[1].DefaultData   = pIdtBaseDX;
    RegParmTable[1].DefaultLength = sizeof(ULONG);

	RegParmTable[2].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    RegParmTable[2].Name          = REG_AFFINITY_ON;
    RegParmTable[2].EntryContext  = pbAffinityOn;
    RegParmTable[2].DefaultType   = REG_DWORD;
    RegParmTable[2].DefaultData   = pbAffinityOn;
    RegParmTable[2].DefaultLength = sizeof(ULONG);

    RegParmTable[3].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    RegParmTable[3].Name          = REG_NUM_FILTERS;
    RegParmTable[3].EntryContext  = pNumFilters;
    RegParmTable[3].DefaultType   = REG_DWORD;
    RegParmTable[3].DefaultData   = pNumFilters;
    RegParmTable[3].DefaultLength = sizeof(ULONG);

    RegParmTable[4].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    RegParmTable[4].Name          = REG_MAX_DEVICES;
    RegParmTable[4].EntryContext  = pMaxDevices;
    RegParmTable[4].DefaultType   = REG_DWORD;
    RegParmTable[4].DefaultData   = pMaxDevices;
    RegParmTable[4].DefaultLength = sizeof(ULONG);

    RegParmTable[5].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    RegParmTable[5].Name          = REG_MAD_DATA_EXTENT;
    RegParmTable[5].EntryContext  = pMadDataXtent;
    RegParmTable[5].DefaultType   = REG_DWORD;
    RegParmTable[5].DefaultData   = pMadDataXtent;
    RegParmTable[5].DefaultLength = sizeof(ULONG);

    NtStatus = 
    RtlQueryRegistryValues((RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL | RTL_REGISTRY_HANDLE),
                           (PCWSTR)hKey, &RegParmTable[0], NULL, NULL);
    if (!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadBusReadRegParms:RtlQueryRegistryValues failed...NtStatus(x%X)\n",
                    NtStatus);
        }
    else
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadBusReadRegistry - Device IRQL   = %d\n", *pDevIRQL);
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadBusReadRegistry - IdtBaseDX     = x%X\n", *pIdtBaseDX);
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadBusReadRegistry - AffinityOn    = %d\n", *pbAffinityOn);
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadBusReadRegistry - NumFilters    = %d\n", *pNumFilters);
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadBusReadRegistry - MaxDevices    = %d\n", *pMaxDevices);
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadBusReadRegistry - MadDataXtent  = %d\n", *pMadDataXtent);
        }

	NTSTATUS NtStatZw = ZwClose(hKey);
	ASSERT(NtStatZw == STATUS_SUCCESS);
	UNREFERENCED_PARAMETER(NtStatZw);

    return (NtStatus);
}

/************************************************************************//**
 * MadBusReadRegConfig
 *
 * DESCRIPTION:
 *    This function reads the default config-space data from the registry
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice    handle to our device 
 *     @param[in]  pCnfgPath  pointer to the unicode name of the reg path
 *     @param[in]  pCnfgData  pointer to the buffer for the returend data
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadBusReadRegConfig(IN WDFDEVICE  hDevice,   
                             IN PUNICODE_STRING pCnfgPath, 
                             OUT PMADBUS_DEVICE_CONFIG_DATA pCnfgData) 
{
UNICODE_STRING ValueName;
NTSTATUS       NtStatus;
ULONG          LenQueried;
ULONG          ValueType;

    UNREFERENCED_PARAMETER(hDevice);

    RtlInitUnicodeString(&ValueName, CONFIG_DATA_VALUE_NAME);
    NtStatus = MadRegReadChunk(hDevice, pCnfgPath, &ValueName, MADBUS_DEV_CNFG_SIZE,
                               (PUCHAR)pCnfgData, &LenQueried, &ValueType);
	if (NT_SUCCESS(NtStatus))
	    {
		//ASSERT(LenQueried == MADBUS_DEV_CNFG_SIZE);
        //ASSERT(ValueType == REG_NONE);
	    }

    //TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	//"MadBusReadRegConfig:MadRegReadChunk returned...x%X\n", NtStatus));

    return NtStatus;
}
