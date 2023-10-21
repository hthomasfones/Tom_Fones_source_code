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
/*  Module  NAME : MadBusPdo.cpp                                               */
/*                                                                             */
/*  DESCRIPTION  : PDO functions for the simulator-bus driver                  */
/*                 Derived from WDK-Toaster\bus\pdo.c                          */
/*                                                                             */
/*******************************************************************************/

#include "MadBus.h"
#include <ntddscsi.h>
#include <ntdddisk.h>
//#include <ntddstor.h>
#include "..\Includes\MadUtilFunxns.h"

#ifdef WPP_TRACING
    #include "trace.h"
    #include "MadBusPdo.tmh"
#endif

ULONG BusEnumDebugLevel;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, MadBus_CreatePdo)
#pragma alloc_text(PAGE, MadBusEvtDeviceListCreatePdo)
#endif

/************************************************************************//**
 * MadBusEvtChildListIdentificationDescriptionDuplicate
 *
 * DESCRIPTION:
 *    This function is called when the framework needs to make a copy of a 
 *    description.  This happens when a request is made to create a new child
 *    device by calling WdfChildListAddOrUpdateChildDescriptionAsPresent.
 *    If this function is left unspecified, RtlCopyMemory will be used instead.
 *    Memory for the description is managed by the framework.
 *
 *   NOTE:   Callback is invoked with an internal lock held.  So do not call out
 *   to any WDF function which will require this lock
 *   (basically any other WDFCHILDLIST api)initiates a read to the Bob device.
 *    
 * PARAMETERS: 
 *     @param[in]  hDeviceList  Handle to the default WDFCHILDLIST created
 *                              by the framework.
 *     @param[in]  pSrcpIdDesc  Description of the child being created
 *     @param[in]  pDestpIdDesc Created by the framework in nonpaged pool 
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS 
MadBusEvtChildListIdentificationDescriptionDuplicate(WDFCHILDLIST hDeviceList,
                                                     PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER pSrcpIdDesc,
                                                     PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER pDestpIdDesc)
{
WDFDEVICE hBusFDO = WdfChildListGetDevice(hDeviceList);
PFDO_DEVICE_DATA pFdoData = MadBusFdoGetData(hBusFDO);
PPDO_IDENTIFICATION_DESCRIPTION pSrc, pDest;
size_t safeMultResult;
NTSTATUS NtStatus = STATUS_SUCCESS;
SIZE_T   Len;

    UNREFERENCED_PARAMETER(hDeviceList);

    pSrc = CONTAINING_RECORD(pSrcpIdDesc,
                             PDO_IDENTIFICATION_DESCRIPTION,
                             Header);
    pDest = CONTAINING_RECORD(pDestpIdDesc,
                              PDO_IDENTIFICATION_DESCRIPTION,
                              Header);

    pDest->SerialNo = pSrc->SerialNo;
    pDest->CchHardwareIds = pSrc->CchHardwareIds;
    NtStatus = 
    RtlSizeTMult(pDest->CchHardwareIds, sizeof(WCHAR), &safeMultResult);
    if (NT_SUCCESS(NtStatus))
        {
        pDest->HardwareIds =
        (PWCHAR)ExAllocatePoolWithTag(NonPagedPoolNx, safeMultResult, MADBUS_POOL_TAG);
        if (pDest->HardwareIds == NULL)
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        else
            {
            Len = pDest->CchHardwareIds * sizeof(WCHAR);
            RtlCopyMemory(pDest->HardwareIds, pSrc->HardwareIds, Len);
            }
        }

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadBusEvtChildListIdentificationDescriptionDuplicate... Bus#:Ser#=%d:%d HdwIDs=%S status=x%X\n",
                pFdoData->SerialNo, pDest->SerialNo, pDest->HardwareIds,
                NtStatus);

    return NtStatus;
}

/************************************************************************//**
 * MadBusEvtChildListIdentificationDescriptionCompare
 *
 * DESCRIPTION:
 *    This function is called when the framework needs to compare one 
 *    description with another. Typically this happens whenever a request is
 *     made to add a new child device.  If this function is left unspecified, 
 *    RtlCompareMemory will be used to compare the descriptions.
 *    
 * PARAMETERS: 
 *     @param[in]  hChildList  Handle to the framnework's default WDFCHILDLIST
 *     
 * RETURNS:
 *    @return      BOOLEAN     indicates success or failure
 * 
 ***************************************************************************/
BOOLEAN
MadBusEvtChildListIdentificationDescriptionCompare(WDFCHILDLIST hChildList,
                                                   PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER p1stIdDesc,
                                                   PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER p2ndIdDesc)
{
PPDO_IDENTIFICATION_DESCRIPTION pLHS, pRHS;

    UNREFERENCED_PARAMETER(hChildList);

    pLHS = CONTAINING_RECORD(p1stIdDesc, PDO_IDENTIFICATION_DESCRIPTION, Header);
    pRHS = CONTAINING_RECORD(p2ndIdDesc, PDO_IDENTIFICATION_DESCRIPTION, Header);

    return (pLHS->SerialNo == pRHS->SerialNo) ? TRUE : FALSE;
}

/************************************************************************//**
 * MadBusEvtChildListIdentificationDescriptionCleanup
 *
 * DESCRIPTION:
 *    This function is called to free up any memory resources allocated as
 *    part of the description.  This happens when a child device is unplugged 
 *    or ejected from the bus.
 *   Memory for the description itself will be freed by the framework.
 *    
 * PARAMETERS: 
 *     @param[in]  hDeviceList Handle to the framework's default WDFCHILDLIST
 *     @param[in]  pIdDesc     Description of the child being de-allocated 
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
VOID
#pragma warning(suppress: 6101)
MadBusEvtChildListIdentificationDescriptionCleanup(WDFCHILDLIST hDeviceList,
                                                   PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER pIdDesc)

{
PPDO_IDENTIFICATION_DESCRIPTION pDesc;

    UNREFERENCED_PARAMETER(hDeviceList);

    pDesc = CONTAINING_RECORD(pIdDesc,
                              PDO_IDENTIFICATION_DESCRIPTION,
                              Header);
    if (pDesc->HardwareIds != NULL) 
        {
        ExFreePool(pDesc->HardwareIds);
        pDesc->HardwareIds = NULL;
        }

    return;
}

/************************************************************************//**
 * MadBusEvtDeviceListCreatePdo

  *
 * DESCRIPTION:
 *    This function is called by the framework in response to Query-Device 
 *    relation when a new PDO for a child device needs to be created.
 *    
 * PARAMETERS: 
 *     @param[in]  hDeviceList Handle to the framework's default WDFCHILDLIST
 *     @param[in]  pIdDesc     Description of the child being de-allocated 
 *     @param[in]  pChildInit  pointer to an opaque structure used in 
 *                             collecting device settings and passed in as a 
 *                             parameter to CreateDevice 
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadBusEvtDeviceListCreatePdo(WDFCHILDLIST hDeviceList,
                                      PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER pIDdesc,
                                      PWDFDEVICE_INIT pChildInit)

{
PPDO_IDENTIFICATION_DESCRIPTION pDesc;

    PAGED_CODE();

    pDesc = CONTAINING_RECORD(pIDdesc, PDO_IDENTIFICATION_DESCRIPTION, Header);

    return MadBus_CreatePdo(WdfChildListGetDevice(hDeviceList),
                            pChildInit, pDesc->HardwareIds, pDesc->SerialNo);
}

/************************************************************************//**
 * MadBus_CreatePdo
 *
 * DESCRIPTION:
 *    This function creates and initializes a PDO. 
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice     handle to our device 
 *     @param[in]  pDeviceInit pointer to a framework device init structure
 *     @param[in]  HardwareIDs pointer the hardware id description
 *     @param[in]  SerialNo    Serial number of the pdo to be created
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadBus_CreatePdo(__in WDFDEVICE       hDevice,
                          __in PWDFDEVICE_INIT pDeviceInit,
                          __in PWCHAR          HardwareIDs,
                          __in ULONG           SerialNo)
{
//static WCHAR wcDigits[] = MAD_OBJECTNAME_UNITNUM_WSTR;
//static WCHAR wcNtMadDeviceName[] = MADBUS_NT_DEVICE_NAME_WSTR;
static WCHAR  MadDiskDeviceId[] = MAD_DISK_HARDWARE_ID;
//
DECLARE_CONST_UNICODE_STRING(deviceLocation, L"MadDevice Bus 0");
DECLARE_UNICODE_STRING_SIZE(deviceDescription, MAX_INSTANCE_ID_LEN);
DECLARE_UNICODE_STRING_SIZE(instanceID, MAX_INSTANCE_ID_LEN);
DECLARE_UNICODE_STRING_SIZE(deviceID, MAX_INSTANCE_ID_LEN);
DECLARE_CONST_UNICODE_STRING(deviceLoc1, L"MadBus1");
DECLARE_CONST_UNICODE_STRING(deviceLoc2, L"MadBus2");
//
PFDO_DEVICE_DATA  pFdoData = MadBusFdoGetData(hDevice);
PDRIVER_DATA      pDriverData = pFdoData->pDriverData;
PDRIVER_OBJECT    pDriverObj = pDriverData->pDriverObj;
ULONG             BusNum = pFdoData->SerialNo;
PPDO_DEVICE_DATA              pPdoData = NULL;
WDFDEVICE                     hChild = NULL;
WDF_QUERY_INTERFACE_CONFIG  QueryIntfConfig;
WDF_OBJECT_ATTRIBUTES         pdoAttributes;
WDF_PNPPOWER_EVENT_CALLBACKS  PnpPowerCallbacks;
WDF_DEVICE_PNP_CAPABILITIES   pnpCaps;
WDF_DEVICE_POWER_CAPABILITIES powerCaps;
NTSTATUS NtStatus = STATUS_SUCCESS;

//Which set of Pnp Minor IRPs depends on which devicde type
UCHAR PnpIrpMinrFunxnsGen[]  = MADSIM_PNP_MINORS;
PUCHAR pPnpIrpMinrFunxns     = PnpIrpMinrFunxnsGen;
ULONG  NumMinrFunxns         = sizeof(PnpIrpMinrFunxnsGen);

    PAGED_CODE();

     // Set DeviceType and set of Pnp IRPs to prpocess
    if (BusNum == eGenericBus)
        WdfDeviceInitSetDeviceType(pDeviceInit, MAD_DEVICE_TYPE_GENERIC);
    else
        {
        ASSERT(BusNum == eScsiDiskBus);
        WdfDeviceInitSetDeviceType(pDeviceInit, MAD_DEVICE_TYPE_DISK);
        //pPnpIrpMinrFunxns = PnpIrpMinrFunxnsDisk;
        //NumMinrFunxns     = sizeof(PnpIrpMinrFunxnsDisk);
        }

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "Entering MadBus_CreatePdo... Bus#:Ser#=%d:%d HdwIds=%S NumMinrPnpIrps=%d\n",
                BusNum, SerialNo, HardwareIDs, NumMinrFunxns);

//Name our device object
#if 0 
//Let's name our device object so we can find it in the object data base
//Using Sysinternals:WinObj.exe / OSR:DeviceTree.exe
//When we do this we prevent the device driver from creating an interface_guid for the FDO *!*
    wcNtMadDeviceName[MADBUS_NT_DEVICE_NAME_UNITID_INDX] = wcDigits[SerialNo];
    RtlInitUnicodeString(&NtDeviceName, wcNtMadDeviceName);
    NtStatus = WdfDeviceInitAssignName(pDeviceInit, &NtDeviceName);
    if (!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                    "MadBus_CreatePDO:WdfDeviceInitAssignName returned x%X; the device object will be unnamed\n",
                    NtStatus);
        }
#endif

    // Provide DeviceID, HardwareIDs, CompatibleIDs and InstanceId
    RtlInitUnicodeString(&deviceID, HardwareIDs);
    NtStatus = WdfPdoInitAssignDeviceID(pDeviceInit, &deviceID);
    if (!NT_SUCCESS(NtStatus)) 
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                    "MadBus_CreatePdo:WdfPdoInitAssignDeviceID... ntstatus=x%X\n", 
                    NtStatus);
        MadWriteEventLogMesg(pDriverObj, MADBUS_CREATE_CHILD_DEVICE_ERROR, 1,
                             sizeof(NTSTATUS), (PWSTR)&NtStatus); //Minimal payload
        return NtStatus;
        }

    NtStatus = WdfPdoInitAddHardwareID(pDeviceInit, &deviceID);
    if (!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                    "MadBus_CreatePdo:WdfPdoInitAddHardwareID... ntstatus=x%X\n",
                    NtStatus);
        MadWriteEventLogMesg(pDriverObj, MADBUS_CREATE_CHILD_DEVICE_ERROR, 1,
                             sizeof(NTSTATUS), (PWSTR)&NtStatus); //Minimal payload
        return NtStatus;
        }

    //Set instance ID
    NtStatus =  RtlUnicodeStringPrintf(&instanceID, L"%d", SerialNo);
    if (!NT_SUCCESS(NtStatus)) 
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                    "MadBus_CreatePdo:RtlUnicodeStringPrintf... ntstatus=x%X\n",
                    NtStatus);
        //MadWriteEventLogMesg(pDriverObj, MADBUS_CREATE_CHILD_DEVICE_ERROR, 1,
        //                     sizeof(NTSTATUS), (PWSTR)&NtStatus); //Minimal payload
        //return NtStatus;
        } 

    NtStatus = WdfPdoInitAssignInstanceID(pDeviceInit, &instanceID);
    if (!NT_SUCCESS(NtStatus)) 
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                    "MadBus_CreatePdo:WdfPdoInitAssignInstanceID... ntstatus=x%X\n",
                    NtStatus);
        MadWriteEventLogMesg(pDriverObj, MADBUS_CREATE_CHILD_DEVICE_ERROR, 1,
                             sizeof(NTSTATUS), (PWSTR)&NtStatus); //Minimal payload
        return NtStatus;
        } 

    // Provide a description about the device. This text is usually read from the device.
    // In the case of USB device, this text comes from the string descriptor. 
    // This text is displayed momentarily by the PnP manager while it's looking for a matching INF.
    // If it finds one, it uses the Device Description from the INF file or the friendly name created by
    // coinstallers to display in the device manager. 
    // FriendlyName takes precedence over the DeviceDesc from the INF file.
    if (BusNum == eGenericBus) 
        //On the 1st instance of the bus we install the (generic/memory/storage) device
        NtStatus = RtlUnicodeStringPrintf(&deviceDescription,
                                          L"HTFC_MadDevice%d", SerialNo);
    else 
        //On the 2nd instance of the bus we install the scsi disk
        NtStatus = RtlUnicodeStringPrintf(&deviceDescription,
                                          L"HTFC_MadScsiAdapter%d", SerialNo);
    if (!NT_SUCCESS(NtStatus)) 
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                    "MadBus_CreatePdo:RtlUnicodeStringPrintf... ntstatus=x%X\n",
                    NtStatus);
        MadWriteEventLogMesg(pDriverObj, MADBUS_CREATE_CHILD_DEVICE_ERROR, 1, 
                             sizeof(NTSTATUS), (PWSTR)&NtStatus); //Minimal payload
        return NtStatus;
        } 

     // We can call WdfPdoInitAddDeviceText multiple times, adding device text for multiple locales. 
    // When the system displays the text, it chooses the text that matches the current locale, if available.
    // Otherwise it will use the string for the default locale.
    // The driver can specify the driver's default locale by calling WdfPdoInitSetDefaultLocale.
    if (BusNum == eGenericBus) 
        //On the 1st instance of the bus we install the (generic/memory/storage) device
        NtStatus = WdfPdoInitAddDeviceText(pDeviceInit, &deviceDescription,
                                           &deviceLoc1, MAD_DFLT_LOCALE_ID);
    else
        NtStatus = WdfPdoInitAddDeviceText(pDeviceInit, &deviceDescription,
                                           &deviceLoc2, MAD_DFLT_LOCALE_ID);
    if (!NT_SUCCESS(NtStatus)) 
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                    "MadBus_CreatePdo:WdfPdoInitAddDeviceText... ntstatus=x%X - continuing\n",
                    NtStatus);
        NtStatus = STATUS_SUCCESS;
        } 
    WdfPdoInitSetDefaultLocale(pDeviceInit, MAD_DFLT_LOCALE_ID);

    // Register PNP & power callbacks.
    // 1st - initialize the PnpPowerCallbacks structure.  
    MadBusPdoSetPnpPowerCallbacks(&PnpPowerCallbacks);
    WdfDeviceInitSetPnpPowerEventCallbacks(pDeviceInit, &PnpPowerCallbacks);

#if 0
    NtStatus = 
    WdfDeviceInitAssignWdmIrpPreprocessCallback(pDeviceInit,
                                                MadBusPdoEvtWdmIrpPreprocess,
                                                IRP_MJ_SCSI, //IRP_MJ_INTERNAL_DEVICE_CONTROL,
                                                NULL, 0); //No list - we'll get everything
    if (NtStatus == STATUS_SUCCESS)
#endif
        NtStatus = 
        WdfDeviceInitAssignWdmIrpPreprocessCallback(pDeviceInit,
                                                    MadBusPdoEvtWdmIrpPreprocess,
                                                    IRP_MJ_PNP, 
                                                    pPnpIrpMinrFunxns, //List of minor funxn codes
                                                    NumMinrFunxns);
   if (!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                    "MadBus_CreatePdo:WdfDeviceInitAssignWdmIrpPreprocessCallback... ntstatus=x%X\n",
                    NtStatus);
        MadWriteEventLogMesg(pDriverObj, MADBUS_CREATE_CHILD_DEVICE_ERROR, 1,
                             sizeof(NTSTATUS), (PWSTR)&NtStatus); //Minimal payload
        return NtStatus;
        } 

    // Initialize the attributes to specify the size of PDO device extension.
    // All the state information private to the PDO will be tracked here.
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&pdoAttributes, PDO_DEVICE_DATA);

    NtStatus = WdfDeviceCreate(&pDeviceInit, &pdoAttributes, &hChild);
    if (!NT_SUCCESS(NtStatus)) 
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                    "MadBus_CreatePdo:WdfDeviceCreate... ntstatus=x%X\n",
                    NtStatus);
        MadWriteEventLogMesg(pDriverObj, MADBUS_CREATE_CHILD_DEVICE_ERROR, 1,
                             sizeof(NTSTATUS), (PWSTR)&NtStatus); //Minimal payload
        return NtStatus;
        } 

    // Get and set the device contexts.
    pFdoData = MadBusFdoGetData(hDevice);
    pPdoData = MadBusPdoGetData(hChild);
    RtlZeroMemory(pPdoData, sizeof(PDO_DEVICE_DATA));
    //
    pPdoData->SerialNo = SerialNo;
    pPdoData->hDevice  = hChild;
    pPdoData->pFdoData = pFdoData;

#ifdef _MAD_SIMULATION_MODE_  
    pPdoData->pMadSimSpIoFunxns = 
    &pPdoData->pFdoData->pDriverData->MadSimSpIoFunxns;
#endif

    pFdoData->CurrNumPDOs++;
    pFdoData->pDevObjs[SerialNo] = WdfDeviceWdmGetDeviceObject(hChild);

    //Record Device Object IDs for searching/retrieving  
    pFdoData->PdoDevOidsList[SerialNo].pPhysDevObj = WdfDeviceWdmGetDeviceObject(hChild);
    pFdoData->PdoDevOidsList[SerialNo].hPhysDevice = hChild;

    //Retrieve Config space data from the working copy of this bus instance
    RtlCopyMemory(&pPdoData->BusPdoCnfgData,
                  &pFdoData->BusInstanceConfigData, MADBUS_DEV_CNFG_SIZE);

    // Set Pmp & Power Mngt properties for the child device.
    MadBusPdoInitPnpCapabilities(&pnpCaps, SerialNo);
    WdfDeviceSetPnpCapabilities(hChild, &pnpCaps);

    MadBusPdoInitPowerCapabilities(&powerCaps);
    WdfDeviceSetPowerCapabilities(hChild, &powerCaps);

    //Here we setup some real-word-useful query_interfaces so that
    //PnpMngr/other windows(inbox) drivers can query (IRP_MN_QUERY_INTERFACE) & use these callbacks directly.
    //Setup the standard bus interface structure for query_interface 
    MadBusPdoInitBusInterface(pPdoData);
    WDF_QUERY_INTERFACE_CONFIG_INIT(&QueryIntfConfig,
                                    (PINTERFACE)&pPdoData->BusInterfaceStd,
                                    &GUID_BUS_INTERFACE_STANDARD,
                                    MadBusPdoEvtProcessQueryInterfaceRequest);
    NtStatus = WdfDeviceAddQueryInterface(hChild, &QueryIntfConfig);
    if (!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                    "MadsBus_CreatePdo:WdfDeviceAddQueryInterface... ntstatus=x%X\n",
                    NtStatus);
        } //Not a show-stopper

    //If we have more interfaces, we can set up again & call WdfDeviceAddQueryInterface again for each add'l interface.
    //Setup the standard pnp location interface structure for query_interface 
    MadBusPdoInitPnpLocInterface(pPdoData);
    WDF_QUERY_INTERFACE_CONFIG_INIT(&QueryIntfConfig,
                                    (PINTERFACE)&pPdoData->PnpLocInterface,
                                    &GUID_PNP_LOCATION_INTERFACE, 
                                    MadBusPdoEvtProcessQueryInterfaceRequest);
    NtStatus = WdfDeviceAddQueryInterface(hChild, &QueryIntfConfig);
    if (!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                    "MadBus_CreatePdo:WdfDeviceAddQueryInterface... ntstatus=x%X\n",
                    NtStatus);
        } //Not a show-stopper

    MadBusPdoInitPnpBusInfo(pPdoData);
    MadBusPdoInitReEnumSelf(pPdoData);
    MadBusPdoInitIoResrcs(pPdoData);
    MadBusPdoInitPnpXtendedAddr(pPdoData);
    MadBusPdoInitInterface(pPdoData, 
                           &pPdoData->DfltInterface, sizeof(INTERFACE));
 
    //Override Config space data w/ device-specific data (addresses etc.) 
    MadBusPdoSetPciDeviceAddresses(pPdoData);

    if (BusNum == eScsiDiskBus) 
        { //This will be a storport:miniport disk - override the bus-wide defaults
        //pPdoData->BusPdoCnfgData.LegacyPci.DeviceID = MAD_DEVICE_ID_DISK; //set in PciVndrData
        MadSubSysId_U Rssidu;
        Rssidu.BitFields.Features = 0; //It looks like Features should be named Limitations
                                       // See CheckMadSubSysID in MadDiskMP.c
        Rssidu.BitFields.ConfigType = 5;
        Rssidu.BitFields.AdapterType = 1;
        Rssidu.BitFields.SubType = 0;
        Rssidu.BitFields.ExcludeFlag = 0;
        pPdoData->BusPdoCnfgData.LegacyPci.u.type0.SubSystemID = Rssidu.ssid;
        }

    //Vendor-specific data
    #ifdef MAD_KERNEL_WONT_CREATE_DMA_ADAPTER
    pPdoData->pMadSimDmaFunxns = 
    &pPdoData->pFdoData->pDriverData->MadSimDmaFunxns; 
    #endif
    MadBusPdoSetPciVendorData(pPdoData, SerialNo);

    //Assign CM_RESOURCE_LIST parameters to be passed up to the functional (device) driver
    pPdoData->Irql        = pFdoData->OurIRQL; 
    pPdoData->IDTindx     = pFdoData->OurVector; 

    //Processor affinity for this device is constrained to one specific processor = Serial# ... or no constraint
    pPdoData->IntAffinity = 
    pDriverData->bAffinityOn ? (0x01<<SerialNo) : 0xFFFFFFFF;
    MadBusPdoSetDeviceAddresses(pPdoData);

    //Initialize device data
    ASSERTMSG("Uninitialized device data pointer!", 
              (pPdoData->pDeviceData != NULL));
    RtlFillMemory(pPdoData->pDeviceData, pDriverData->MadDataXtent, 0x00);

    //TBD: Anything saved to disk to be retrieved maybe ?
    //Let the user do it in the UI

#if 0
    //Initialize our interrupt thread synchronization events 
    KeInitializeEvent(&pPdoData->evIntThreadExit, NotificationEvent, FALSE);
    KeInitializeEvent(&pPdoData->MadSimIntParms.evDevPowerUp, NotificationEvent, FALSE);
    KeInitializeEvent(&pPdoData->MadSimIntParms.evDevPowerDown, NotificationEvent, FALSE);
    pPdoData->MadSimIntParms.pEvIntThreadExit = &pPdoData->evIntThreadExit;

    if (BusNum == eGenericBus)
        NtStatus = PsCreateSystemThread(&pPdoData->hDevIntThread,
                                        (SYNCHRONIZE | GENERIC_EXECUTE),
                                        NULL, NULL, NULL,
                                        MadPdoIntThread, pPdoData);
    else
        {
        pPdoData->MadSimIntParms.u.StorIntParms.pSetPowerUpEvFunxn   = 
                                   (PFN_BUS_SET_POWER_UP)&MadBusPdoSetPowerUp;
        pPdoData->MadSimIntParms.u.StorIntParms.pSetPowerDownEvFunxn = 
                                   (PFN_BUS_SET_POWER_DOWN)&MadBusPdoSetPowerDown;
        NtStatus = PsCreateSystemThread(&pPdoData->hDevIntThread,
                                        (SYNCHRONIZE | GENERIC_EXECUTE),
                                        NULL, NULL, NULL,
                                        MadPdoScsiIntThread, pPdoData);
        }
 
    if (!NT_SUCCESS(NtStatus)) 
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                    "MadBus_CreatePdo:PsCreateSystemThread returned... ntstatus=x%X\n",
                    NtStatus);
        MadWriteEventLogMesg(pDriverObj, MADBUS_CREATE_CHILD_DEVICE_ERROR, 1,
                             sizeof(NTSTATUS), (PWSTR)&NtStatus); //Minimal payload
        return NtStatus;
        }
#endif
    NtStatus = MadBusPdoInitDeviceThreadParms(pPdoData);
    if (!NT_SUCCESS(NtStatus))
        return NtStatus;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadBus_CreatePdo...normal exit; Bus#:Ser#=%d:%d\n",
                BusNum, SerialNo);

    return NtStatus;
}
 
/************************************************************************//**
 * MadBusPdoEvtPrepareHardware
 *
 * DESCRIPTION:
 *    This event callback function performs operations that are necessary to
 *    make the driver's device operational. The framework calls the driver's 
 *    EvtDevicePrepareHardware callback when the PnP manager sends an
 *    IRP_MN_START_DEVICE request to the driver stack.
 *
 *   Specifically, most drivers will use this callback to map resources.  USB
 *   drivers may use it to get device descriptors, config descriptors and to
 *   select configs.
 *
 *   Some drivers may choose to download firmware to a device in this callback,
 *   but that is usually only a good choice if the device firmware won't be
 *   destroyed by a D0 to D3 transition.  If firmware will be gone after D3,
 *   then firmware downloads should be done in EvtDeviceD0Entry, not here.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice     handle to our device 
 *     @param[in]  hResrcsRaw  handle to a collection of framework resource
 *                             objects.  This collection identifies the raw
 *                             (bus-relative) hardware resources that have been
 *                             assigned to the device. 
 *     @param[in] hResrcsXlatd  handle to a collection of framework resource
 *                              objects.  This collection identifies the 
 *                              translated (system-physical) hardware resources 
 *                              that have been assigned to the device.
 *                              The resources appear from the CPU's point of view.
 *                              Use this list of resources to map I/O space and
 *                              device-accessible memory into virtual address space
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadBusPdoEvtPrepareHardware(WDFDEVICE      hDevice,
                                     WDFCMRESLIST   hResrcsRaw,
                                     WDFCMRESLIST   hResrcsXlatd)
{
PPDO_DEVICE_DATA                pPdoData = MadBusPdoGetData(hDevice);
ULONG                           Count = WdfCmResourceListGetCount(hResrcsRaw);
PCM_PARTIAL_RESOURCE_DESCRIPTOR pPartlResrcDescRaw   =
                                WdfCmResourceListGetDescriptor(hResrcsRaw, 0);
PCM_PARTIAL_RESOURCE_DESCRIPTOR pPartlResrcDescXlatd = 
                                WdfCmResourceListGetDescriptor(hResrcsXlatd, 0);
//LONG ResrcCount = WdfCmResourceListGetCount(hResrcsXlatd);
NTSTATUS NtStatus = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(hDevice);
    UNREFERENCED_PARAMETER(pPartlResrcDescXlatd);
    UNREFERENCED_PARAMETER(pPartlResrcDescRaw);
    
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadBusPdoEvtPrepareHardware: Bus#:Ser#=%d:%d ListCount=%d pPartlResrcDescRaw[0]=%p pPartlResrcDescXlatd[0]=%p\n",
                pPdoData->pFdoData->SerialNo, pPdoData->SerialNo, Count,
                pPartlResrcDescRaw, pPartlResrcDescXlatd);
    
    return NtStatus;
}

/************************************************************************//**
 * MadBusPdoEvtD0Entry
 *
 * DESCRIPTION:
 *    This function is the framework callback called when the pdo goes into
 *    device power-state zero.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice        handle to our device 
 *     @param[in]  PrevPowerState the power state thedevice is coming from
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadBusPdoEvtD0Entry(IN WDFDEVICE hDevice, 
                             IN WDF_POWER_DEVICE_STATE PrevPowerState)

{
    PPDO_DEVICE_DATA   pPdoData = MadBusPdoGetData(hDevice);
    UNREFERENCED_PARAMETER(PrevPowerState);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadBusPdoEvtD0Entry... Bus#:Seri#=%d:%d coming from PowerState %d\n",
                pPdoData->pFdoData->SerialNo, pPdoData->SerialNo, PrevPowerState);

    return STATUS_SUCCESS;
}

/************************************************************************//**
 * MadBusPdoEvtD0Exit
 *
 * DESCRIPTION:
 *    This function is the framework callback called when the pdo goes into
 *    a lower power-state.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice     handle to our device (PDO)
 *     @param[in]  PowerState  power state the pdo is going into 
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadBusPdoEvtD0Exit(IN WDFDEVICE hDevice, 
                            IN WDF_POWER_DEVICE_STATE PowerState)
{
PPDO_DEVICE_DATA   pPdoData = MadBusPdoGetData(hDevice);
PFDO_DEVICE_DATA   pFdoData = pPdoData->pFdoData;
NTSTATUS  NtStatus = STATUS_SUCCESS;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadBusPdoEvtD0Exit... Bus#:Seri#=%d:%d going to PowerState %d\n",
                pFdoData->SerialNo, pPdoData->SerialNo, PowerState);

    if (PowerState == WdfPowerDeviceD3Final) //Our device is going to the morgue
        //Don't do this for the Scsi disk because Storport wants to play god and bounce the power state right away
        if (pFdoData->SerialNo != eScsiDiskBus)
            {
            WDFDEVICE  hParentDev = WdfPdoGetParent(hDevice);
            NtStatus = MadBus_UnPlugDevice(hParentDev, pPdoData->SerialNo);
            }

    return NtStatus;
}

/************************************************************************//**
 * MadBusPdoEvtWdmIrpPreprocess
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
NTSTATUS MadBusPdoEvtWdmIrpPreprocess(WDFDEVICE hDevice, PIRP pIRP)
{
PPDO_DEVICE_DATA   pPdoData    = MadBusPdoGetData(hDevice);
PIO_STACK_LOCATION pIoStackLoc = IoGetCurrentIrpStackLocation(pIRP);
UCHAR              MajrFunxn   = pIoStackLoc->MajorFunction;
UCHAR              MinrFunxn   = pIoStackLoc->MinorFunction;
GUID               InterfaceType;
PPNP_BUS_INFORMATION pPnpBusInfo;
BOOLEAN            bKnownIntf = TRUE;
size_t             AllocSize = 0;
NTSTATUS NtStatus = STATUS_PENDING;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadBusPdoEvtWdmIrpPreprocess... Bus#:Ser#=%d:%d Major:Minor=x%X:x%X\n", 
                pPdoData->pFdoData->SerialNo, pPdoData->SerialNo,
                MajrFunxn, MinrFunxn);

    pIRP->IoStatus.Status = STATUS_NOT_SUPPORTED; //Make DV happy
    switch (MajrFunxn)
        {
        case IRP_MJ_PNP:
            switch (MinrFunxn)
                {
                case IRP_MN_START_DEVICE:
                    MadBusPdo_PlugCmResrcsIntoFdoStackLoc(pPdoData, 
                                                          pIRP, pIoStackLoc); 
                    pIRP->IoStatus.Status = STATUS_SUCCESS;
                    break;

                case IRP_MN_READ_CONFIG:
                    //Verify that we have the event object for the thread
                    //ASSERT(pPdoData->pIntThreadObj != NULL); 
                    RtlCopyMemory(pIoStackLoc->Parameters.ReadWriteConfig.Buffer, 
                                  &pPdoData->BusPdoCnfgData, 
                                  sizeof(MADBUS_DEVICE_CONFIG_DATA)); 
                    pIRP->IoStatus.Status = STATUS_SUCCESS;
                    break;

                case IRP_MN_QUERY_INTERFACE:
                    RtlCopyMemory(&InterfaceType, 
                                  pIoStackLoc->Parameters.QueryInterface.InterfaceType,
                                  sizeof(GUID));
                    bKnownIntf = MadBusPdo_ProcessQueryInterface(pPdoData, 
                                                                 pIoStackLoc, 
                                                                 &InterfaceType);
                    if (bKnownIntf)
                        {
                        pIRP->IoStatus.Information = 
                        (ULONG_PTR)pIoStackLoc->Parameters.QueryInterface.Interface;
                        pIRP->IoStatus.Status = STATUS_SUCCESS;
                        }
                    break;

                case IRP_MN_QUERY_BUS_INFORMATION:
                case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:
                    {
                    //PnpMngr wants this data in a pool & will delete the pool for us
                    pPnpBusInfo =
                    (PPNP_BUS_INFORMATION)ExAllocatePoolWithTag(PagedPool,
                                                                sizeof(PNP_BUS_INFORMATION),
                                                                MADBUS_POOL_TAG);
                    if (pPnpBusInfo == NULL)
                        {
                        pIRP->IoStatus.Information = (ULONG_PTR)NULL;
                        pIRP->IoStatus.Status      = STATUS_NO_MEMORY;
                        break;
                        }

                    RtlCopyMemory(pPnpBusInfo, &pPdoData->PnpBusInfo, 
                                  sizeof(PNP_BUS_INFORMATION));
                    pIRP->IoStatus.Information = (ULONG_PTR)pPnpBusInfo;
                    pIRP->IoStatus.Status = STATUS_SUCCESS;
                    }
                    break;

                case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
                    //pIRP->IoStatus.Information = (ULONG_PTR)NULL;
                    // pIRP->IoStatus.Status = STATUS_SUCCESS;
                    //DBG_UNREFERENCED_LOCAL_VARIABLE(AllocSize);
                    //break;
//#if 0
                    {
                    AllocSize = sizeof(MAD_IO_RESRC_REQ_LIST);
                    PIO_RESOURCE_REQUIREMENTS_LIST pIoResrcReqList =
                    (PIO_RESOURCE_REQUIREMENTS_LIST)ExAllocatePoolWithTag(PagedPool,
                                                                          AllocSize,
                                                                          MADBUS_POOL_TAG);
                    if (pIoResrcReqList == NULL)
                        {
                        pIRP->IoStatus.Information = (ULONG_PTR)NULL;
                        pIRP->IoStatus.Status = STATUS_NO_MEMORY;
                        break;
                        }

                    RtlCopyMemory(pIoResrcReqList,
                                  &pPdoData->MadIoResrcReqList, //The static list details in the PDO
                                  sizeof(MAD_IO_RESRC_REQ_LIST));

                    //Pnp Mngr will free the pool 
                    pIRP->IoStatus.Information = (ULONG_PTR)pIoResrcReqList;
                    pIRP->IoStatus.Status = STATUS_SUCCESS;
                    }
                    break;
//#endif
                case IRP_MN_QUERY_RESOURCES:
                    //pIRP->IoStatus.Information = (ULONG_PTR)NULL;
                    //pIRP->IoStatus.Status = STATUS_SUCCESS;
                    //DBG_UNREFERENCED_LOCAL_VARIABLE(AllocSize);
                    //break;
//#if 0
                    {
                    AllocSize = sizeof(MAD_IO_RESRC_LIST);
                    PCM_RESOURCE_LIST pCmResrcList =
                    (PCM_RESOURCE_LIST)ExAllocatePoolWithTag(PagedPool,
                                                             AllocSize,
                                                             MADBUS_POOL_TAG);
                    if (pCmResrcList == NULL)
                        {
                        pIRP->IoStatus.Information = (ULONG_PTR)NULL;
                        pIRP->IoStatus.Status = STATUS_NO_MEMORY;
                        break;
                        }

                    pCmResrcList->Count = MAD_NUM_RESRC_LISTS;
                    RtlCopyMemory(pCmResrcList->List,
                                  //The static list details in the PDO
                                  pPdoData->MadIoResrcReqList.MadIoResrcList[0].Descriptors,
                                  (sizeof(IO_RESOURCE_DESCRIPTOR) * MADDEV_TOTAL_CM_RESOURCES));

                    //Pnp Mngr will free the pool 
                    pIRP->IoStatus.Information = (ULONG_PTR)pCmResrcList;
                    pIRP->IoStatus.Status = STATUS_SUCCESS;
                    break; 
                    }
//#endif
                case MAD_PNP_MINOR_DV_INVALID:
                    pIRP->IoStatus.Information = (ULONG_PTR)NULL;
                    break;

                default:
                    GENERIC_SWITCH_DEFAULTCASE_ASSERT;
                }
                break;

        case IRP_MJ_SCSI: // == case IRP_MJ_INTERNAL_DEVICE_CONTROL:
            MadBusPdo_ProcessDeviceControlIoctl(pPdoData, pIRP, pIoStackLoc);
            break;

        default:
            pIRP->IoStatus.Information = (ULONG_PTR)NULL;
            GENERIC_SWITCH_DEFAULTCASE_ASSERT;
        };

    TraceEvents(TRACE_LEVEL_VERBOSE, MYDRIVER_ALL_INFO,
                "MadBusPdoEvtWdmIrpPreprocess... Bus#:Ser#=%d:%d Info=%p IoStatus=x%X NtStatus=x%X\n",
                pPdoData->pFdoData->SerialNo, pPdoData->SerialNo,
                pIRP->IoStatus.Information, pIRP->IoStatus.Status, NtStatus);

    IoSkipCurrentIrpStackLocation(pIRP);
    NtStatus = WdfDeviceWdmDispatchPreprocessedIrp(hDevice, pIRP);
    return NtStatus;
}

//Process (Internal) Device Control (Storage) Ioctls
void MadBusPdo_ProcessDeviceControlIoctl(PPDO_DEVICE_DATA pPdoData, PIRP pIRP,
                                         IN PIO_STACK_LOCATION pIoStackLoc)
{
ULONG IoctlCode = pIoStackLoc->Parameters.DeviceIoControl.IoControlCode;
USHORT DevType  = (USHORT)(IoctlCode >> 16);
UCHAR  Access   = (UCHAR)((IoctlCode & 0x0000FFFF) >> 14);
USHORT Funxn    = (USHORT)((IoctlCode & 0x0000FFFF) >> 2);
UCHAR  XferType = (UCHAR)(IoctlCode & 0x00000003);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadBusPdo_ProcessDeviceControlIoctl... Bus#:Ser#=%d:%d pIRP=%p pIoStackLoc=%p\n",
                pPdoData->pFdoData->SerialNo, pPdoData->SerialNo, 
                pIRP, pIoStackLoc);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadBusPdo_ProcessDeviceControlIoctl... BusNum=%d SerialNo=%d Ioctl=x%X {x%4X:%2X:%4X:%2X}\n",
                pPdoData->pFdoData->SerialNo, pPdoData->SerialNo,
                IoctlCode, DevType, Access, Funxn, XferType);

    switch (IoctlCode)
        {
        case IOCTL_DISK_SET_DRIVE_LAYOUT:
            #if 0
            {
            PDRIVE_LAYOUT_INFORMATION_EX  pDrvFormatInfo =
            (PDRIVE_LAYOUT_INFORMATION_EX)pIRP->AssociatedIrp.SystemBuffer;
            TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                        "MadBusPdo_ProcessDeviceControlIoctl... SerialNo=%d SetDrvLayout: Style=%ld Count=%ld\n",
                        pPdoData->SerialNo,
                        pDrvFormatInfo->PartitionStyle, pDrvFormatInfo->PartitionCount);

            pIoStackLoc->Parameters.DeviceIoControl.OutputBufferLength = 
                        sizeof(DRIVE_LAYOUT_INFORMATION_EX);
            pIRP->IoStatus.Information = (ULONG_PTR)pDrvFormatInfo;
            pIRP->IoStatus.Status = STATUS_SUCCESS;
            return;
            } 
            #endif

        case IOCTL_STORAGE_LOAD_MEDIA:
        case IOCTL_STORAGE_LOAD_MEDIA2:
            TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                        "MadBusPdo_ProcessDeviceControlIoctl... Bus#:Ser#=%d:%d Load_Media\n",
                        pPdoData->pFdoData->SerialNo, pPdoData->SerialNo);
            pIRP->IoStatus.Information = (ULONG_PTR)NULL;
            pIRP->IoStatus.Status = STATUS_SUCCESS;
            break;

        case IOCTL_STORAGE_MEDIA_REMOVAL:
        case IOCTL_STORAGE_EJECT_MEDIA:
        case IOCTL_STORAGE_RESET_BUS:
        case IOCTL_STORAGE_RESET_DEVICE:   
        case IOCTL_STORAGE_GET_DEVICE_NUMBER:
        case IOCTL_STORAGE_GET_DEVICE_NUMBER_EX:
        case IOCTL_SCSI_GET_CAPABILITIES:
        case IOCTL_SCSI_MINIPORT:
            pIRP->IoStatus.Information = (ULONG_PTR)NULL;
            pIRP->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
            break;

        default:
            pIRP->IoStatus.Information = (ULONG_PTR)NULL;
            pIRP->IoStatus.Status = STATUS_NOT_SUPPORTED;
            GENERIC_SWITCH_DEFAULTCASE_ASSERT;
        };
}

/************************************************************************//**
 * MadBusPdo_PlugCmResrcsIntoFdoStackLoc
 *
 * DESCRIPTION:
 *    This function plugs the device config-mngt parameters into the 
 *    functional (device) driver's IoStack location in the start-device irp so
 *    that the device driver can retrieve them in its PrepareHardware callback.
 *    This function replaces what Pnp-Mngr can't/won't do for us.
 *    
 * PARAMETERS: 
 *     @param[in]  pPdoData    pointer to the framework device extension.
 *     @param[in]  pIRP        pointer to the request IRP
 *     @param[in]  pIoStackLoc pointer to the Io-Stack location for this IRP 
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
void MadBusPdo_PlugCmResrcsIntoFdoStackLoc(PPDO_DEVICE_DATA pPdoData, PIRP pIRP, 
                                           IN PIO_STACK_LOCATION pIoStackLoc) 
{
register ULONG j;
PCM_RESOURCE_LIST pResrcList      = (PCM_RESOURCE_LIST)&pPdoData->ResrcList;
PCM_RESOURCE_LIST pResrcListXlatd = (PCM_RESOURCE_LIST)&pPdoData->ResrcListXlatd;
PCM_PARTIAL_RESOURCE_DESCRIPTOR pPartlResrcDesc =
                                &(pResrcList->List[0].PartialResourceList.PartialDescriptors[0]);
PCM_PARTIAL_RESOURCE_DESCRIPTOR pPartlResrcDescXlatd =
                                &(pResrcListXlatd->List[0].PartialResourceList.PartialDescriptors[0]);

//*** Establish a pointer to upper (previous) stack location (Functional Driver)
PIO_STACK_LOCATION pFdoStackLoc; 
PIO_STACK_LOCATION pTempStackLoc = pIoStackLoc; 

    //Step up the iostack 1+NumFilters times
    for (j = 0; j <= pPdoData->pFdoData->pDriverData->NumFilters; j++) 
        pTempStackLoc = GET_PREV_IOSTACK_LOCATION(pTempStackLoc); 
    pFdoStackLoc = pTempStackLoc;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadBusPdo_PlugCmResrcsIntoFdoStackLoc... Bus#:Ser#=%d:%d pIRP pIoStackLoc PrevStackLoc=%p,%p,%p\n",
                pPdoData->pFdoData->SerialNo, pPdoData->SerialNo, 
                pIRP, pIoStackLoc, pFdoStackLoc);

    pResrcList->Count      = MAD_NUM_RESRC_LISTS; 
    pResrcListXlatd->Count = MAD_NUM_RESRC_LISTS;
    pResrcList->List[0].PartialResourceList.Count      = MADDEV_TOTAL_CM_RESOURCES;
    pResrcListXlatd->List[0].PartialResourceList.Count = MADDEV_TOTAL_CM_RESOURCES;

//*** Assign device memory address
    pPartlResrcDesc->Type               = CmResourceTypePort;
    pPartlResrcDescXlatd->Type          = CmResourceTypePort;
    pPartlResrcDescXlatd->u.Port.Length = MAD_REGISTER_BLOCK_SIZE;   
    pPartlResrcDescXlatd->u.Port.Start  = pPdoData->liDevBase;
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadBusPdo_PlugCmResrcsIntoFdoStackLoc... Bus#:Ser#=%d:%d  Portmem_Start_Hi:Lo_Len=x%X:%X %d\n",
                pPdoData->pFdoData->SerialNo, pPdoData->SerialNo,
                pPartlResrcDescXlatd->u.Port.Start.HighPart, 
                pPartlResrcDescXlatd->u.Port.Start.LowPart,
                pPartlResrcDescXlatd->u.Port.Length);

//*** Point to next paired descriptors
    pPartlResrcDesc++; 
    pPartlResrcDescXlatd++; 

//*** Assign device interrupt parms 
    pPartlResrcDesc->Type                      = CmResourceTypeInterrupt;
    pPartlResrcDescXlatd->Type                 = CmResourceTypeInterrupt;
    pPartlResrcDescXlatd->u.Interrupt.Level    = pPdoData->Irql;   
    pPartlResrcDescXlatd->u.Interrupt.Vector   = pPdoData->IDTindx;  
    pPartlResrcDescXlatd->u.Interrupt.Affinity = pPdoData->IntAffinity;  
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                 "MadBusPdo_PlugCmResrcsIntoFdoStackLoc... Bus#:Ser#=%d:%d  Interrupt:Lvl:Vector:Affin=%d:%d:x%X\n",
                 pPdoData->pFdoData->SerialNo, pPdoData->SerialNo,
                 pPartlResrcDescXlatd->u.Interrupt.Level,
                 pPartlResrcDescXlatd->u.Interrupt.Vector,
                 pPartlResrcDescXlatd->u.Interrupt.Affinity);

//*** Point to next paired descriptors
    pPartlResrcDesc++; 
    pPartlResrcDescXlatd++; 

//*** Assign static (Non-DMA) buffer addrs.
//*** Device read buffer address
    pPartlResrcDesc->Type                 = CmResourceTypeMemory;
    pPartlResrcDescXlatd->Type            = CmResourceTypeMemory;
    pPartlResrcDescXlatd->u.Memory.Length = MAD_MAPD_READ_SIZE;  
    pPartlResrcDescXlatd->u.Memory.Start  = pPdoData->liDevPioRead;
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadBusPdo_PlugCmResrcsIntoFdoStackLoc... Bus#:Ser#=%d:%d  Memory1_Start_Hi:Lo_Len=x%X:%X %d\n",
                pPdoData->pFdoData->SerialNo, pPdoData->SerialNo,
                pPartlResrcDescXlatd->u.Memory.Start.HighPart,
                pPartlResrcDescXlatd->u.Memory.Start.LowPart,
                pPartlResrcDescXlatd->u.Memory.Length);

//*** Point to next paired descriptors
    pPartlResrcDesc++; 
    pPartlResrcDescXlatd++; 

//*** Device write buffer address
    pPartlResrcDesc->Type                 = CmResourceTypeMemory;
    pPartlResrcDescXlatd->Type            = CmResourceTypeMemory;
    pPartlResrcDescXlatd->u.Memory.Length = MAD_MAPD_WRITE_SIZE;  
    pPartlResrcDescXlatd->u.Memory.Start  = pPdoData->liDevPioWrite;
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadBusPdo_PlugCmResrcsIntoFdoStackLoc... Bus#:Ser#=%d:%d  Memory2_Start_Hi:Lo_Len=x%X:%X %d\n",
                pPdoData->pFdoData->SerialNo, pPdoData->SerialNo,
                pPartlResrcDescXlatd->u.Memory.Start.HighPart,
                pPartlResrcDescXlatd->u.Memory.Start.LowPart,
                pPartlResrcDescXlatd->u.Memory.Length);

//* Update the functional (device) driver's stack location parameters as would PnP Mngr
//* Non-null address values for the resource lists indicate that there is resource information
    pFdoStackLoc->Parameters.StartDevice.AllocatedResources           = pResrcList;
    pFdoStackLoc->Parameters.StartDevice.AllocatedResourcesTranslated = pResrcListXlatd;

    return;
}

BOOLEAN MadBusPdo_ProcessQueryInterface(PPDO_DEVICE_DATA pPdoData, 
                                       PIO_STACK_LOCATION pIoStackLoc, 
                                       PGUID pInterfaceType)
{
GUID guid = *pInterfaceType;
PDO_QUERY_INTF_TYPE eIntfQ = 
                    SetEnumIntfType(pInterfaceType, 
                                    pPdoData->pFdoData->pDriverData->QueryIntfGuids);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadBusPdo_ProcessQueryInterface... IRP_MJ_PNP:IRP_MN_QUERY_INTERFACE Size=%d Version=%d eIntfQ=%d\n",
                pIoStackLoc->Parameters.QueryInterface.Size,
                pIoStackLoc->Parameters.QueryInterface.Version, eIntfQ);
    if (eIntfQ == eUndefined)
        Trace_Guid(guid);

    NTSTATUS NtStatus = 
    MadBusPdoProcessQueryInterfaceRequest(pPdoData, eIntfQ,
                                          pIoStackLoc->Parameters.QueryInterface.Interface,
                                          pIoStackLoc->Parameters.QueryInterface.InterfaceSpecificData);
    if (NtStatus != STATUS_SUCCESS)
        TraceEvents(TRACE_LEVEL_ERROR, MYDRIVER_ALL_INFO,
                    "MadBusPdo_ProcessQueryInterface... Bus#:Serial#=%d:%d eIntfQuery=%d status=x%X\n",
                    pPdoData->pFdoData->SerialNo, pPdoData->SerialNo, 
                    eIntfQ, NtStatus);

    return (NtStatus == STATUS_SUCCESS);

#if 0
    size_t EqualLen = RtlCompareMemory(&GUID_BUS_INTERFACE_STANDARD, 
                                       pInterfaceType, (ULONG)sizeof(GUID));
    if (EqualLen == sizeof(GUID)) //We recognize this one so we do our business through driver-framework qi config above
        {
        ASSERT(pIoStackLoc->Parameters.QueryInterface.Size >= sizeof(BUS_INTERFACE_STANDARD));
        //RtlCopyMemory(pInterface, &pPdoData->BusInterfaceStd, sizeof(BUS_INTERFACE_STANDARD));
        return TRUE;
        }

    EqualLen = RtlCompareMemory(&GUID_PNP_LOCATION_INTERFACE, 
                                pInterfaceType, (ULONG)sizeof(GUID));
    if (EqualLen == sizeof(GUID)) //We recognize this one so we do our business through driver-framework qi config above
        {
        ASSERT(pIoStackLoc->Parameters.QueryInterface.Size >= sizeof(PNP_LOCATION_INTERFACE));
        return TRUE;
        }

    EqualLen = RtlCompareMemory(&GUID_REENUMERATE_SELF_INTERFACE_STANDARD, 
                                pInterfaceType, (ULONG)sizeof(GUID));
    if (EqualLen == sizeof(GUID)) //We recognize this one as well
        return TRUE; //Nothing to do

    //Report any unknown we haven't filtered out
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadBusPdoEvtWdmIrpPreprocess:QueryInterface... unrecognized query_interface guid!\n");
    //
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "IntfType=0x%08X-%04X-%04X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n",
                pInterfaceType->Data1, pInterfaceType->Data2, pInterfaceType->Data3,
                pInterfaceType->Data4[0], pInterfaceType->Data4[1], 
                pInterfaceType->Data4[2], pInterfaceType->Data4[3],
                pInterfaceType->Data4[4], pInterfaceType->Data4[5], 
                pInterfaceType->Data4[6], pInterfaceType->Data4[7]);

    //A bogus guid sent by Driver-Verifier
    EqualLen = RtlCompareMemory(&GUID_NULL, pInterfaceType, sizeof(GUID));
    if (EqualLen != sizeof(GUID))
        {
        //This may be better than nothing
        //DfltInterface.Context = pPdoData;
        //RtlCopyMemory(pIoStackLoc->Parameters.QueryInterface.Interface, &DfltInterface, sizeof(INTERFACE));
        }
#endif
    //return FALSE;
}

/************************************************************************//**
 * MadBusPdoXlateBusAddr
 *
 * DESCRIPTION:
 *    This function returns a dma adapter.
 *    It is a DDI Query interface function
 *
 * PARAMETERS:
 *     @param[in]  Context         the PDO Context
 *     @param[in]  BusAddress  the bus-relative logical address to translate
 *     @param[in]  Len         length of address space to translate
 *     @param[in]  pXlatdAddr  pointer to an address space type variable
 *
 * RETURNS:
 *    @return      BOOLEAN     TRUE if the translation worked
 *
 ***************************************************************************/
BOOLEAN MadBusPdoXlateBusAddr(PVOID Context, _In_ PHYSICAL_ADDRESS BusAddress, 
                              _In_ ULONG Len, _Out_ PULONG pAddrSpace, 
                              _Out_ PPHYSICAL_ADDRESS pXlatdAddr)
{
    PPDO_DEVICE_DATA pPdoData = (PPDO_DEVICE_DATA)Context;

    UNREFERENCED_PARAMETER(Len);
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadBusPdoXlateBusAddr... Bus#:Ser#=%d:%d\n", 
                pPdoData->pFdoData->SerialNo, pPdoData->SerialNo);

    *pAddrSpace = 0; //Memory vs IoSpace
    RtlCopyMemory(pXlatdAddr, &BusAddress, sizeof(PHYSICAL_ADDRESS)); //Our bus is in ram 

    return TRUE;
}

/************************************************************************//**
 * MadBusPdoGetDmaAdapter
 *
 * DESCRIPTION:
 *    This function returns a dma adapter.
 *    It is a DDI Query interface function
 *
 * PARAMETERS:
 *     @param[in]  Context         the PDO Context
 *     @param[in]  pNumMapRegstrs  pointer to the assigned # of map registers
 *
 * RETURNS:
 *    @return      pDmaAdapter pointer to the DMA adapter
 *
 ***************************************************************************/
struct _DMA_ADAPTER* 
MadBusPdoGetDmaAdapter(PVOID Context, 
                       _In_ struct _DEVICE_DESCRIPTION* pDevDescriptor,
                       _Out_ PULONG pNumMapRegstrs)
{
    PPDO_DEVICE_DATA pPdoData = (PPDO_DEVICE_DATA)Context;

    UNREFERENCED_PARAMETER(pDevDescriptor);
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadBusPdoGetDmaAdapter... SerialNo=%d\n", pPdoData->SerialNo);

    *pNumMapRegstrs = MAD_DMA_MAX_SECTORS;
    if (pPdoData->pDmaAdapterRd != NULL) //Already created
        return pPdoData->pDmaAdapterRd;

    //Create a DMA Adapter to provide to Storport in the query_interface irp
    WdfDeviceSetAlignmentRequirement(pPdoData->hDevice, MAD_DMA_ALIGN_REQ);
    WDF_DMA_ENABLER_CONFIG  DmaConfig;
    WDF_DMA_ENABLER_CONFIG_INIT(&DmaConfig, MAD_DMA_PROFILE,
                                (MAD_SECTOR_SIZE * MAD_DMA_MAX_SECTORS));
    NTSTATUS NtStatus =
             WdfDmaEnablerCreate(pPdoData->hDevice, &DmaConfig, 
                                 WDF_NO_OBJECT_ATTRIBUTES, &pPdoData->hDmaEnabler);
    if (!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                    "MadBusPdoGetDmaAdapter:WdfDmaEnablerCreate failed...status=x%X\n",
                    NtStatus);

        *pNumMapRegstrs = 0;
        return NULL;
        }

    WdfDmaEnablerSetMaximumScatterGatherElements(pPdoData->hDmaEnabler,
                                                 MAD_DMA_MAX_SECTORS);

    pPdoData->pDmaAdapterRd =
    WdfDmaEnablerWdmGetDmaAdapter(pPdoData->hDmaEnabler, 
                                  (WDF_DMA_DIRECTION)FALSE);
    pPdoData->pDmaAdapterWr =
    WdfDmaEnablerWdmGetDmaAdapter(pPdoData->hDmaEnabler,
                                  (WDF_DMA_DIRECTION)TRUE);

    return pPdoData->pDmaAdapterRd;
}

/************************************************************************//**
 * MadBusPdoGetConfigData
 *
 * DESCRIPTION:
 *    This function returns a bus configuration space data.
 *    It is a DDI Query interface function
 *
 * PARAMETERS:
 *     @param[in]  Context    the PDO Context
 *     @param[in]  DataType   the configuration space identifier
 *     @param[in]  pBuffer    pointer to the caller's buffer
 *     @param[in]  Offset     Offset into the configuration space
 *     @param[in]  Len        Length of data requested
 *
 * RETURNS:
 *    @return      Len        Length of data returned
 *
 ***************************************************************************/
ULONG MadBusPdoGetConfigData(_Inout_opt_ PVOID Context, _In_ ULONG DataType,
                             _Inout_updates_bytes_(Length) PVOID pBuffer, 
                             _In_ ULONG Offset, _In_ ULONG Len)
{
PPDO_DEVICE_DATA pPdoData = (PPDO_DEVICE_DATA)Context;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadBusPdoGetConfigData... Bus#:Ser#=%d:%d DataType=%d Offset=%d Len=%d pBufr=%p\n",
                pPdoData->pFdoData->SerialNo, 
                pPdoData->SerialNo, DataType, Offset, Len, pBuffer);

    ASSERT(Offset == 0);
    ASSERT(pBuffer != NULL);
    ASSERT(Len >= sizeof(PCI_COMMON_HEADER));
    if (pPdoData->pFdoData->SerialNo == eGenericBus)
        {
        ASSERT(DataType == MADBUS_WHICHSPACE_CONFIG);
        ASSERT(Len >= sizeof(MADBUS_DEVICE_CONFIG_DATA));
        }

    //Update the config_space data with a pointer to the vendor specific data 
    //when we don't have a full PCI config space.  We are stepping on the PCI_HEADER
    if (pPdoData->PnpBusInfo.LegacyBusType == Internal)
        {
        PVENDOR_SPECIFIC_DATA pVndrSpecData = 
                              &pPdoData->BusPdoCnfgData.VndrSpecData;
        PVOID pTempVar = 
              MadSetConfigTempVarLoc(&pPdoData->BusPdoCnfgData.LegacyPci);
        RtlCopyMemory(pTempVar, &pVndrSpecData, sizeof(PVOID));
        }

    RtlCopyMemory(pBuffer, &pPdoData->BusPdoCnfgData, Len);
    return Len;
}

/************************************************************************//**
 * MadBusPdoSetConfigData
 *
 * DESCRIPTION:
 *    This function updates bus configuration space data.
 *    It is a DDI Query interface function
 *
 * PARAMETERS:
 *     @param[in]  Context    the PDO Context
 *     @param[in]  DataType   the configuration space identifier
 *     @param[in]  pBuffer    pointer to the caller's buffer
 *     @param[in]  Offset     Offset into the configuration space
 *     @param[in]  Len        Length of data requested
 *
 * RETURNS:
 *    @return      Len        Length of data written
 *
 ***************************************************************************/
ULONG MadBusPdoSetConfigData(_Inout_opt_ PVOID Context, _In_ ULONG DataType,
                             _Inout_updates_bytes_(Length) PVOID pBuffer,
                             _In_ ULONG Offset, _In_ ULONG Len)
{
    PPDO_DEVICE_DATA pPdoData = (PPDO_DEVICE_DATA)Context;

    ASSERT(Offset == 0);

    //ASSERT(DataType == MADBUS_WHICHSPACE_CONFIG);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadBusPdoSetConfigData... Bus#:Ser#=%d:%d DataType=%d Offset=%d Len=%d\n",
                pPdoData->SerialNo, DataType, Offset, Len);

    RtlCopyMemory(&pPdoData->BusPdoCnfgData, pBuffer, Len);
    return Len;
}

//EVT_WDF_DEVICE_PROCESS_QUERY_INTERFACE_REQUEST
NTSTATUS MadBusPdoEvtProcessQueryInterfaceRequest(_In_ WDFDEVICE hDevice, 
                                                  _In_ LPGUID pInterfaceType,
                                                  _Inout_ PINTERFACE ExposedInterface,
                                                  _Inout_opt_ PVOID ExposedInterfaceSpecificData)
{
    PPDO_DEVICE_DATA pPdoData  = MadBusPdoGetData(hDevice);
    PDO_QUERY_INTF_TYPE eIntfQ = 
                        SetEnumIntfType(pInterfaceType, 
                                        pPdoData->pFdoData->pDriverData->QueryIntfGuids);
    GUID guid                  = *pInterfaceType;

    NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;

    UNREFERENCED_PARAMETER(ExposedInterface);
    UNREFERENCED_PARAMETER(ExposedInterfaceSpecificData);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadBusPdoEvtProcessQueryInterfaceRequest... Bus#:Ser#=%d:%d eIntfQ=%d\n",
                pPdoData->pFdoData->SerialNo, pPdoData->SerialNo, eIntfQ);
    if (eIntfQ == eUndefined)
        Trace_Guid(guid);

    NtStatus = 
    MadBusPdoProcessQueryInterfaceRequest(pPdoData, eIntfQ,
                                         ExposedInterface,
                                         ExposedInterfaceSpecificData);
    if (NtStatus != STATUS_SUCCESS)
        TraceEvents(TRACE_LEVEL_ERROR, MYDRIVER_ALL_INFO,
                    "MadBusPdoEvtProcessQueryInterfaceRequest... Bus#:Ser#=%d:%d eIntfQ=%d ntstatus=x%X\n",
                    pPdoData->pFdoData->SerialNo, pPdoData->SerialNo, eIntfQ, NtStatus);

    return NtStatus;
}

NTSTATUS MadBusPdoProcessQueryInterfaceRequest(PPDO_DEVICE_DATA  pPdoData,
                                               PDO_QUERY_INTF_TYPE eIntfQ,
                                               _Inout_ PINTERFACE pExposedInterface,
                                               _Inout_opt_ PVOID pInterfaceSpecificData)
{
    GUID guid = pPdoData->pFdoData->pDriverData->QueryIntfGuids[eIntfQ];
    NTSTATUS NtStatus = STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(pInterfaceSpecificData);

    TraceEvents(TRACE_LEVEL_WARNING, MYDRIVER_ALL_INFO,
                "MadBusPdoProcessQueryInterfaceRequest... Bus#:Ser#=%d:%d eIntfQ=%d\n",
                pPdoData->pFdoData->SerialNo, pPdoData->SerialNo, eIntfQ);

    switch (eIntfQ)
        {
        case eNullIntfQ: // GUID_NULL:a bogus guid sent by Driver-Verifier
            break;

        case eBusIntfQ:
            RtlCopyMemory(pExposedInterface, &pPdoData->BusInterfaceStd, 
                          sizeof(BUS_INTERFACE_STANDARD));
            break;

        case ePnpLocIntfQ:
            RtlCopyMemory(pExposedInterface, &pPdoData->PnpLocInterface, 
                          sizeof(PNP_LOCATION_INTERFACE));
            break;

        case eRenumSelfIntfQ:
            RtlCopyMemory(pExposedInterface, &pPdoData->ReEnumSelfStd, 
                          sizeof(REENUMERATE_SELF_INTERFACE_STANDARD));
            break;

        case eXAddrIntfQ:
            RtlCopyMemory(pExposedInterface, &pPdoData->PnpXtendedAddrIntf,
                          sizeof(PNP_EXTENDED_ADDRESS_INTERFACE));
            break;

        case eIommuBusIntfQ:
            //This is just a shot in the dark.
            //Copy the genertic portion of the businterfacestd + the pointer to the translate bus addr function
            {
            PINTERFACE pInterface = (PINTERFACE)pExposedInterface;
            USHORT Sizeof = sizeof(INTERFACE) + sizeof(PVOID);
            RtlCopyMemory(pInterface, &pPdoData->BusInterfaceStd, Sizeof);
            pInterface->Size = Sizeof;
            }
            break;

        case ePciBusIntfQ:
        case ePciBusIntfQ2:
        case eXlateIntfQ:
            NtStatus = STATUS_NOT_IMPLEMENTED;
            break;

        case eAcpiIntfQ:
        case eIntRouteIntfQ:
        case eArbtrIntfQ:
        case ePcmciaBusIntfQ:
        case eAcpiRegsIntfQ:
        case eLegacyDevIntfQ:
        case eMfEnumIntfQ:
            NtStatus = STATUS_NOT_SUPPORTED;
            break;

        default:
            //NtStatus = STATUS_INVALID_PARAMETER; // STATUS_INVALID_DEVICE_REQUEST;
            //GENERIC_SWITCH_DEFAULTCASE_ASSERT;
            //BRKPNT;
            TraceEvents(TRACE_LEVEL_WARNING, MYDRIVER_ALL_INFO,
                         "MadBusPdoProcessQueryInterfaceRequest... Bus#:Ser#=%d:%d using default Intf!\n",
                         pPdoData->pFdoData->SerialNo, pPdoData->SerialNo);
            RtlCopyMemory(pExposedInterface, &pPdoData->DfltInterface,
                          sizeof(INTERFACE));
        }

    if (NtStatus != STATUS_SUCCESS)
        {
        TraceEvents(TRACE_LEVEL_WARNING, MYDRIVER_ALL_INFO,
                    "MadBusProcessQueryInterfaceRequest... Bus#:Ser#=%d:%d eIntfQ=%d ntstatus=x%X\n",
                    pPdoData->pFdoData->SerialNo, pPdoData->SerialNo,
                    eIntfQ, NtStatus);
        Trace_Guid(guid);
        }

    return NtStatus;
}

//Here we would issue the Pnp Irp IoInvalidateDeviceRelations - if necessarey
VOID MadBusPdoReEnumerateSelf(PVOID Context)
{
    PPDO_DEVICE_DATA  pPdoData = (PPDO_DEVICE_DATA)Context;
    TraceEvents(TRACE_LEVEL_WARNING, MYDRIVER_ALL_INFO,
                "MadBusPdoReEnumerateSelf... Bus#:Ser#=%d:%d\n",
                pPdoData->pFdoData->SerialNo, pPdoData->SerialNo);
    ASSERTMSG("Unprocessed callback!", FALSE);
}

VOID MadBusPdoQueryExtendedAddress(_In_ PVOID Context,
                                   _Out_ PULONG64 ExtendedAddress)
{
PPDO_DEVICE_DATA  pPdoData = (PPDO_DEVICE_DATA)Context;

    TraceEvents(TRACE_LEVEL_WARNING, MYDRIVER_ALL_INFO,
                "MadBusPdoQueryExtendedAddress... Bus#:Ser#=%d:%d\n",
                pPdoData->pFdoData->SerialNo, pPdoData->SerialNo);

    *ExtendedAddress = (ULONG64)NULL;
}

#if 0
//Here we simulate all the Driver-Framework DMA processing functions which 
// KMDF won't provide because the OS isn't providing a DMA Adapter for devices
// not really on PCI. 
// Therefore WDF can't provide a DMA Enabler ... etc.
/************************************************************************//**
* MadSimDmaEnablerCreate
*
* DESCRIPTION:
*    This is the simulation mode replacement for WdfDmaEnablerCreate
*    Create a generic object with context to work around no real Dma-Enabler
*   
* PARAMETERS: 
*     @param[in]  hFDO         handle to the parent device 
*     @param[out] phDmaEnabler pointer to the dma enabler handle
*     
* RETURNS:
*    @return      NtStatus    indicates success or reason for the failure
* 
***************************************************************************/
NTSTATUS MadSimDmaEnablerCreate(IN WDFDEVICE hFDO, //This will be the DEVICE driver FDO
                                IN PWDF_DMA_ENABLER_CONFIG pDmaConfig, 
                                IN PWDF_OBJECT_ATTRIBUTES pObjAttr,
                                OUT WDFDMAENABLER* phDmaEnabler)
{
PPDO_DEVICE_DATA  pPdoData = MadBusGetPdoContextFromFdo(hFDO);

WDF_OBJECT_ATTRIBUTES  ObjAttrs;
NTSTATUS               NtStatus;
WDFDMAENABLER          hDmaEnabler;

    UNREFERENCED_PARAMETER(pDmaConfig);
    UNREFERENCED_PARAMETER(pObjAttr);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadSimDmaEnablerCreate... BusNum=%d SerialNo=%d hFDO(FDO)=%p\n",
                pPdoData->pFdoData->SerialNo, pPdoData->SerialNo, hFDO);

    WDF_OBJECT_ATTRIBUTES_INIT(&ObjAttrs);
    ObjAttrs.ParentObject = hFDO;
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&ObjAttrs, 
                                           MADSIM_DMA_ENABLER_CONTEXT);

    NtStatus = WdfObjectCreate(&ObjAttrs, (WDFOBJECT *)&hDmaEnabler);
    if (!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                    "MadSimDmaEnablerCreate:WdfObjectCreate returned x%X\n", 
                    NtStatus); 
        return NtStatus;
        }

    PMADSIM_DMA_ENABLER_CONTEXT pEnablerData = 
                                WdfObjectGetTypedContext(hDmaEnabler,
                                                         MADSIM_DMA_ENABLER_CONTEXT);
    ASSERT(pEnablerData != NULL);
    if (pEnablerData == NULL) //Return something distinctive other than WDF reporting STATUS_UNSUCCESSFUL ...
        return STATUS_NOT_IMPLEMENTED; //We may as well say we can't do this

    //Update the object context for other functions
    pEnablerData->hFDO = hFDO;
    *phDmaEnabler = hDmaEnabler;

    return STATUS_SUCCESS;
}

/************************************************************************//**
* MadSimDmaTransactionCreate
*
* DESCRIPTION:
*    This is the simulation mode replacement for WdfDmaTransactionCreate
*    Create a generic object with context to work around no real Dma-Enabler
*    
* PARAMETERS: 
*     @param[in]  hDmaEnabler handle to the parent dma enabler  
*     @param[out] pDmaXaxn    pointer to the dma transaction handle
*     
* RETURNS:
*    @return      NtStatus    indicates success or reason for the failure
* 
***************************************************************************/
NTSTATUS MadSimDmaTransactionCreate(IN WDFDMAENABLER hDmaEnabler,
                                    IN OPTIONAL WDF_OBJECT_ATTRIBUTES *pObjAttrs,
                                    OUT WDFDMATRANSACTION *phDmaXaxn)
{
PMADSIM_DMA_ENABLER_CONTEXT pEnablerData = 
                            WdfObjectGetTypedContext(hDmaEnabler, 
                                                     MADSIM_DMA_ENABLER_CONTEXT);
WDFDEVICE         hDevFDO = pEnablerData->hFDO;
PPDO_DEVICE_DATA  pPdoData = MadBusGetPdoContextFromFdo(hDevFDO);
//
WDF_OBJECT_ATTRIBUTES  ObjAttrs;
NTSTATUS               NtStatus;
WDFDMATRANSACTION      hDmaXaxn;

    UNREFERENCED_PARAMETER(pObjAttrs);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadSimDmaTransactionCreate...SerialNo=%d hDmaEnabler=%p\n",
                pPdoData->SerialNo, hDmaEnabler);

    WDF_OBJECT_ATTRIBUTES_INIT(&ObjAttrs);
    ObjAttrs.ParentObject = hDmaEnabler;
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&ObjAttrs, 
                                           MADSIM_DMA_TRANSACTION_CONTEXT);

    NtStatus = WdfObjectCreate(&ObjAttrs, (WDFOBJECT *)&hDmaXaxn);
    if (!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                    "MadSimDmaTransactionCreate:WdfObjectCreate returned x%X\n",
                    NtStatus); 
        return NtStatus;
        }

    PMADSIM_DMA_TRANSACTION_CONTEXT pXaxnData = 
                                    WdfObjectGetTypedContext(hDmaXaxn, 
                                                             MADSIM_DMA_TRANSACTION_CONTEXT);
    if (pXaxnData == NULL) //Return something distinctive other than WDF reporting STATUS_UNSUCCESSFUL ...
        return STATUS_NOT_IMPLEMENTED; //We may as well say we can't do this

    //Update the object context for other functions
    //Everything else in this context will be determined below in XxxDmaTransactionInitializeUsingRequest
    pXaxnData->hFDO = pEnablerData->hFDO;
    *phDmaXaxn = hDmaXaxn;

    return STATUS_SUCCESS;   
}

VOID MadSimDmaEnablerSetMaximumScatterGatherElements(IN WDFDMAENABLER hDmaEnabler, 
                                                     IN size_t MaximumFragments)
{
    UNREFERENCED_PARAMETER(hDmaEnabler);
    UNREFERENCED_PARAMETER(MaximumFragments);
    return; //Nothing to do
}

/************************************************************************//**
* MadSimDmaTransactionInitializeUsingRequest
*
* DESCRIPTION:
*    This is the simulation mode replacement for 
*    WdfDmaTransactionInitializeUsingRequest
*    The main purpose is to convert the request's MDL to an OS-internal
*    Scatter-Gather list getting ready for DmaTransactionExecute.
*    
* PARAMETERS: 
*     @param[in]  hDmaXaxn            handle to the Dma transaction  
*     @param[in]  hRequest            handle to the parent request   
*     @param[in]  pEvtProgramDmaFunxn pointer to the dma processing function
*                                     in the DEVICE driver
*     @param[in]  Direxn              indicates read or write
*     
* RETURNS:
*    @return      NtStatus    indicates success or reason for the failure
* 
***************************************************************************/
NTSTATUS 
MadSimDmaTransactionInitializeUsingRequest(IN WDFDMATRANSACTION hDmaXaxn, 
                                           IN WDFREQUEST hRequest,
                                           IN PFN_WDF_PROGRAM_DMA pEvtProgramDmaFunxn,
                                           IN WDF_DMA_DIRECTION Direxn)
{
PMADSIM_DMA_TRANSACTION_CONTEXT pXaxnData = 
                                WdfObjectGetTypedContext(hDmaXaxn,
                                                         MADSIM_DMA_TRANSACTION_CONTEXT);
WDFDEVICE         hDevFDO = pXaxnData->hFDO;
PPDO_DEVICE_DATA  pPdoData = MadBusGetPdoContextFromFdo(hDevFDO);
//
NTSTATUS         NtStatus = STATUS_SUCCESS; 
ULONG            NumItems = 0;
PMDL             pMDL = NULL;
PMDL             pCurrMdlItem;
PMDL             pNextMdlItem;
PVOID            SysAddr;
PHYSICAL_ADDRESS PhysAddr;

    PSCATTER_GATHER_LIST pSgList = (PSCATTER_GATHER_LIST)&pPdoData->SgList;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadSimDmaTransactionInitializeUsingRequest...SerialNo=%d hDmaXaxn=%p hRequest=%p Direxn=%d pEvtProgramDmaFunxn=%p\n",
                pPdoData->SerialNo, hDmaXaxn, hRequest, (ULONG)Direxn, pEvtProgramDmaFunxn);

    pPdoData->pEvtProgramDmaFunxn = pEvtProgramDmaFunxn; 

    if (!(BOOLEAN)Direxn) //It's a read
        NtStatus = WdfRequestRetrieveOutputWdmMdl(hRequest, &pMDL);
    else
        NtStatus = WdfRequestRetrieveInputWdmMdl(hRequest, &pMDL);
    if (!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                    "MadSimDmaTransactionInitializeUsingRequest:WdfRequestRetrieveIoWdmMdl... ntstatus=x%X\n",
                    NtStatus);
        return NtStatus;
        }

    //Build the OS Scatter-Gather List by converting the MDL
    for (pCurrMdlItem = pMDL; pCurrMdlItem != NULL; pCurrMdlItem = pNextMdlItem) 
        {
        pNextMdlItem = pCurrMdlItem->Next;
        if (!(pCurrMdlItem->MdlFlags & MDL_PAGES_LOCKED)) 
            {MmProbeAndLockPages(pCurrMdlItem, KernelMode, IoModifyAccess);}

        SysAddr = MmGetMdlVirtualAddress(pCurrMdlItem);
        //
        PhysAddr = MmGetPhysicalAddress(SysAddr);
        pSgList->Elements[NumItems].Address = PhysAddr;
        //
        pSgList->Elements[NumItems].Length = MmGetMdlByteCount(pCurrMdlItem); 
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                    "MadSimDmaTransactionInitializeUsingRequest: SysAddr=%p PhysAddr=x%X:%X Len=%d\n",
                    SysAddr, PhysAddr.HighPart,PhysAddr.LowPart, pSgList->Elements[NumItems].Length);
        NumItems++;
        }

    pSgList->NumberOfElements = NumItems;

    //Complete the transaction context for XxxDmaTransactionExecute
    pXaxnData->hRequest = hRequest;
    pXaxnData->Direxn   = Direxn;
    pXaxnData->pSgList  = pSgList; 

    return NtStatus;
}

/************************************************************************//**
* MadSimDmaTransactionExecute
*
* DESCRIPTION:
*    This is the simulation mode replacement for WdfDmaTransactionExecute
*    
* PARAMETERS: 
*     @param[in]  hDmaEnabler handle to the parent dma enabler  
*     @param[out] pDmaXaxn    pointer to the dma transaction handle
*     
* RETURNS:
*    @return      NtStatus    indicates success or reason for the failure
* 
***************************************************************************/
NTSTATUS MadSimDmaTransactionExecute(IN WDFDMATRANSACTION hDmaXaxn, 
                                     IN OPTIONAL PVOID Context)
{
PMADSIM_DMA_TRANSACTION_CONTEXT pXaxnData =
                                WdfObjectGetTypedContext(hDmaXaxn,
                                                         MADSIM_DMA_TRANSACTION_CONTEXT);
WDFDEVICE         hDevFDO = pXaxnData->hFDO;
PPDO_DEVICE_DATA  pPdoData = MadBusGetPdoContextFromFdo(hDevFDO);
NTSTATUS NtStatus = STATUS_SUCCESS;
BOOLEAN bRC;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadSimDmaTransactionExecute...SerialNo=%d hDmaXaxn=%p pEvtProgramDmaFunxn=%p\n",
                pPdoData->SerialNo, hDmaXaxn, pPdoData->pEvtProgramDmaFunxn);

    bRC = pPdoData->pEvtProgramDmaFunxn(hDmaXaxn, hDevFDO, Context, 
                                        pXaxnData->Direxn, pXaxnData->pSgList);
    NtStatus = bRC ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;

    return NtStatus;
}

/************************************************************************//**
* MadSimDmaTransactionGetRequest
*
* DESCRIPTION:
*    This is the simulation mode replacement for WdfDmaTransactionGetRequest
*
* PARAMETERS:
*     @param[out] hDmaXaxn    handle to the dma transaction
*
* RETURNS:
*    @return      hRequest    handle to the parent/associated request
*
***************************************************************************/
WDFREQUEST MadSimDmaTransactionGetRequest(IN WDFDMATRANSACTION hDmaXaxn)

{
PMADSIM_DMA_TRANSACTION_CONTEXT pXaxnData = 
                                WdfObjectGetTypedContext(hDmaXaxn, 
                                                         MADSIM_DMA_TRANSACTION_CONTEXT);
WDFREQUEST hRequest = pXaxnData->hRequest;

    return hRequest;
}

/************************************************************************//**
* MadSimDmaTransactionGetBytesTransferred
*
* DESCRIPTION:
*    This is the simulation mode replacement for
*     WdfDmaTransactionGetBytesTransferred
*
* PARAMETERS:
*     @param[in]  hDmaXaxn    handle to the dma transaction
*     @param[out] pDmaXaxn    pointer to the dma transaction handle
*
* RETURNS:
*    @return      BytesXferd  nuber of bytes dma-transferred
*
***************************************************************************/
size_t  MadSimDmaTransactionGetBytesTransferred(IN WDFDMATRANSACTION hDmaXaxn)
{
PMADSIM_DMA_TRANSACTION_CONTEXT pXaxnData = 
                                WdfObjectGetTypedContext(hDmaXaxn,
                                                         MADSIM_DMA_TRANSACTION_CONTEXT);
WDFDEVICE         hDevFDO = pXaxnData->hFDO; 
PPDO_DEVICE_DATA  pPdoData = MadBusGetPdoContextFromFdo(hDevFDO);
PMADREGS          pMadRegs = pPdoData->pMadRegs;
size_t            BytesXferd = (size_t)pMadRegs->DTBC; //Updated upon completion by our hardware

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadSimDmaTransactionGetBytesTransferred... SerialNo=%d hDmaXaxn=%p BytesXferd=%d\n",
                pPdoData->SerialNo, hDmaXaxn, (ULONG)BytesXferd);

    return BytesXferd;
}

BOOLEAN MadSimDmaTransactionDmaCompleted(IN WDFDMATRANSACTION hDmaXaxn, 
                                         OUT NTSTATUS *pStatus)
{
PMADSIM_DMA_TRANSACTION_CONTEXT pXaxnData =
                                WdfObjectGetTypedContext(hDmaXaxn, 
                                                         MADSIM_DMA_TRANSACTION_CONTEXT);
WDFDEVICE         hDevFDO = pXaxnData->hFDO;
PPDO_DEVICE_DATA  pPdoData = MadBusGetPdoContextFromFdo(hDevFDO);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadSimDmaTransactionCompleted... SerialNo=%d hDmaXaxn=%p\n",
                pPdoData->SerialNo, hDmaXaxn);

    *pStatus = STATUS_SUCCESS;

    return TRUE; //Always in the prototype
}

/************************************************************************//**
* MadSimDmaTransactionGetDevice
*
* DESCRIPTION:
*    This is the simulation mode replacement for WdfDmaTransactionGetDevice
*
* PARAMETERS:
*     @param[out] hDmaXaxn    handle to the dma transaction
*
* RETURNS:
*    @return      hFDO     handle to the parent/associated device
*
***************************************************************************/
WDFDEVICE MadSimDmaTransactionGetDevice(IN WDFDMATRANSACTION hDmaXaxn)
{
PMADSIM_DMA_TRANSACTION_CONTEXT pXaxnData = 
                                WdfObjectGetTypedContext(hDmaXaxn, 
                                                         MADSIM_DMA_TRANSACTION_CONTEXT);
WDFDEVICE                       hFDO  = pXaxnData->hFDO;

    return hFDO;
}
#endif //0

#if 0
/************************************************************************//**
 * MadBus_GetPowerLevel
 *
 * DESCRIPTION:
 *    This function gets the current power level of the MadDevice.
 *    It is a DDI Query interface function
 *
 * PARAMETERS:
 *     @param[in]  hChildDev  handle to our device
 *     @param[in]  Level      pointer to the power level
 *
 * RETURNS:
 *    @return      BOOLEAN   indicates success or failure
 *
 ***************************************************************************/
BOOLEAN MadBus_GetPowerLevel(IN WDFDEVICE hChildDev, OUT PUCHAR Level)

{
    UNREFERENCED_PARAMETER(hChildDev);

    // Validate the context to see if it's really a pointer to PDO's device extension. 
    // You can store some kind of signature in the PDO for this purpose
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, "GetPowerLevel\n");

    *Level = 10;

    return TRUE;
}

/************************************************************************//**
 * MadBus_SetPowerLevel
 *
 * DESCRIPTION:
 *    This function sets the current power level of the MadDevice.
 *    It is a DDI Query interface function
 *
 * PARAMETERS:
 *     @param[in]  hChildDev  handle to our device
 *     @param[in]  Level      pointer to the power level
 *
 * RETURNS:
 *     @return     BOOLEAN    indicates success or failure
 *
 ***************************************************************************/
BOOLEAN MadBus_SetPowerLevel(IN WDFDEVICE hChildDev, IN UCHAR Level)

{
    UNREFERENCED_PARAMETER(hChildDev);
    UNREFERENCED_PARAMETER(Level);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, "SetPowerLevel\n");

    return TRUE;
}

/************************************************************************//**
 * MadBus_IsSafetyLockEnabled
 *
 * DESCRIPTION:
 *    This function checks whether safety lock is enabled
 *    It is a DDI Query interface function
 *
 * PARAMETERS:
 *     @param[in]  hChildDev   handle to our device
 *
 * RETURNS:
 *    @return      BOOLEAN     indicates yes/no
 *
 ***************************************************************************/
BOOLEAN MadBus_IsSafetyLockEnabled(IN WDFDEVICE hChildDev)
{
    UNREFERENCED_PARAMETER(hChildDev);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
        "IsSafetyLockEnabled\n");

    return TRUE;
}

/************************************************************************//**
 * MadBusPdoGetLocString
 *
 * DESCRIPTION:
 *    This function returns a PnP device location string.
 *    It is a DDI Query interface function
 *
 * PARAMETERS:
 *     @param[in]  Context    the PDO Context
 *     @param[in]  Level      pointer to the power level
 *
 * RETURNS:
 *    @return      NSTATUS    indicates success or failure
 *
 ***************************************************************************/
NTSTATUS MadBusPdoGetLocString(_Inout_  PVOID Context, _Out_ PWCHAR* ppPnpLocStrings)
{
    static WCHAR Digits[] = L"0123456789";
    static WCHAR PnpLocString[] = L"PCI(0d00)\0\0"; //This is the format by convention (XXYY) = device_num:function_num
    //
    PPDO_DEVICE_DATA pPdoData = (PPDO_DEVICE_DATA)Context;

    //PnpMngr wants this data in a pool & will delete the pool for us
    PVOID pPnpLocStr = ExAllocatePoolWithTag(PagedPool, sizeof(WCHAR) * 20, MADBUS_POOL_TAG);
    ASSERT(pPnpLocStr != NULL);

    PnpLocString[5] = Digits[pPdoData->SerialNo];
    RtlFillMemory(pPnpLocStr, sizeof(WCHAR) * 20, 0x00);
    RtlCopyMemory(pPnpLocStr, PnpLocString, sizeof(WCHAR) * 11);
    *ppPnpLocStrings = (PWCHAR)pPnpLocStr;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadBusPdoGetLocString... SerialNo=%d, pLocStr=%p, assigned pnp_loc_str=%S\n",
                pPdoData->SerialNo, *ppPnpLocStrings, *ppPnpLocStrings);

    return STATUS_SUCCESS;
}
#endif
