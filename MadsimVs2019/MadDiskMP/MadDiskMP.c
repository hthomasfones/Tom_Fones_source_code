/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : MAD Device Simulation Framework                             */
/*  COPYRIGHT    : (c) 2022 HTF Consulting                                     */
/*                                                                             */
/* This source code is provided by exclusive license for non-commercial use to */
/* SOC Limited                                                                 */
/*                                                                             */
/*******************************************************************************/
/*                                                                             */
/*  Exe file ID  : MadDiskMP.sys                                               */
/*                                                                             */
/*  Module  NAME : MadDiskMP.c                                                 */
/*                                                                             */
/*  DESCRIPTION  : Definitions of structures and function prototypes for       */
/*                 MadDiskMP.sys                                               */
/*                 Derived from WDK\src\storage\StorportMiniports\lsi_u3       */
/*                 Copyright 1994 - 2008 LSI Corporation.All rights reserved.  */
/*                                                                             */
/*******************************************************************************/

#pragma warning(disable:4127) // conditional expression is constant
#pragma warning(disable:4213) // nonstandard extension used : cast on l-value
#pragma warning(disable:4214) // nonstandard extension used : bit field types other than int
#pragma warning(disable:4701) // potentially uninitialized local variable 'n' used
#pragma warning(disable:4055) // 'type cast' : from data pointer 'PVOID' to function pointer
#pragma warning(disable:4152) //nonstandard extension, function/data pointer conversion in expression

/**********************************************************************/

//#define FORCE_SYNC  // this define will default the driver to force sync.
/**********************************************************************/

// include files used by the Miniport
#include "MadDiskMP.h"

#ifdef WPP_TRACING
    //#pragma message("WPP_TRACING defined")
    #include "trace.h"
    #include "MadDiskMP.tmh"
#endif

// version string for DMI
UCHAR driver_version[] = MAD_VERSION_LABEL;   // driver version string

DRIVER_UNLOAD* gpStorportDriverUnload = NULL;

//#ifdef _MAD_SIMULATION_MODE_ 
PVOID          gpDevXtensns[MAD_MAX_DEVICES];
PDRIVER_OBJECT gpDriverObj = NULL;
INQUIRYDATA          gInquiryData;
IDENTIFY_DEVICE_DATA gIdDeviceData;
//#endif//_MAD_SIMULATION_MODE_ 
GUID gDiskGUIDs[] =
{ {0L, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
// {796513A1-1B19-4DEF-9FAD-9292D6EE7CBB}
{0x796513a1, 0x1b19, 0x4def, { 0x9f, 0xad, 0x92, 0x92, 0xd6, 0xee, 0x7c, 0xbb}},
{0x796513a2, 0x1b19, 0x4def, { 0x9f, 0xad, 0x92, 0x92, 0xd6, 0xee, 0x7c, 0xbb}},
{0x796513a3, 0x1b19, 0x4def, { 0x9f, 0xad, 0x92, 0x92, 0xd6, 0xee, 0x7c, 0xbb}} };

/*++
Routine Description:
Initial entry point for LSI_U3 miniport driver.

Arguments:
Driver Object

Return Value:
Status indicating whether adapter(s) were found and initialized.
--*/
ULONG DriverEntry(IN PVOID pDriverObject, IN PVOID pRegistryPath)
{
PDRIVER_OBJECT pDriverObj = (PDRIVER_OBJECT)pDriverObject;

ULONG  Status;
#ifdef _MAD_SIMULATION_MODE_ 
	VIRTUAL_HW_INITIALIZATION_DATA MadHdwInitData;
	ULONG SizeofHdwInit = sizeof(VIRTUAL_HW_INITIALIZATION_DATA);
#else
	HW_INITIALIZATION_DATA MadHdwInitData;
	ULONG SizeofHdwInit = sizeof(HW_INITIALIZATION_DATA);
#endif

#ifdef WPP_TRACING
	WPP_INIT_TRACING(pDriverObj, pRegistryPath);
#endif

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDisk SCSI Miniport DriverEntry... build date:time %s:%s HdwInitSize=%d\n", 
		        BUILD_DATE, BUILD_TIME, SizeofHdwInit);

	//Set up the HW_INITIALIZATION_DATA
	RtlZeroMemory(&MadHdwInitData, SizeofHdwInit);
	MadHdwInitData.HwInitializationDataSize = SizeofHdwInit;

	// Adapter specific information.
	MadHdwInitData.AdapterInterfaceType = MAD_BUS_INTERFACE_TYPE_SCSI;
	MadHdwInitData.DeviceExtensionSize = sizeof(MAD_HDW_DEVICE_EXTENSION);
	MadHdwInitData.SpecificLuExtensionSize = sizeof(MAD_DISK_LU_EXTENSION);
	MadHdwInitData.SrbExtensionSize = sizeof(SRB_EXTENSION);
	MadHdwInitData.NumberOfAccessRanges = MADDISK_NUM_ACCESS_RANGES;
	MadHdwInitData.MapBuffers = STOR_MAP_ALL_BUFFERS_INCLUDING_READ_WRITE;
	MadHdwInitData.NeedPhysicalAddresses = TRUE;
	MadHdwInitData.TaggedQueuing = TRUE; //Required
	MadHdwInitData.AutoRequestSense = TRUE; //Required
	MadHdwInitData.MultipleRequestPerLu = TRUE; //Necesssary
	MadHdwInitData.PortVersionFlags = 0;

	//MadHdwInitData.SrbTypeFlags = SRB_TYPE_FLAG_STORAGE_REQUEST_BLOCK;
	/*MadHdwInitData.FeatureSupport =
		           (MAD_DEVICE_TYPE_DISK == FILE_DEVICE_VIRTUAL_DISK) ?
		           STOR_FEATURE_MAD_VIRTUAL_MINIPORT : 0; */

	// Identify required miniport entry point routines.
	MadHdwInitData.HwInitialize     = MadDiskHdwInitialize; 
	MadHdwInitData.HwStartIo        = MadDiskStartIo; 
	MadHdwInitData.HwInterrupt      = MadDiskISR; 
	MadHdwInitData.HwFindAdapter    = MadDiskFindAdapter; 
	MadHdwInitData.HwResetBus       = MadDiskBusReset; 
	MadHdwInitData.HwAdapterControl = MadDiskAdapterControl;
	MadHdwInitData.HwBuildIo        = MadDiskBuildIo;

#ifdef  MAD_VIRTUAL_MINIPORT
	MadHdwInitData.HwFreeAdapterResources = MadDiskFreeResources;
	MadHdwInitData.HwProcessServiceRequest = NULL; // MadDiskProcessServiceRequest;
	MadHdwInitData.HwCompleteServiceIrp = NULL; // MadDiskCompleteServiceIrp;
#endif

	//MadHdwInitData.HwDmaStarted = MadDiskDmaStarted; //Subordinate-mode DMA not supported
	MadHdwInitData.HwDmaStarted = NULL;
	//MadHdwInitData.HwAdapterState = MadDiskHdwAdapterState;
	MadHdwInitData.HwAdapterState = NULL;

	// We need to hook Storport's DriverUnload routine, since one is not explicitly available to
    // Storport miniports.  We need a callback routine at PASSIVE IRQL so that we can call
    // WPP_CLEANUP() for event tracing.  We store off Storport's DriverUnload routine, so that
    // we can call it from our DriverUnload routine when we are finished with WPP_CLEANUP().
	gpStorportDriverUnload  = (DRIVER_UNLOAD *)pDriverObj->DriverUnload;
	pDriverObj->DriverUnload = &MadDiskDriverUnload;
	gpDriverObj = pDriverObj;

#ifdef _MAD_SIMULATION_MODE_
	MadDiskInitDefaultInquiryData(&gInquiryData);
	MadDiskInitDefaultIdDeviceData(&gIdDeviceData);
#endif

	// call StorPort to register our HW init data
	Status = StorPortInitialize(pDriverObj, pRegistryPath,
		                        (PHW_INITIALIZATION_DATA)&MadHdwInitData, NULL);

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDisk SCSI Miniport DriverEntry... StorPortInit status=x%X\n",
		        Status);

	return(Status);
} 

void MadDiskDriverUnload(PDRIVER_OBJECT pDriverObj)
{
	ULONG j;
	PMAD_HDW_DEVICE_EXTENSION pDevXtensn;

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDiskDriverUnload... the MadDiskMP driver is exiting *!*\n");

#ifdef WPP_TRACING
  	WPP_CLEANUP(pDriverObj);
#endif

	for (j = 0; j < MAD_MAX_DEVICES; j++)
		if (gpDevXtensns[j] != NULL)
		    {
			pDevXtensn = (PMAD_HDW_DEVICE_EXTENSION)gpDevXtensns[j];
			if (pDevXtensn->ConnectListEntry.bConnected == TRUE)
			    {
				pDevXtensn->ConnectListEntry.bConnected = FALSE;
				StorPortNotification(BusChangeDetected, pDevXtensn, 0);
			    }
		    }

    gpStorportDriverUnload(pDriverObj);
    return;
}

/*++
Routine Description:

This function initializes the LU flags and all of the svdt and I/O
tracking queues, then starts the scripts waiting for commands.

Arguments:
Context - Pointer to the device extension for this SCSI bus.

Return Value:
TRUE
--*/
BOOLEAN MadDiskHdwInitialize(IN PVOID Context)
{
PMAD_HDW_DEVICE_EXTENSION pDevXtensn = (PMAD_HDW_DEVICE_EXTENSION)Context;
ULONG SerialNo = pDevXtensn->SerialNo;
BOOLEAN bRC = TRUE; 
ULONG SpGetDevObjs;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	            "MadDiskHdwInitialize enter... SerialNo=%d\n", 
		        pDevXtensn->SerialNo);

	SpGetDevObjs = StorPortGetDeviceObjects(pDevXtensn, 
		                                    &pDevXtensn->pThisDevObj,
			                                &pDevXtensn->pParentDevObj, 
		                                    &pDevXtensn->pLowerDevObj);
	if (SpGetDevObjs != STOR_STATUS_SUCCESS)
	    {
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadDiskHdwInitialize:StorPortGetDeviceObjects... SerialNo=%d Status=%d\n",
			        SerialNo, SpGetDevObjs);
		return FALSE;
	    }

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	"MadDiskHdwInitialize:StorPortGetDeviceObjects... SerialNo=%d pThis=%p pPrnt=%p pLower=%p\n",
		        pDevXtensn->SerialNo, pDevXtensn->pThisDevObj,
		        pDevXtensn->pParentDevObj, pDevXtensn->pLowerDevObj);

	StorPortInitializeDpc(pDevXtensn, &pDevXtensn->MadIsrDpc, MadDiskDpc);

	pDevXtensn->pActiveSrb = NULL;
	bRC = StorPortReady(pDevXtensn);

#ifdef _MAD_SIMULATION_MODE_
	StorPortCopyMemory(&pDevXtensn->DiskGUID,
		               &gDiskGUIDs[SerialNo], sizeof(GUID));
	bRC = MadDiskAttachVDisk(pDevXtensn);
	//TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	//		   "MadDiskHdwInitialize:MadDiskAttachVDisk... SerialNo=%d rc=%d\n",
	//		    SerialNo, bRC);
#endif

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDiskHdwInitialize returning %d\n", bRC);
	return bRC;
} 

/*++
Routine Description:
This function fills in the configuration information structure

Arguments:
Context - Supplies a pointer to the device extension.
Reserved1 - Unused.
Reserved2 - Unused.
ArgumentString - DriverParameter string.

ConfigInfo - Pointer to the configuration information structure to be
filled in.
Reserved3 - Unused.

Return Value:
Returas status based upon results of adapter parameter acquisition.
--*/
#ifdef MAD_VIRTUAL_MINIPORT
ULONG MadDiskFindAdapter(_In_ PVOID DeviceExtension, _In_ PVOID HdwContext,
	                     _In_ PVOID BusInformation, _In_ PVOID LowerDevice,
	                     _In_ PCHAR ArgumentString,
	                     _Inout_ PPORT_CONFIGURATION_INFORMATION  pPortConfigInfo,
	                     _In_ PBOOLEAN Again)
{
	PMAD_HDW_DEVICE_EXTENSION pDevXtensn =
		(PMAD_HDW_DEVICE_EXTENSION)DeviceExtension;

#else //===================================================================

ULONG MadDiskFindAdapter(__in PVOID Context,
	__in PVOID Reserved1, __in PVOID Reserved2,
	__in PCSTR ArgumentString,
	__inout PPORT_CONFIGURATION_INFORMATION pPortConfigInfo,
	__in PUCHAR Reserved3)
{
	PMAD_HDW_DEVICE_EXTENSION pDevXtensn = (PMAD_HDW_DEVICE_EXTENSION)Context;
#endif

	//PACCESS_RANGE AccessRange, /*SAccessRange,*/ MM_Range; // , IO_Range;
	PPCI_COMMON_CONFIG pPciConf = NULL;
	ULONG PciCnfgLen;
	UCHAR PciCnfgBufr[MADBUS_DEV_CNFG_SIZE]; // , i; // , reqId, i, hbaDeviceID;
	ULONG MpStatus = 0;
	//USHORT hbaCap;

#ifdef MAD_VIRTUAL_MINIPORT
	UNREFERENCED_PARAMETER(ArgumentString);
	UNREFERENCED_PARAMETER(Again);
	UNREFERENCED_PARAMETER(HdwContext);
	UNREFERENCED_PARAMETER(BusInformation);
	UNREFERENCED_PARAMETER(LowerDevice);
#else
	UNREFERENCED_PARAMETER(Reserved1);
	UNREFERENCED_PARAMETER(Reserved2);
	UNREFERENCED_PARAMETER(ArgumentString);
	UNREFERENCED_PARAMETER(Reserved3);
#endif

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	"MadDiskFindAdapter... PortCnfgInfo:Len=%d SysBusNum=%d IntfType=%d Slot#=%d\n",
		        pPortConfigInfo->Length, pPortConfigInfo->SystemIoBusNumber,
		        pPortConfigInfo->AdapterInterfaceType, 
		        pPortConfigInfo->SlotNumber);

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	"MadDiskFindAdapter... PortCnfgInfo BusIntlevel = % d BusIntVector = % d IntMode = % d\n",
		pPortConfigInfo->BusInterruptLevel,
		pPortConfigInfo->BusInterruptVector, pPortConfigInfo->InterruptMode);

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	"MadDiskFindAdapter... PortCnfgInfo SctrGthr=%d BusMaster=%d NumPhysBreaks=%d Dma_Channel:Port:Width:Speed=%d:%d:%d:%d\n",
		        pPortConfigInfo->ScatterGather, pPortConfigInfo->Master,
		        pPortConfigInfo->NumberOfPhysicalBreaks,
		        pPortConfigInfo->DmaChannel, pPortConfigInfo->DmaPort,
		       pPortConfigInfo->DmaWidth, pPortConfigInfo->DmaSpeed);

#if 0
	// see if the pDevXtensn has not been zeroed out
	if (pDevXtensn->NonCachedExtension)
	{
		// no, zero it out
		ptr = (PUCHAR)pDevXtensn;
		for (loop = 0; loop < sizeof(MAD_HDW_DEVICE_EXTENSION); loop++)
			*ptr++ = 0;
	}

	// get memory-mapped and port I/O access range info.
	MM_Range = IO_Range = NULL;
	IO_Range = NULL;

	for (i = 0; i < 3; i++)     // scan ranges 0 & 1
	{
		AccessRange = &((*(pPortConfigInfo->AccessRanges))[i]);
		if (!(AccessRange->RangeLength))       // check for NULL entry
			continue;

		if (AccessRange->RangeInMemory)     // memory-mapped or port I/O
			MM_Range = AccessRange;
		else
			IO_Range = AccessRange;
	}

	if (MM_Range == NULL)       // if desired access range is not available
	{
		MpStatus = SP_RETURN_NOT_FOUND;
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			"MadDiskFindAdapter... bad memory access range pDevXtensn=%p MpStatus=%d\n",
			pDevXtensn, MpStatus);
		return MpStatus;
	}

	AccessRange = MM_Range;
	pDevXtensn->pMadRegs =
		(PMADREGS)StorPortGetDeviceBase(pDevXtensn,
			pPortConfigInfo->AdapterInterfaceType,
			pPortConfigInfo->SystemIoBusNumber,
			AccessRange->RangeStart,
			AccessRange->RangeLength,
			(BOOLEAN)!AccessRange->RangeInMemory);
	pDevXtensn->BARs[0] = AccessRange->RangeStart; // .LowPart;

	// check for error in mapping adapter registers to a virtual address
	if (!pDevXtensn->pMadRegs)
	{
		MpStatus = SP_RETURN_NOT_FOUND;
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			"MadDiskMP(%2x) MadDiskFindAdapter... StorPortGetDeviceBase (regs) Failed! MpStatus=%d\n",
			MpStatus);
	}
#endif

	// get PCI config space header for future use
	PciCnfgLen = StorPortGetBusData(pDevXtensn, PCIConfiguration,
		                            pPortConfigInfo->SystemIoBusNumber,
		                            (ULONG)pPortConfigInfo->SlotNumber,
		                            (PVOID)PciCnfgBufr, MADDISK_DEV_CNFG_SIZE);
	if (PciCnfgLen < MADDISK_DEV_CNFG_SIZE)
	    {
		MpStatus = SP_RETURN_BAD_CONFIG;
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		"MadDiskFindAdapter... failed to retreive PciConfigInfo! MpStatus=%d\n",
			        MpStatus);
		return MpStatus;
	    }

	// set pointer if we got valid PCI config data
	pPciConf = (PPCI_COMMON_CONFIG)PciCnfgBufr;
	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	"MadDiskFindAdapter... pPciConf=%p ConfigLen=%d Vendorid:Deviceid=(%4X:%4X)\n",
		        pPciConf, PciCnfgLen, pPciConf->VendorID, pPciConf->DeviceID);
	if ((pPciConf->VendorID != MAD_VENDOR_ID) ||
		(pPciConf->DeviceID != MAD_DEVICE_ID_DISK))
	    {
		MpStatus = SP_RETURN_BAD_CONFIG;
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadDiskFindAdapter... Unknown device MpStatus=%d\n",
			        MpStatus);
		return MpStatus;
	    }

	pDevXtensn->BARs[0].LowPart = pPciConf->u.type0.BaseAddresses[0];
	pDevXtensn->BARs[1].LowPart = pPciConf->u.type0.BaseAddresses[1];
	pDevXtensn->BARs[2].LowPart = pPciConf->u.type0.BaseAddresses[2];
	pDevXtensn->BARs[3].LowPart = pPciConf->u.type0.BaseAddresses[3];

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	"MadDiskFindAdapter... Bar0=%X:%08X Bar1=%X:%08X Bar2=%X:%08X Bar3=%X:%08X\n",
		        pDevXtensn->BARs[0].HighPart, pDevXtensn->BARs[0].LowPart,
		        pDevXtensn->BARs[1].HighPart, pDevXtensn->BARs[1].LowPart,
		        pDevXtensn->BARs[2].HighPart, pDevXtensn->BARs[2].LowPart,
		        pDevXtensn->BARs[3].HighPart, pDevXtensn->BARs[3].LowPart);

#ifdef _MAD_SIMULATION_MODE_
	MadDiskExchangeSimParms(pDevXtensn, pPciConf);
#endif //_MAD_SIMULATION_MODE_

	gpDevXtensns[pDevXtensn->SerialNo] = pDevXtensn;

	//ULONG MapLen = MAD_REGISTER_BLOCK_SIZE;
	pDevXtensn->VirtBARs[0] =
	MadDiskGetDeviceBase(pDevXtensn, pPortConfigInfo->AdapterInterfaceType,
			             pPortConfigInfo->SystemIoBusNumber,
			             pDevXtensn->BARs[0],
			             MAD_REGISTER_BLOCK_SIZE, FALSE);
	pDevXtensn->pMadRegs = (PMADREGS)pDevXtensn->VirtBARs[0];

	//MapLen = MAD_CACHE_SIZE_BYTES;
	pDevXtensn->VirtBARs[1] =
	MadDiskGetDeviceBase(pDevXtensn, pPortConfigInfo->AdapterInterfaceType,
			             pPortConfigInfo->SystemIoBusNumber,
			             pDevXtensn->BARs[1], MAD_CACHE_SIZE_BYTES, FALSE);

	//MapLen = MAD_CACHE_SIZE_BYTES;
	pDevXtensn->VirtBARs[2] =
	MadDiskGetDeviceBase(pDevXtensn, pPortConfigInfo->AdapterInterfaceType,
			             pPortConfigInfo->SystemIoBusNumber,
			             pDevXtensn->BARs[2], MAD_CACHE_SIZE_BYTES, FALSE);

	//MapLen = MAD_DEFAULT_DATA_EXTENT;
	pDevXtensn->VirtBARs[3] =
	MadDiskGetDeviceBase(pDevXtensn, pPortConfigInfo->AdapterInterfaceType,
			             pPortConfigInfo->SystemIoBusNumber,
			            pDevXtensn->BARs[3], MAD_DEFAULT_DATA_EXTENT, FALSE);

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	"MadDiskFindAdapter... VirtBar0=%p VirtBar1=%p VirtBar2=%p VirtBar3=%p\n",
		        pDevXtensn->VirtBARs[0], pDevXtensn->VirtBARs[1],
		        pDevXtensn->VirtBARs[2], pDevXtensn->VirtBARs[3]);

	// Fill in all the Port Config Info fields we own & know 
	pPortConfigInfo->Length = sizeof(PORT_CONFIGURATION_INFORMATION);
	//pPortConfigInfo->SystemIoBusNumber = 0;
	pPortConfigInfo->AdapterInterfaceType = MAD_BUS_INTERFACE_TYPE_SCSI;

#ifdef _MAD_SIMULATION_MODE_
	pPortConfigInfo->BusInterruptLevel = pDevXtensn->pVndrSpecData->Irql;
	pPortConfigInfo->BusInterruptVector = pDevXtensn->pVndrSpecData->IntVector;
	pPortConfigInfo->InterruptMode = Latched;
#endif

	pPortConfigInfo->MaximumTransferLength = MAD_DMA_MAX_BYTES;
	pPortConfigInfo->NumberOfPhysicalBreaks = MAD_DMA_MAX_SECTORS - 1;
	pPortConfigInfo->AlignmentMask = 0x000001ff; // FILE_512_BYTE_ALIGNMENT;
	pPortConfigInfo->NumberOfAccessRanges = MADDISK_NUM_ACCESS_RANGES;
	pPortConfigInfo->MaximumNumberOfTargets = 1; // TARGETS_PER_PORT; // NUMBER_OF_PORTS;
	pPortConfigInfo->ScatterGather = TRUE;
	pPortConfigInfo->Master = TRUE;
	pPortConfigInfo->CachesData = FALSE;
	pPortConfigInfo->NeedPhysicalAddresses = TRUE;
	pPortConfigInfo->MaximumNumberOfTargets = 1; // TARGETS_PER_PORT;
	pPortConfigInfo->SrbType  = SRB_TYPE_STORAGE_REQUEST_BLOCK;
	//pPortConfigInfo->SlotNumber = pDevXtensn->SerialNo;
	pPortConfigInfo->DeviceExtensionSize = sizeof(MAD_HDW_DEVICE_EXTENSION);
	pPortConfigInfo->SpecificLuExtensionSize = 0;
	pPortConfigInfo->SrbExtensionSize = 256; //?
	pPortConfigInfo->Dma64BitAddresses = 0;
	pPortConfigInfo->WmiDataProvider = FALSE;
	pPortConfigInfo->SynchronizationModel = StorSynchronizeHalfDuplex;
	pPortConfigInfo->HwMSInterruptRoutine = NULL; //No MSI support now

	//ONE disk per adapter
	pPortConfigInfo->NumberOfBuses = 1; // TARGETS_PER_PORT;
	pPortConfigInfo->MaximumNumberOfTargets = 1; // TARGETS_PER_PORT;
	pPortConfigInfo->MaximumNumberOfLogicalUnits = 1;

	pPortConfigInfo->VirtualDevice = MAD_VIRTUAL_DEVICE_BOOL;

	//pPortConfigInfo->MaxNumberOfIO               = 0xff; 
	//pPortConfigInfo->MaxIOsPerLun                = 0xff; 
	//pPortConfigInfo->InitialLunQueueDepth        = 0xff; 
	pPortConfigInfo->FeatureSupport = 0;

	MadDiskInitDevExtension(pDevXtensn, gDiskGUIDs, &gIdDeviceData);

#ifdef _MAD_SIMULATION_MODE_
	//Initiate the device worker thread for this device
	PFN_BUS_SET_POWER_UP pSetPowerUpEvFunxn =
	pDevXtensn->pMadSimIntParms->u.StorIntParms.pSetPowerUpEvFunxn;
	pSetPowerUpEvFunxn(pDevXtensn->SerialNo);
#endif //_MAD_SIMULATION_MODE_

	StorPortCopyMemory(&pDevXtensn->PortConfigInfo, pPortConfigInfo,
		               sizeof(PORT_CONFIGURATION_INFORMATION));

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDiskFindAdapter normal exit\n");

	return(SP_RETURN_FOUND);
}

/*++
Routine Description:
This externally called routine resets the SIOP and the SCSI bus.

Arguments:
pDevXtensn  - Supplies a pointer to the specific device extension.
PathId - Indicates adapter to reset.

Return Value:
TRUE - bus successfully reset
--*/
BOOLEAN MadDiskBusReset(IN PVOID Context, IN ULONG PathId)
{
	PMAD_HDW_DEVICE_EXTENSION pDevXtensn = (PMAD_HDW_DEVICE_EXTENSION)Context;
	//PSTORAGE_REQUEST_BLOCK  srb;
	//UCHAR IntStatus;
	BOOLEAN bRC = TRUE;

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	"MadDiskBusReset O/S requested SCSI bus reset... SerialNo=%d PathId=%d\n",
		        pDevXtensn->SerialNo, PathId);

	bRC = StorPortSynchronizeAccess(pDevXtensn, /*SynchronizeReset*/NULL, NULL);
	StorPortCompleteRequest(pDevXtensn, (UCHAR)PathId, SP_UNTAGGED, SP_UNTAGGED,
		                    (UCHAR)SRB_STATUS_BUS_RESET);

	//TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, 
	//	        "MadDiskBusReset exit\n");

	return bRC;
} // MadDiskBusReset


/*++
Routine Description:
	This is a generic routine to allow for special adapter control routines
	to be implemented without changing the HW_INITIALIZATION_DATA structure.
	Currently, it contains the StopAdapter routine and the ScsiRestartAdapter
	routine which replaces FindAdapter and HwInitialize for adapters going
	through a power management cycle.  Since the pDevXtensn is maintained
	through the power cycle, it is not necessary to use ScsiSetRunningConfig
	to access system memory.

Arguments:
	pDevExt - Pointer to the device extension for this SCSI bus.
	ControlType - Specifies the type of call being made through this routine.
	Parameters - Pointer to parameters needed for this control type (optional).

Return Value:
	SCSI_ADAPTER_CONTROL_STATUS - currently either:
		ScsiAdapterControlSuccess (= 0)
		ScsiAdapterControlUnsuccessful (= 1)
	(additional status codes can be added with new control codes)
--*/
SCSI_ADAPTER_CONTROL_STATUS 
MadDiskAdapterControl(IN PVOID pDevExt,
	                  IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
	                  IN PVOID Parameters)
{
	PMAD_HDW_DEVICE_EXTENSION pDevXtensn = (PMAD_HDW_DEVICE_EXTENSION)pDevExt;
	PSCSI_SUPPORTED_CONTROL_TYPE_LIST pCtlTypeList =
		(PSCSI_SUPPORTED_CONTROL_TYPE_LIST)Parameters;
	SCSI_ADAPTER_CONTROL_STATUS ScsiCtlStatus = ScsiAdapterControlSuccess;
	register ULONG j;

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDiskAdapterControl... SerialNo=%d CtlType=%d pParms=%p\n",
		        pDevXtensn->SerialNo, ControlType, Parameters);

	// do a switch on the ControlType
	switch (ControlType)
	    {
		// determine which control types (routines) are supported
	    case ScsiQuerySupportedControlTypes:
		    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		    "MadDiskAdapterControl:ScsiQuerySupportedControlTypes MaxCtlTypes=%d\n",
			            pCtlTypeList->MaxControlType);

		    // Mark all types that we support.
		    // Note that we must be careful to not overrun the type list buffer.
		    RtlZeroMemory(pCtlTypeList->SupportedTypeList,
			              pCtlTypeList->MaxControlType);
		    for (j = 0; j < pCtlTypeList->MaxControlType; j++)
			     pCtlTypeList->SupportedTypeList[j] = TRUE;

		     pCtlTypeList->SupportedTypeList[ScsiSetBootConfig] = FALSE;
		     pCtlTypeList->SupportedTypeList[ScsiSetRunningConfig] = FALSE;
		     break;

		// StopAdapter routine called just before power down of adapter
	    case ScsiStopAdapter:
		    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			            "MadDiskAdapterControl:ScsiStopAdapter... entry\n");
		    pDevXtensn->StopAdapter = TRUE; // set StopAdapter flag
		    break;

		// routine to reinitialize adapter while system in running.  Since
		// the adapter pDevXtensn is maintained through a power management
		// cycle, we can just restore the scripts and reinitialize the chip.
	    case ScsiRestartAdapter:
		    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			            "MadDiskAdapterControl:ScsiRestartAdapter... entry\n");
		    pDevXtensn->StopAdapter = FALSE; // clear StopAdapter flag
		    break;

		//This operation is requested when Storport wants to restore any settings on a SMD
		//that the BIOS might need to reboot.
	    case ScsiSetBootConfig:
		    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			           "MadDiskAdapterControl:ScsiSetBootConfig... entry\n");
		    break;

		// This operation is requested when Storport wants to restore any settings on a
		// virtual adapter that the miniport driver might need to control while the system is running.
	    case ScsiSetRunningConfig:
		    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			            "MadDiskAdapterControl:ScsiSetBootConfig... entry\n");
		    break;

	    case ScsiAdapterPrepareForBusReScan:
		     TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			             "MadDiskAdapterControl:PrepareForBusReScan... entry\n");
		    break;

	    case ScsiPowerSettingNotification:
	    case ScsiAdapterPower:
	    case ScsiAdapterSurpriseRemoval:
	    case ScsiAdapterSerialNumber:
	    case ScsiAdapterPoFxPowerRequired:
	    case ScsiAdapterPoFxPowerActive:
	    case ScsiAdapterPoFxPowerSetFState:
	    case ScsiAdapterPoFxPowerControl:
	    case ScsiAdapterSystemPowerHints:
	    case ScsiAdapterFilterResourceRequirements:
	    case ScsiAdapterPoFxMaxOperationalPower:
	    case ScsiAdapterPoFxSetPerfState:
	    case ScsiAdapterCryptoOperation:
	    case ScsiAdapterQueryFruId:
	    case ScsiAdapterSetEventLogging:
	    case ScsiAdapterReportInternalData:
		    ScsiCtlStatus = ScsiAdapterControlUnsuccessful;
		    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		    "MadDiskAdapterControl unimplemented CtlType... SerialNo=%d rc=%d\n",
			            pDevXtensn->SerialNo, ScsiCtlStatus);
		    break;

	    default:
		    ScsiCtlStatus = ScsiAdapterControlUnsuccessful;
		    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			"MadDiskAdapterControl unknown CtlType!... SerialNo=%d rc=%d\n",
			pDevXtensn->SerialNo, ScsiCtlStatus);
	    } // end of switch

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDiskAdapterControl exit... CtlStatus=%d\n", ScsiCtlStatus);

	return ScsiCtlStatus;
} // MadDiskAdapterControl

/*++
Routine Description:
	This routine receives requests from the port driver.  This routine has no
	locks held (any number of BuildIo threads can be running concurrently).
	Build as much of the I/O structure as possible without modifying shared
	memory.

Arguments:
	Context - pointer to the device extension for the adapter.
	pSRB - pointer to the request to be started.

Return Value:
	TRUE - the request was accepted.
	FALSE - the request must be submitted later.
--*/
BOOLEAN MadDiskBuildIo(IN PVOID Context, IN PSCSI_REQUEST_BLOCK pScsiRB)
{
	PMAD_HDW_DEVICE_EXTENSION pDevXtensn = (PMAD_HDW_DEVICE_EXTENSION)Context;
	PSTORAGE_REQUEST_BLOCK pSRB = (PSTORAGE_REQUEST_BLOCK)pScsiRB;
	PSRB_EXTENSION       pSrbXtensn = (PSRB_EXTENSION)pSRB->SrbExDataOffset; // SrbExtension;
	ULONG                SrbFunxn = pSRB->SrbFunction;
	BOOLEAN              bRC = TRUE; //Assuming we pass the Srb on to StartIo

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDiskBuildIo... SerialNo=%d pSRB=%p SrbFunxn=%d\n",
		        pDevXtensn->SerialNo, pDevXtensn, pSRB, SrbFunxn);

	if (pSrbXtensn == NULL)
	    {
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadDiskBuildIo: called with bogus SRB set.\n");
		pSRB->SrbStatus = SRB_STATUS_INVALID_REQUEST;
		StorPortNotification(RequestComplete, pDevXtensn, pSRB);
		return FALSE; // indicate that we are done with SRB, don't send it to StartIo
	    }

	switch (SrbFunxn)
	    {
	    case SRB_FUNCTION_EXECUTE_SCSI:
		    RtlZeroMemory(pSrbXtensn, sizeof(SRB_EXTENSION));
		    break;

	    case SRB_FUNCTION_IO_CONTROL:
	    case SRB_FUNCTION_RESET_LOGICAL_UNIT:
	    case SRB_FUNCTION_RESET_DEVICE:
	    case SRB_FUNCTION_RESET_BUS:
	    case SRB_FUNCTION_FLUSH:
	    case SRB_FUNCTION_DUMP_POINTERS:
	    case SRB_FUNCTION_FREE_DUMP_POINTERS:
		    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			"MadDiskBuildIo... SerialNo=%d SrbFunxn=%d - rc=TRUE pSRB=%p\n",
			            pDevXtensn->SerialNo, SrbFunxn, pSRB);
		    break;

	    default:
		    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			            "MadDiskBuildIo: called with bogus Srb SrbFunxn (%d)\n", SrbFunxn);
		    pSRB->SrbStatus = SRB_STATUS_INVALID_REQUEST;
		    StorPortNotification(RequestComplete, pDevXtensn, pSRB);
		    bRC = FALSE; // indicate that we are done with SRB, don't send it to StartIo
	}

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDiskBuildIo normal exit... returning (%d)\n", bRC);

	return(bRC);    // If ready for StartIo, return TRUE
}

/*++
Routine Description:
This routine receives requests from the port driver after they have been
processed by BuildIo.  This routine is synchronized with the StartIo lock
so shared memory (for the StartIo thread) can be modified.

Arguments:
Context - pointer to the device extension for the adapter.
Srb - pointer to the request to be started.

Return Value:
TRUE - the request was accepted.
FALSE - the request must be submitted later.
--*/
BOOLEAN MadDiskStartIo(IN PVOID Context, IN PSCSI_REQUEST_BLOCK pScsiRB)
{
PMAD_HDW_DEVICE_EXTENSION pDevXtensn = (PMAD_HDW_DEVICE_EXTENSION)Context;
PSTORAGE_REQUEST_BLOCK pSRB = (PSTORAGE_REQUEST_BLOCK)pScsiRB;
ULONG SrbFunxn = pSRB->SrbFunction;
//PMAD_DISK_LU_EXTENSION pMadLuExt =
//(PMAD_DISK_LU_EXTENSION)StorPortGetLogicalUnit(pDevXtensn, pSRB->PathId,
//	                                           pSRB->TargetId, pSRB->Lun); 
BOOLEAN bInit = TRUE;
BOOLEAN bSrbDone = TRUE;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	            "MadDiskStartIo... SerialNo=%d SRB=%p SrbFunxn=x%X\n",
		        pDevXtensn->SerialNo, pSRB, SrbFunxn);

	ASSERT(pSRB->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK);
	pSRB->SrbStatus = SRB_STATUS_PENDING; //Until completed

	switch (SrbFunxn)
	    {
	    case SRB_FUNCTION_EXECUTE_SCSI:
			bInit = MadDiskInitScsiRequest(pDevXtensn, pSRB, &bSrbDone);
		    break;

		case SRB_FUNCTION_IO_CONTROL:
		    {bInit = MadDiskExecSrbIoControl(pDevXtensn, pSRB, &bSrbDone);}
			break;

	    case SRB_FUNCTION_RESET_LOGICAL_UNIT:
	    case SRB_FUNCTION_RESET_DEVICE:
			pSRB->SrbStatus = SRB_STATUS_SUCCESS;
	    	break;

	    case SRB_FUNCTION_RESET_BUS:
		    pSRB->SrbStatus = SRB_STATUS_BUS_RESET;
		    break;

		case SRB_FUNCTION_ABORT_COMMAND: //Our prototype can't play in this league
		case SRB_FUNCTION_TERMINATE_IO:
			pSRB->SrbStatus = SRB_STATUS_ABORT_FAILED; //SRB_STATUS_ERROR, SRB_STATUS_INVALID_REQUEST ...?
			break;

		case SRB_FUNCTION_PNP: 
			bSrbDone = MadDiskStartIoPnp(pDevXtensn, pSRB);
			break;

		case SRB_FUNCTION_POWER:
		    {
			PSCSI_POWER_REQUEST_BLOCK pPowerSrb = (PSCSI_POWER_REQUEST_BLOCK)pSRB;
			TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			"MadDiskMP(%2x) MadDiskStartIO: POWER request received... SerialNo=%d power-state=%d\n",
				        pDevXtensn->SerialNo, pPowerSrb->DevicePowerState);

			if (pPowerSrb->DevicePowerState == StorPowerDeviceD0)
			    {
				pDevXtensn->StopAdapter = FALSE; // Reset StopAdapter flag

                #ifdef _MAD_REAL_HARDWARE_MODE_ //Needs work
				pSRB->SrbStatus = SRB_STATUS_BAD_FUNCTION; 
				#else //_MAD_SIMULATION_MODE_
				PFN_BUS_SET_POWER_UP pSetPowerUpEvFunxn =
				pDevXtensn->pMadSimIntParms->u.StorIntParms.pSetPowerUpEvFunxn;
				pSetPowerUpEvFunxn(pDevXtensn->SerialNo);
				pSRB->SrbStatus = SRB_STATUS_SUCCESS;
                #endif //_MAD_SIMULATION_MODE_
				break;
			    }

			if (pPowerSrb->DevicePowerState == StorPowerDeviceD3)
			    {
				pDevXtensn->StopAdapter = TRUE; // set StopAdapter flag

                #ifdef _MAD_REAL_HARDWARE_MODE_
			    pSRB->SrbStatus = SRB_STATUS_BAD_FUNCTION; //Until whenever
                #else //_MAD_SIMULATION_MODE_
				PFN_BUS_SET_POWER_DOWN pSetPowerDownEvFunxn =
				pDevXtensn->pMadSimIntParms->u.StorIntParms.pSetPowerDownEvFunxn;
				pSetPowerDownEvFunxn(pDevXtensn->SerialNo);
				pSRB->SrbStatus = SRB_STATUS_NOT_POWERED;
                #endif //_MAD_SIMULATION_MODE_
			    }
		    }
			break;

		case SRB_FUNCTION_WMI:
	    default:
			pSRB->SrbStatus = SRB_STATUS_INVALID_REQUEST;
	    }

	if (bSrbDone)
	    {
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		"MadDiskStartIo... SerialNo=%d completing SRB=%p SrbFunxn=x%X DataLen=%d SrbStatus=x%X\n\n",
			        pDevXtensn->SerialNo, pSRB, SrbFunxn,
			        pSRB->DataTransferLength, pSRB->SrbStatus);

		pDevXtensn->pActiveSrb = NULL;
		StorPortNotification(RequestComplete, pDevXtensn, pSRB);
	    }
	else
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		            "MadDiskStartIo exit... SerialNo=%d SRB=%p SrbStatus=x%X  PENDING\n",
		             pDevXtensn->SerialNo, pSRB, pSRB->SrbStatus);

	return bInit;
} 

BOOLEAN MadDiskStartIoPnp(PMAD_HDW_DEVICE_EXTENSION pDevXtensn,
	                      IN PSTORAGE_REQUEST_BLOCK pPnpSrb)
{
	PSRBEX_DATA_PNP pPnpSrbEx = 
	(PSRBEX_DATA_PNP)SrbGetSrbExDataByType(pPnpSrb, SrbExDataTypePnP);
	UCHAR PnpSubFunxn;
	STOR_PNP_ACTION PnpAction = -1;
	ULONG PnpFlags = 0;
	SRBEXDATATYPE Type = SrbExDataTypeUnknown;
	BOOLEAN bSrbDone = TRUE;

	if (pPnpSrbEx != NULL)
	    {
		PnpSubFunxn = pPnpSrbEx->PnPSubFunction;
		PnpAction   = pPnpSrbEx->PnPAction;
		PnpFlags    = pPnpSrbEx->SrbPnPFlags;
		Type        = pPnpSrbEx->Type;
	    }

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	"MadDiskStartIoPnp... SerialNo=%d Type=%d SubFunxn=%d Action=%d Flags=x%X\n",
		        pDevXtensn->SerialNo, Type, PnpSubFunxn, PnpAction, PnpFlags);

	if (pPnpSrbEx == NULL)
		return FALSE;

	switch (PnpAction)
	    {
	    case StorStartDevice:
	    case StorRemoveDevice:
	    case StorStopDevice:
	    case StorSurpriseRemoval:
            #ifdef _MAD_SIMULATION_MODE_
			       pPnpSrb->SrbStatus = SRB_STATUS_SUCCESS;
            #else
			      //Any real-hardware Pnp minor function needs work
			      pPnpSrb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
            #endif
			break;

		case StorQueryCapabilities:
		case StorQueryResourceRequirements:
		case StorFilterResourceRequirements:
			pPnpSrb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
			break;

		default:
			pPnpSrb->SrbStatus = SRB_STATUS_INVALID_PARAMETER;
			TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				        "MadDiskStartIoPnp... SerialNo=%d unknown pnpAction Status=%d\n",
				        pDevXtensn->SerialNo, pPnpSrb->SrbStatus);
	    }

	return bSrbDone;
}

BOOLEAN MadDiskExecSrbIoControl(PMAD_HDW_DEVICE_EXTENSION pDevXtensn,
	                            PSTORAGE_REQUEST_BLOCK pSRB,
	                            BOOLEAN* bSrbDone)
{
	PSRB_IO_CONTROL pSrbIoCtrl = (PSRB_IO_CONTROL)pSRB->DataBuffer;
	ULONG CtlCode = pSrbIoCtrl->ControlCode;
	ULONG CtlDevType = DEVICE_TYPE_FROM_CTL_CODE(CtlCode);
	ULONG CtlFunction = FUNCTION_TYPE_FROM_CTL_CODE(CtlCode);
	ULONG BufrLen = pSRB->DataTransferLength;
	ULONG DataLen = pSrbIoCtrl->Length; // BufrLen - sizeof(SRB_IO_CONTROL);
	PVOID DataPntr = (PVOID)((PUCHAR)pSrbIoCtrl + sizeof(SRB_IO_CONTROL));

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	"MadDiskExecSrbIoControl... SerialNo=%d CtlCode=x%X DT=%d Funxn=x%X SrbBufrLen=%d IoCtlDataLen=%d\n",
		        pDevXtensn->SerialNo, CtlCode, CtlDevType, CtlFunction, 
                BufrLen, DataLen);
	
	switch (CtlCode)
	    {
	    case IOCTL_SCSI_MINIPORT_IDENTIFY:
			if (DataLen < sizeof(IDENTIFY_DEVICE_DATA))
			    {
				pSRB->SrbStatus = SRB_STATUS_INVALID_PARAMETER;
				break;
			    }

			StorPortCopyMemory(DataPntr, &pDevXtensn->IdDeviceData, 
				               sizeof(IDENTIFY_DEVICE_DATA));
			pSRB->SrbStatus = SRB_STATUS_SUCCESS;
			break;

		case IOCTL_STORAGE_QUERY_PROPERTY:
		    {
			if (DataLen < sizeof(STORAGE_PROPERTY_QUERY))
				{
				pSRB->SrbStatus = SRB_STATUS_INVALID_PARAMETER;
				break;
			    }

			PSTORAGE_PROPERTY_QUERY pPropQuery = 
				                    (PSTORAGE_PROPERTY_QUERY)DataPntr;
			TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			"MadDiskExecSrbIoControl... Query_property QueryType:PropId=%d:%d\n",
				        pPropQuery->QueryType, pPropQuery->PropertyId);

			if ((pPropQuery->QueryType >= PropertyQueryMaxDefined) || 
				(pPropQuery->PropertyId > StorageFruIdProperty))
			    {
				pSRB->SrbStatus = SRB_STATUS_INVALID_PARAMETER;
				break;
			    }

			pSRB->SrbStatus = SRB_STATUS_INVALID_REQUEST;
			break;
		    }

        //Unknown ioctls (private? - IHV) 
		case 0x1B0620:
		case 0x1B0780:
			TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			"MadDiskExecSrbIoControl... undocumented srb_io_control (x%X)\n",
				        CtlCode);
			pSRB->SrbStatus = SRB_STATUS_INVALID_REQUEST;
			break;

	    default:
			pSRB->SrbStatus = SRB_STATUS_BAD_FUNCTION;
		    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			"MadDiskExecSrbIoControl... invalid srb_io_control (x%X)\n",
			            CtlCode);
	    }

	*bSrbDone = TRUE;
	return TRUE;
}

//Here is where we program the device
BOOLEAN MadDiskInitScsiRequest(PMAD_HDW_DEVICE_EXTENSION pDevXtensn, 
	                           PSTORAGE_REQUEST_BLOCK pSRB, BOOLEAN* bSrbDone)
{
UCHAR CdbLength8;
ULONG CdbLength32;
UCHAR ScsiStatus;
PVOID pSenseInfoBuffer;
UCHAR SenseInfoBufferLength;
PCDB pCDB = SrbGetScsiData(pSRB, &CdbLength8, &CdbLength32, &ScsiStatus,
						   &pSenseInfoBuffer, &SenseInfoBufferLength);
//UCHAR Lun    = pCDB->CDB6GENERIC.LogicalUnitNumber;
ULONG CdbLen = SrbGetCdbLength(pSRB);
PUCHAR pCdb = (PUCHAR)pCDB;
BOOLEAN bInitDone = TRUE;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	 	        "MadDiskInitScsiRequest enter... SerialNo=%d pSRB=%p CdbLen=%d\n",
		        pDevXtensn->SerialNo, pSRB, CdbLen);

	if (pDevXtensn->pActiveSrb != NULL)
	    {
		pSRB->SrbStatus = SRB_STATUS_BUSY;
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		"MadDiskInitScsiRequest... SerialNo=%d an SRB is active - returning SrbStatus=%d\n",
			        pDevXtensn->SerialNo, pSRB->SrbStatus);
		*bSrbDone = TRUE;
		return FALSE; //The request must be submitted later
	    }

	/*if (pMadLuExt == NULL)
	    {
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadDiskInitScsiRequest... Null pointer to LUN! SerialNo=%d pSRB=%p\n",
			        pDevXtensn->SerialNo, pSRB);
		//pSRB->SrbStatus = SRB_STATUS_NO_DEVICE;
		//*bSrbDone = TRUE;
		//return FALSE;
	    } */

	*bSrbDone = FALSE;
	pSRB->SrbStatus = SRB_STATUS_PENDING;
	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	"MadDiskInitScsiRequest... CdbBytes x%X x%X x%X x%X x%X x%X x%X x%X x%X x%X\n",
		        pCdb[0], pCdb[1], pCdb[2], pCdb[3], pCdb[4],
		        pCdb[5], pCdb[6], pCdb[7], pCdb[8], pCdb[9]);

	bInitDone = MadDiskScsiProcess(pDevXtensn, pSRB, pCDB);
	if (pSRB->SrbStatus == SRB_STATUS_PENDING)
	    {
		pDevXtensn->pActiveSrb = pSRB;
		BOOLEAN bRC = StorPortBusy(pDevXtensn, MAD_MP_Q_LIMIT);
		if (!bRC)
			TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			"MadDiskInitScsiRequest:StorPortBusy returned false!... SerialNo=%d ActiveSrb=%p\n",
				        pDevXtensn->SerialNo, pSRB);
	    }
	else
	    {*bSrbDone = TRUE;}

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDiskInitScsiRequest... SerialNo=%d SrbSstatus=x%X\n",
				 pDevXtensn->SerialNo, pSRB->SrbStatus);
	
	return bInitDone; //Initializing the SRB-request is completed
}

BOOLEAN  MadDiskScsiProcess(PMAD_HDW_DEVICE_EXTENSION pDevXtensn,
	                        PSTORAGE_REQUEST_BLOCK pSRB, PCDB pCDB)
{
UCHAR   OpCode = pCDB->CDB6GENERIC.OperationCode;
BOOLEAN bInitDone = TRUE;
ULONG SpStatus = SRB_STATUS_SUCCESS;
PVOID pVoid = NULL;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	"MadDiskScsiProcess enter... SerialNo=%d OpCode=x%X XferLen=%d\n",
	            pDevXtensn->SerialNo, OpCode, pSRB->DataTransferLength);

	SpStatus =
	StorPortGetSystemAddress(pDevXtensn, (PSCSI_REQUEST_BLOCK)pSRB, &pVoid);

    switch (OpCode)
        {
	    case SCSIOP_TEST_UNIT_READY:
		    {
			PVOID pTUR = pVoid;
			if (pTUR != NULL)
			    RtlZeroMemory(pTUR, pSRB->DataTransferLength);
			pSRB->SrbStatus = SRB_STATUS_SUCCESS;
			break;
		    }

		case SCSIOP_READ_CAPACITY: 
		case SCSIOP_READ_CAPACITY16:
		    {
			PREAD_CAPACITY_DATA pCapsData = pVoid;
			if (pCapsData == NULL)
			    {
				SpStatus = SRB_STATUS_INVALID_PARAMETER;
				pSRB->SrbStatus = (UCHAR)SpStatus;
				break;
			    }

			ULONG Len = (OpCode == SCSIOP_READ_CAPACITY) ? sizeof(READ_CAPACITY_DATA) :
				                                           sizeof(READ_CAPACITY16_DATA);
			RtlZeroMemory(pCapsData, Len);
			if (OpCode == SCSIOP_READ_CAPACITY)
			    {
				pCapsData->LogicalBlockAddress = MAD_DEVICE_MAX_SECTORS;
				pCapsData->BytesPerBlock = MAD_SECTOR_SIZE;
			    }
			else
			    {
				PREAD_CAPACITY16_DATA pCaps16Data = (PREAD_CAPACITY16_DATA)pCapsData;
				pCaps16Data->LogicalBlockAddress.LowPart = MAD_DEVICE_MAX_SECTORS;
				pCaps16Data->BytesPerBlock = MAD_SECTOR_SIZE;
			    }
			pSRB->DataTransferLength = Len;
			pSRB->SrbStatus = SRB_STATUS_SUCCESS;
			break;
		    }

		case SCSIOP_FORMAT_UNIT:
			pSRB->SrbStatus = SRB_STATUS_SUCCESS;
			break;

		case SCSIOP_MODE_SENSE: // 0x1A
		case SCSIOP_MODE_SENSE10:
		    {
			PUCHAR pBuffer = pVoid;
			if (pBuffer == NULL)
			    {
				SpStatus = SRB_STATUS_INVALID_PARAMETER;
				pSRB->SrbStatus = (UCHAR)SpStatus;
				break;
			    }
			//SpStatus = MadDiskScsiSetModeSense(pDevXtensn, pSRB, pCDB,
			//	                              (PMODE_PARAMETER_HEADER)pBuffer);
			SpStatus = SRB_STATUS_INVALID_REQUEST;
			pSRB->SrbStatus = (UCHAR)SpStatus;
			break;
		    }

		case SCSIOP_REPORT_LUNS:
			SpStatus = MadDiskScsiSetErrorSenseData(pDevXtensn, pSRB, OpCode);
			break;

		case SCSIOP_INQUIRY:
		    {
		    PCDB pCdb = pCDB; // (PCDB)&pSRB->Cdb;
			PINQUIRYDATA  pBuffer = pVoid;
			
			TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			"MadDiskScsiProcess enter:SCSIOP_INQUIRY... SerialNo=%d EVPD=%d PgCode=%d\n",
				        pDevXtensn->SerialNo,
				        pCdb->CDB6INQUIRY3.EnableVitalProductData,
				        pCdb->CDB6INQUIRY3.PageCode);

			if ((pBuffer == NULL) || (pSRB->DataTransferLength < INQUIRYDATABUFFERSIZE))
			    {
				SpStatus = SRB_STATUS_INVALID_PARAMETER;
				pSRB->SrbStatus = (UCHAR)SpStatus;
				break;
			    }

			if (pCdb->CDB6INQUIRY3.EnableVitalProductData)
			    {
				SpStatus = MadDiskScsiSetErrorSenseData(pDevXtensn, pSRB, OpCode);
				break;
			    }

			if (pCdb->CDB6INQUIRY3.PageCode > 0)
			    {
				SpStatus = MadDiskScsiSetErrorSenseData(pDevXtensn, pSRB, OpCode);
				break;
			    }

			if ((pDevXtensn->MadVmDisk.pInquiryData == NULL) || 
				(pDevXtensn->ConnectListEntry.bConnected == FALSE))
			    {
				pSRB->DataTransferLength = 0;
				pSRB->SrbStatus = SRB_STATUS_NO_DEVICE;
			    }
			else
			    {
				StorPortCopyMemory(pBuffer,
					               pDevXtensn->MadVmDisk.pInquiryData,
					               INQUIRYDATABUFFERSIZE);
				pSRB->DataTransferLength = INQUIRYDATABUFFERSIZE;
				pSRB->SrbStatus = SRB_STATUS_SUCCESS;
			    }
			break;
		    }

		case SCSIOP_WRITE6:
		case SCSIOP_VERIFY6: 
		case SCSIOP_READ6:
			bInitDone = MadDiskScsiProcessCDB6(pDevXtensn, pSRB, pCDB, OpCode);
			break;

		case SCSIOP_WRITE:
		case SCSIOP_WRITE_VERIFY:
		case SCSIOP_READ:
			bInitDone = MadDiskScsiProcessCDB10(pDevXtensn, pSRB, pCDB, OpCode);
			break;

		case SCSIOP_WRITE12:
		case SCSIOP_WRITE_VERIFY12:
		case SCSIOP_READ12:
			bInitDone = MadDiskScsiProcessCDB12(pDevXtensn, pSRB, pCDB, OpCode);
			break;

		case SCSIOP_WRITE16:
		case SCSIOP_WRITE_VERIFY16:
		case SCSIOP_READ16:
			bInitDone = MadDiskScsiProcessCDB16(pDevXtensn, pSRB, pCDB, OpCode);
			break;

		case SCSIOP_REWIND: // 0x01
		case SCSIOP_REQUEST_BLOCK_ADDR: // 0x02
		case SCSIOP_REASSIGN_BLOCKS:
		case SCSIOP_READ_BLOCK_LIMITS: // 0x05
		case SCSIOP_TRACK_SELECT: // 0x0B
		case SCSIOP_SEEK_BLOCK: // 0x0C
		case SCSIOP_PARTITION: // 0x0D
		case SCSIOP_READ_REVERSE: // 0x0F
		case SCSIOP_FLUSH_BUFFER: // 0x10
		case SCSIOP_SPACE: // 0x11
		case SCSIOP_RECOVER_BUF_DATA: // 0x14
		case SCSIOP_MODE_SELECT: // 0x15
		case SCSIOP_RELEASE_UNIT:
		case SCSIOP_COPY: // 0x18
		case SCSIOP_ERASE: // 0x19
		case SCSIOP_RECEIVE_DIAGNOSTIC: // 0x1C
		case SCSIOP_SEND_DIAGNOSTIC: // 0x1D
		case SCSIOP_MEDIUM_REMOVAL: // 0x1E
		case SCSIOP_READ_FORMATTED_CAPACITY: // 0x23
		case SCSIOP_SEEK:
		case SCSIOP_SEARCH_DATA_HIGH: // 0x30
		case SCSIOP_SEARCH_DATA_EQUAL: // 0x31
		case SCSIOP_SEARCH_DATA_LOW: // 0x32
		case SCSIOP_SET_LIMITS: // 0x33
		case SCSIOP_READ_POSITION: // 0x34
		case SCSIOP_SYNCHRONIZE_CACHE: // 0x35
		case SCSIOP_COMPARE: // 0x39
		case SCSIOP_COPY_COMPARE: // 0x3A
		case SCSIOP_WRITE_DATA_BUFF: // 0x3B
		case SCSIOP_READ_DATA_BUFF: // 0x3C
		case SCSIOP_CHANGE_DEFINITION: // 0x40
		case SCSIOP_READ_SUB_CHANNEL: // 0x42
		case SCSIOP_READ_TOC:             // 0x43
		case SCSIOP_READ_HEADER: // 0x44
		case SCSIOP_PLAY_AUDIO: // 0x45
		case SCSIOP_PLAY_AUDIO_MSF: // 0x47
		case SCSIOP_PLAY_TRACK_INDEX: // 0x48
		case SCSIOP_PLAY_TRACK_RELATIVE: // 0x49
		case SCSIOP_PAUSE_RESUME: // 0x4B
		case SCSIOP_LOG_SELECT: // 0x4C
		case SCSIOP_LOG_SENSE: // 0x4D
		case SCSIOP_STOP_PLAY_SCAN: // 0x4E
		case SCSIOP_READ_DISK_INFORMATION: // 0x51
		case SCSIOP_READ_TRACK_INFORMATION: // 0x52
		case SCSIOP_MODE_SELECT10: // 0x55
		//case SCSIOP_MODE_SENSE10: // 0x5A
		case SCSIOP_SEND_KEY: // 0xA3
		case SCSIOP_REPORT_KEY: // 0xA4
		case SCSIOP_MOVE_MEDIUM: // 0xA5
		case SCSIOP_EXCHANGE_MEDIUM: // 0xA6
		case SCSIOP_SET_READ_AHEAD: // 0xA7
		case SCSIOP_READ_DVD_STRUCTURE: // 0xAD
		case SCSIOP_REQUEST_VOL_ELEMENT: // 0xB5
		case SCSIOP_SEND_VOLUME_TAG: // 0xB6
		case SCSIOP_READ_ELEMENT_STATUS: // 0xB8
		case SCSIOP_READ_CD_MSF: // 0xB9
		case SCSIOP_SCAN_CD: // 0xBA
		case SCSIOP_PLAY_CD: // 0xBC
		case SCSIOP_MECHANISM_STATUS: // 0xBD
		case SCSIOP_READ_CD: // 0xBE
		case SCSIOP_INIT_ELEMENT_RANGE: // 0xE7
		case SCSIOP_REQUEST_SENSE:
			SpStatus = MadDiskScsiSetErrorSenseData(pDevXtensn, pSRB, OpCode);
			pSRB->SrbStatus = (UCHAR)SpStatus;
			break;

		default:
			TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			"MadDiskScsiProcess... invalid scsi CDB opcode SerialNo=%d OpCode=x%X\n",
				        pDevXtensn->SerialNo, OpCode);
			pSRB->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        }

	return bInitDone;
}

BOOLEAN  MadDiskScsiProcessCDB6(PMAD_HDW_DEVICE_EXTENSION pDevXtensn,
	                            PSTORAGE_REQUEST_BLOCK pSRB, PCDB pCDB, UCHAR OpCode)
{
ULONG   XferBlocks;
ULONG   LogicalBlock;
BOOLEAN bValidOC = ((OpCode == SCSIOP_WRITE6) || (OpCode == SCSIOP_READ6));
BOOLEAN bWrite = (OpCode == SCSIOP_WRITE6);
//BOOLEAN bInitDone = TRUE;

    if (!bValidOC)
        {
	    pSRB->SrbStatus = SRB_STATUS_INVALID_REQUEST;
	    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		"MadDiskScsiProcessCDB6... invalid scsi CDB6 i/o opcode SerialNo=%d OpCode=x%X\n",
		            pDevXtensn->SerialNo, OpCode);
	    return TRUE;
        }

	XferBlocks   = pCDB->CDB6READWRITE.TransferBlocks;
    LogicalBlock = (pCDB->CDB6READWRITE.LogicalBlockMsb1 >> 3);
    LogicalBlock = (LogicalBlock << 8) + pCDB->CDB6READWRITE.LogicalBlockMsb0;
    LogicalBlock = (LogicalBlock << 8) + pCDB->CDB6READWRITE.LogicalBlockLsb;

	if ((XferBlocks + LogicalBlock) >= MAD_DEVICE_MAX_SECTORS)
	    { 
		pSRB->SrbStatus = SRB_STATUS_DATA_OVERRUN;
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		"MadDiskScsiProcessCDB6 enter... SerialNo=%d OpCode=x%X LBA+XferLen > Device extent\n",
			        pDevXtensn->SerialNo, OpCode);
		return TRUE;
	    }

    return MadDiskScsiInitDmaIo(pDevXtensn, pSRB, LogicalBlock, XferBlocks, bWrite);
}

BOOLEAN  MadDiskScsiProcessCDB10(PMAD_HDW_DEVICE_EXTENSION pDevXtensn, 
	                             PSTORAGE_REQUEST_BLOCK pSRB, PCDB pCDB, UCHAR OpCode)
{
//BOOLEAN bRead = (OpCode == SCSIOP_READ);
BOOLEAN bWrite = (OpCode != SCSIOP_READ) ? TRUE : FALSE;
BOOLEAN bValidOC = ((OpCode == SCSIOP_READ) ||
	                (OpCode == SCSIOP_WRITE) || (OpCode == SCSIOP_WRITE_VERIFY));
ULONG   XferBlocks;
ULONG   LogicalBlock;
//BOOLEAN bInitDone = TRUE;

    //TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	//"MadDiskScsiProcessCDB10 enter... SerialNo=%d OpCode=x%X XferMsb=%d XferLsb=%d\n",
	//           pDevXtensn->SerialNo, OpCode,
	//	        pCDB->CDB10.TransferBlocksMsb, pCDB->CDB10.TransferBlocksLsb);

    if (!bValidOC)
        {
		pSRB->SrbStatus = SRB_STATUS_INVALID_REQUEST;
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		"MadDiskScsiProcessCDB10... invalid scsi CDB10 i/o opcode SerialNo=%d OpCode=x%X\n",
			        pDevXtensn->SerialNo, OpCode);
		return TRUE;
        }

	XferBlocks   = ((USHORT)pCDB->CDB10.TransferBlocksMsb << 8) + 
				    pCDB->CDB10.TransferBlocksLsb;
	LogicalBlock =  (((ULONG)pCDB->CDB10.LogicalBlockByte0) << 24) +
				    (((ULONG)pCDB->CDB10.LogicalBlockByte1) << 16) +
				    (((ULONG)pCDB->CDB10.LogicalBlockByte2) <<  8) +
				    pCDB->CDB10.LogicalBlockByte3;

	if ((XferBlocks + LogicalBlock) >= MAD_DEVICE_MAX_SECTORS)
	    {
		pSRB->SrbStatus = SRB_STATUS_DATA_OVERRUN;
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		"MadDiskScsiProcessCDB10 enter... SerialNo=%d OpCode=x%X LBA+XferLen > Device extent\n",
			        pDevXtensn->SerialNo, OpCode);
		return TRUE;
	    }
	
	return MadDiskScsiInitDmaIo(pDevXtensn, pSRB, 
				                LogicalBlock, XferBlocks, bWrite);
}

BOOLEAN  MadDiskScsiProcessCDB12(PMAD_HDW_DEVICE_EXTENSION pDevXtensn, 
	                             PSTORAGE_REQUEST_BLOCK pSRB, PCDB pCDB, UCHAR OpCode)
{
BOOLEAN bWrite = (OpCode != SCSIOP_READ12) ? TRUE : FALSE;
BOOLEAN bValidOC = ((OpCode == SCSIOP_READ12) ||
		            (OpCode == SCSIOP_WRITE12) || (OpCode == SCSIOP_WRITE_VERIFY12));
ULONG   XferBlocks;
ULONG   LogicalBlock;
//BOOLEAN bInitDone = TRUE;

    //TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	//            "MadDiskScsiProcessCDB12 enter... SerialNo=%d  OpCode=x%X\n",
	//            pDevXtensn->SerialNo, OpCode);

    if (!bValidOC)
        {
	    pSRB->SrbStatus = SRB_STATUS_INVALID_REQUEST;
	    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		"MadDiskScsiProcessCDB10... invalid scsi CDB12 i/o opcode SerialNo=%d OpCode=x%X\n",
		           pDevXtensn->SerialNo, OpCode);
	     return TRUE;
        }

	XferBlocks = ((ULONG)pCDB->CDB12.TransferLength[0] << 24) +
		         ((ULONG)pCDB->CDB12.TransferLength[1] << 16) +
		         ((ULONG)pCDB->CDB12.TransferLength[2] << 8) +
		         pCDB->CDB12.TransferLength[3];

	LogicalBlock = ((ULONG)pCDB->CDB12.LogicalBlock[0] << 24) +
		           ((ULONG)pCDB->CDB12.LogicalBlock[1] << 16) +
		           ((ULONG)pCDB->CDB12.LogicalBlock[2] << 8) +
		           pCDB->CDB12.LogicalBlock[3];

	if ((XferBlocks + LogicalBlock) >= MAD_DEVICE_MAX_SECTORS)
	    {
		pSRB->SrbStatus = SRB_STATUS_DATA_OVERRUN;
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		"MadDiskScsiProcessCDB10 enter... SerialNo=%d OpCode=x%X LBA+XferLen > Device extent\n",
			        pDevXtensn->SerialNo, OpCode);
		return TRUE;
	    }

	return MadDiskScsiInitDmaIo(pDevXtensn, pSRB, LogicalBlock, XferBlocks, bWrite);
}

BOOLEAN  MadDiskScsiProcessCDB16(PMAD_HDW_DEVICE_EXTENSION pDevXtensn, 
	                             PSTORAGE_REQUEST_BLOCK pSRB, PCDB pCDB, UCHAR OpCode)
{
//BOOLEAN bRead = (OpCode == SCSIOP_READ16);
BOOLEAN bWrite = (OpCode != SCSIOP_READ16) ? TRUE : FALSE;
BOOLEAN bValidOC = ((OpCode == SCSIOP_READ16) ||
		           (OpCode == SCSIOP_WRITE16) || (OpCode == SCSIOP_WRITE_VERIFY16));
ULONG   XferBlocks;
ULONG   LogicalBlock;
//BOOLEAN bInitDone = TRUE;

    //TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	//            "MadDiskScsiProcessCDB16 enter... SerialNo=%d OpCode=%d\n",
	//            pDevXtensn->SerialNo, OpCode);

    if (!bValidOC)
        {
	    pSRB->SrbStatus = SRB_STATUS_INVALID_REQUEST;
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		"MadDiskScsiProcessCDB16... invalid scsi CDB16 i/o-opcode SerialNo=%d OpCode=%d\n",
			        pDevXtensn->SerialNo, OpCode);
		return TRUE;
	}

	XferBlocks =  ((ULONG)pCDB->CDB16.TransferLength[0] << 24) +
		          ((ULONG)pCDB->CDB16.TransferLength[1] << 16) +
		          ((ULONG)pCDB->CDB16.TransferLength[2] << 8) +
				  pCDB->CDB16.TransferLength[3];

	//Constrained to 4G blocks (32 bits)
	LogicalBlock = ((ULONG)pCDB->CDB16.LogicalBlock[4] << 24) +
		           ((ULONG)pCDB->CDB16.LogicalBlock[5] << 16) +
		           ((ULONG)pCDB->CDB16.LogicalBlock[6] << 8) +
				   pCDB->CDB16.LogicalBlock[7];

	if ((XferBlocks + LogicalBlock) >= MAD_DEVICE_MAX_SECTORS)
	    {
		pSRB->SrbStatus = SRB_STATUS_DATA_OVERRUN;
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		"MadDiskScsiProcessCDB10 enter... SerialNo=%d OpCode=x%X LBA+XferLen > Device extent\n",
			       pDevXtensn->SerialNo, OpCode);
		return TRUE;
	    }

	return MadDiskScsiInitDmaIo(pDevXtensn, pSRB, LogicalBlock, XferBlocks, bWrite);
}

BOOLEAN MadDiskScsiInitDmaIo(PMAD_HDW_DEVICE_EXTENSION pDevXtensn, 
	                         PSTORAGE_REQUEST_BLOCK pSRB,
	                         ULONG LogicalBlock, ULONG XferBlocks, BOOLEAN bWrite)
{
static ULONG AllocSize = 
       sizeof(STOR_SCATTER_GATHER_ELEMENT) * MAD_DMA_MAX_SGLIST_SIZE + 12;

ULONG XferLen = pSRB->DataTransferLength;
//ULONG XferBlkLen = XferBlocks * MAD_SECTOR_SIZE;
UCHAR SrbStatus = SRB_STATUS_SUCCESS;
PSTOR_SCATTER_GATHER_LIST pSgList = NULL;
PMDL  pMDL;
ULONG StorRc = 0;

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	"MadDiskScsiInitDmaIo... SerialNo=%d LBA=x%X XferBlocks=%d XferLen=%ld Dir=%d\n",
		        pDevXtensn->SerialNo, LogicalBlock, XferBlocks, XferLen, bWrite);

	if (XferBlocks == 0)
		{
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		"MadDiskScsiInitDmaIo... SerialNo=%d Zero-length i/o exit!\n",
					pDevXtensn->SerialNo);
		pSRB->SrbStatus = SRB_STATUS_SUCCESS;
		return TRUE;
		//XferBlocks = XferLen / MAD_SECTOR_SIZE;
		}

	pSRB->SrbStatus = SRB_STATUS_PENDING;
    StorRc = 
	StorPortGetOriginalMdl(pDevXtensn, (PSCSI_REQUEST_BLOCK)pSRB, &pMDL);
	if (StorRc != STOR_STATUS_SUCCESS)
	    {
		pSRB->SrbStatus = MadDiskConvertStorRc2SrbStatus(StorRc);
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		"MadDiskScsiInitDmaIo:StorPortGetOriginalMdl... SerialNo=%d StorRc=x%X SrbStatus=%d\n",
			        pDevXtensn->SerialNo, StorRc, pSRB->SrbStatus);
		return TRUE;
	    }

	ASSERT(pMDL != NULL);
	StorRc = 
	StorPortAllocatePool(pDevXtensn, AllocSize, MADDISK_POOL_TAG, &pSgList);
	if (StorRc != STOR_STATUS_SUCCESS)
		{
		SrbStatus = MadDiskConvertStorRc2SrbStatus(StorRc);
		pSRB->SrbStatus = SrbStatus;
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		"MadDiskScsiInitDmaIo:StorPortAllocatePool... SerialNo=%d StorRc=x%X SrbStatus=%d\n",
					pDevXtensn->SerialNo, StorRc, SrbStatus);
		return TRUE;
		}
	
	ASSERT(pSgList != NULL);
	//This is ok because we have drip-drip-drip plumbing... no queueing
	pDevXtensn->pOsSgList = pSgList;
	pDevXtensn->CurLogicalBlock = LogicalBlock;
	pDevXtensn->CurXferBlocks = XferBlocks;
	pDevXtensn->bCurWrite = bWrite;
	StorRc = 
	MadDiskBuildScatterGatherList(pDevXtensn, pMDL, pMDL->StartVa, XferLen,
								  MadDiskScsiDmaExecRtn, pDevXtensn, bWrite,
								  pSgList, AllocSize);
	ASSERT(StorRc != STOR_STATUS_INVALID_DEVICE_REQUEST);
	if (StorRc != STOR_STATUS_SUCCESS)
	    {
		SrbStatus = MadDiskConvertStorRc2SrbStatus(StorRc);
		pSRB->SrbStatus = SrbStatus;
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		"MadDiskScsiInitDmaIo:StorPortBuildScatterGatherList... SerialNo=%d StorRc=x%X SrbStatus=%d\n",
				    pDevXtensn->SerialNo, StorRc, SrbStatus);
	    }

	return TRUE; 
}

VOID MadDiskScsiDmaExecRtn(IN PVOID *DeviceObject, IN PVOID *Irp,
						   PSTOR_SCATTER_GATHER_LIST pSgList, PVOID Context)
{
	PMAD_HDW_DEVICE_EXTENSION pDevXtensn = (PMAD_HDW_DEVICE_EXTENSION)Context;
	ULONG          SerialNo = pDevXtensn->SerialNo;
	PMADREGS       pMadRegs = pDevXtensn->pMadRegs;
	ULONG64        DevLoclAddr = (ULONG64)pDevXtensn->CurLogicalBlock * MAD_SECTOR_SIZE;
	ULONG          XferLen     = pDevXtensn->CurXferBlocks * MAD_SECTOR_SIZE;
	BOOLEAN        bWrite      = pDevXtensn->bCurWrite;
	ULONG          IntEnableReg = MAD_INT_STATUS_ALERT_BIT;
	ULONG          ControlReg = 0;
	ULONG          DTBC = 0;
	ULONG          DmaCntl = MAD_DMA_CNTL_INIT;
	BOOLEAN        bChained = (pSgList->NumberOfElements > 1);
	ULONG          TotlXferLen = 0;
	MAD_DMA_CHAIN_ELEMENT    MadOneBlockDma = { 0,  /*Host addr*/
											   0,  /*DevLoclAddr - device offset)*/
											   0,  /*DmaCntl*/
											   0,  /*DTBC*/
											   0 }; /*CDPP*/

	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);

	ASSERT(pSgList == pDevXtensn->pOsSgList);
	if (!bWrite)
		{IntEnableReg |= MAD_INT_DMA_INPUT_BIT;}
	else  
		{
		//DMA write is indicated by two different bits...our device is a little screwy
		IntEnableReg |= MAD_INT_DMA_OUTPUT_BIT;
		DmaCntl |= MAD_DMA_CNTL_H2D; //Host-->Device
		}

	MadOneBlockDma.DevLoclAddr = DevLoclAddr;
	MadOneBlockDma.DmaCntl = DmaCntl;
	MadOneBlockDma.HostAddr = pSgList->List[0].PhysicalAddress.QuadPart;
	DTBC = (ULONG64)max((pDevXtensn->CurXferBlocks * MAD_SECTOR_SIZE), XferLen);

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	"MadDiskScsiDmaExecRtn... SerialNo=%d NumElems=%d HostAddr=x%llX DevAddr=x%llX XferLen=%ld Dir=%d\n",
				SerialNo, pSgList->NumberOfElements,
				MadOneBlockDma.HostAddr, DevLoclAddr, XferLen, bWrite);

	if (!bChained) //One block is sufficient
		{
	    ASSERT(XferLen == pSgList->List[0].Length);
		MadOneBlockDma.DTBC = DTBC;
		TotlXferLen = DTBC;
		MadOneBlockDma.CDPP = (ULONG64)MAD_DMA_CDPP_END;
		}
	else //Set up chained (SG)DMA
		{
		ControlReg |= MAD_CONTROL_CHAINED_DMA_BIT;

		//Set the chained-DMA pointer to the start of the chain & build the hardware's chained DMA list in memory
		MadOneBlockDma.DTBC = (ULONG64)pSgList->List[0].Length;
		MadOneBlockDma.CDPP = pDevXtensn->liCDPP.QuadPart; //The phys-addr pntr to our static pre-allocated chain
		ULONG64 DevOffset = (DevLoclAddr + MadOneBlockDma.DTBC);
		//DbgBreakPoint();
		TotlXferLen =
		MadDiskBuildDevSgListFromOsSgList(pDevXtensn, pSgList, DevOffset, bWrite);
		ASSERT(TotlXferLen == DTBC);
		}

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	"MadDiskScsiDmaExecRtn...SerialNo=%d DTBC=%ld TotlSgXfer=%ld IntEnable=x%X Control=x%X\n",
				SerialNo, MadOneBlockDma.DTBC, TotlXferLen, 
				IntEnableReg, ControlReg);

	StorPortAcquireSpinLock(pDevXtensn, StartIoLock, NULL, &pDevXtensn->hDevLock);

	//Write the 1st/only Hdw SG list item to the device
	StorPortWriteRegisterBufferUlong(pDevXtensn,
									 (PULONG)&pMadRegs->DmaChainItem0.HostAddr,
									 (PULONG)&MadOneBlockDma,
									 (sizeof(MadOneBlockDma) / sizeof(ULONG)));

	StorPortWriteRegisterUlong(pDevXtensn, &pMadRegs->IntEnable, IntEnableReg);
	StorPortWriteRegisterUlong(pDevXtensn, &pMadRegs->Control, ControlReg);

	//Hit the Go bit as a separate register write
	ControlReg |= MAD_CONTROL_DMA_GO_BIT;
	StorPortWriteRegisterUlong(pDevXtensn, &pMadRegs->Control, ControlReg);

	StorPortReleaseSpinLock(pDevXtensn, &pDevXtensn->hDevLock);

	return;
}

/************************************************************************//**
 * Mad_BuildDevSgListFromOsSgList
 *
 * DESCRIPTION:
 *    This function builds a hardware-specific chained-DMA list from the
 *    Windows scatter-gather list.
 *
 * PARAMETERS:
 *     @param[in]  pFdoData    pointer to the framework device extension
 *     @param[in]  pSgList     pointer to the scatter-gather list derived
 *                             from the MDL
 *     @param[in]  bWrite      indicates read or write
 *
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 *    @return      void        nothing returned
 *
***************************************************************************/
ULONG MadDiskBuildDevSgListFromOsSgList(PMAD_HDW_DEVICE_EXTENSION pDevXtensn,
								        PSTOR_SCATTER_GATHER_LIST pSgList,
								        ULONG64 DevOffset,
								        BOOLEAN bWrite)
{
	register ULONG               j;
	PMAD_DMA_CHAIN_ELEMENT       pHdwSgItem = &(pDevXtensn->HdwSgDmaList[0]);
	PMAD_DMA_CHAIN_ELEMENT       pHdwSgNext = &(pDevXtensn->HdwSgDmaList[1]);
	PSTOR_SCATTER_GATHER_ELEMENT pOsSgElement = &(pSgList->List[1]);
	//
	//ULONG64                DevOffset = pHdwSgItem->DevLoclAddr; //Device-relative start of this i/o
	ULONG                    NumElements = pSgList->NumberOfElements - 1;
	ULONG                    TotlXferLen = pSgList->List[0].Length;

	ULONG                    DmaCntlInit = MAD_DMA_CNTL_INIT;
	ULONG                    BlockSize;
	PHYSICAL_ADDRESS         liCDPP;
	ULONG                    MapLen;

	//ASSERT(NumElements > 1); //Else we shouldn't be here
	ASSERT(NumElements <= MAD_DMA_MAX_SGLIST_SIZE); //No more than our Device extent  

	if (bWrite)
		DmaCntlInit |= MAD_DMA_CNTL_H2D; //Host-->Device

	for (j = 1; j <= NumElements; j++)
		{
		pHdwSgItem->HostAddr = pOsSgElement->PhysicalAddress.QuadPart;
		pHdwSgItem->DevLoclAddr = DevOffset;
		pHdwSgItem->DmaCntl = DmaCntlInit;

		BlockSize = pOsSgElement->Length;
		pHdwSgItem->DTBC = BlockSize;

		TotlXferLen += BlockSize;
		DevOffset += BlockSize;

		if (j == NumElements) //No need to set up the next pass so ...
			break;            //exit from loop

		//Set CDPP to next item: Item[X].CDPP --> Item[X+1]
		MapLen = sizeof(MAD_DMA_CHAIN_ELEMENT);
		liCDPP =
		MadDiskGetPhysicalAddress(pDevXtensn, NULL, pHdwSgNext, &MapLen);
		ASSERT(MapLen == sizeof(MAD_DMA_CHAIN_ELEMENT));
		pHdwSgItem->CDPP = liCDPP.QuadPart;  //Can't cast: C2664

		//Advance the list pointers ...
		//We could work with array indeces but the compiler's address calculations would be gnarly
		pOsSgElement++;
		pHdwSgItem++;
		pHdwSgNext++;
		}

	//End the list the way the hardware expects
	//No need to decrement / backup since we broke out of the loop above
	////pHdwSgItem--; //Point to the last element

	pHdwSgItem->CDPP = (ULONG64)MAD_DMA_CDPP_END; //Indicate no forward link 

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	"MadDiskBuildDeviceChainedDmaListFromOsSgList... SerialNo=%d Direxn=%d #Elements=%d TotlXferLen=%d DevOfst(end)=%d\n",
				pDevXtensn->SerialNo, (ULONG)bWrite, NumElements,
				TotlXferLen, (ULONG)DevOffset);

	return TotlXferLen;
}

/*++
Routine Description:
This is the interrupt service routine for the LSI 53C1010 SCSI chip.
This routine checks for interrupt-on-the-fly first, then for either a
DMA or SCSI core interrupt.  All routines return a disposition code
indicating what action to take next.

Arguments:
Context - Supplies a pointer to the device extension for the
interrupting adapter.

Return Value:
TRUE - Indicates that an interrupt was pending on adapter.
FALSE - Indicates the interrupt was not ours.
--*/
BOOLEAN MadDiskISR(PVOID Context)
{
PMAD_HDW_DEVICE_EXTENSION pDevXtensn = (PMAD_HDW_DEVICE_EXTENSION)Context;
PMADREGS pMadRegs = pDevXtensn->pMadRegs;
ULONG SerialNo    = pDevXtensn->SerialNo;
ULONG IntIdReg    = pMadRegs->IntID;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	"MadDiskISR enter... SerialNo=%d registers: Control=x%X Status=x%X IntEnable=x%X IntId=x%X\n",
		    	SerialNo, pMadRegs->Control, pMadRegs->Status, 
		        pMadRegs->IntEnable, IntIdReg);

	// check if adapter is shutdown
	if (pDevXtensn->StopAdapter)
		return(FALSE);

	if (IntIdReg == 0)
	    {
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		"MadDiskMP:MadDiskISR: unidentified interrupt... SerialNo=%d\n",
			        SerialNo);
		DbgBreakPoint();
		return FALSE;
	    }

	if ((IntIdReg & MAD_INT_ALL_INVALID_MASK) != 0)
	    {
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		"MadDiskISR unexpected IntID bit(s) set! no action defined... SerialNo=%d\n",
			        SerialNo);

		pMadRegs->IntID &= MAD_INT_ALL_VALID_MASK;
	    }

	//Process the defined interrupt conditions
	if (IntIdReg & MAD_INT_STATUS_ALERT_BIT)
	    { //Brackets around null stmt in the case of Free_build & No WPP_TRACING(KdPrint)
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadDiskISR...status alert interrupt detected! SerialNo=%d\n",
			        SerialNo);
		//DbgBreakPoint();
	    }

	BOOLEAN bRC = StorPortIssueDpc(pDevXtensn, &pDevXtensn->MadIsrDpc, pMadRegs, NULL);
	if (bRC)
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, "MadDiskISR normal exit\n");
	else
	    {
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadDiskISR failed to enqueue DPC\n");
		pDevXtensn->pActiveSrb = NULL;
	    }

	return(TRUE); // It was/is our interrupt
} 

VOID MadDiskDpc(PSTOR_DPC pDPC, PVOID HwDeviceExtension, 
	            PVOID SysArg1, PVOID SysArg2)
{
static ULONG  IntsDisabled  = MAD_ALL_INTS_DISABLED;
static ULONG  ControlReset = MAD_CONTROL_RESET_STATE;
// 
PMAD_HDW_DEVICE_EXTENSION pDevXtensn = (PMAD_HDW_DEVICE_EXTENSION)HwDeviceExtension;
PSTORAGE_REQUEST_BLOCK pSRB = (PSTORAGE_REQUEST_BLOCK)pDevXtensn->pActiveSrb; //The one & only 
PMADREGS pMadRegs = (PMADREGS)SysArg1;
ULONG StatusReset = 0; // ~pMadRegs->Status;
ULONG IntIdReset = 0; 

	UNREFERENCED_PARAMETER(SysArg2);
	ASSERT(pDPC == &pDevXtensn->MadIsrDpc);

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDiskDpc enter: SerialNo=%d pDPC=%p ActiveSrb=%p\n",
				pDevXtensn->SerialNo, pDPC, pDevXtensn->pActiveSrb);
	//
	pSRB->SrbStatus = SRB_STATUS_SUCCESS; //Until ...

	if ((pMadRegs->IntID & MAD_INT_STATUS_ALERT_BIT) == 0)
		pSRB->DataTransferLength = pMadRegs->DmaChainItem0.DTBC;
	else
	    {
		pSRB->SrbStatus = SRB_STATUS_ERROR;

		ULONG StatusReg = 
		pMadRegs->Status & ~MAD_STATUS_RW_COUNT_MASK; //Mask out the i/o count bits before examining the error status bits
		switch (StatusReg)
		    {
		    case MAD_STATUS_DEVICE_FAILURE_BIT:
		    case MAD_STATUS_DEAD_DEVICE_MASK:
				//Hardware Error - self-test failed http://en.wikipedia.org/wiki/Key_Code_Qualifier
			    pSRB->SrbStatus = SRB_STATUS_NO_DEVICE;
				MadDiskAssignSenseData(pDevXtensn, 4, 0x3E, 3);
			    break;

		    case MAD_STATUS_OVER_UNDER_ERR_BIT:
				//Illegal Request - LBA out of range
				pSRB->SrbStatus = SRB_STATUS_DATA_OVERRUN;
				MadDiskAssignSenseData(pDevXtensn, 5, 0x21, 0);
				break;

		    case MAD_STATUS_DEVICE_BUSY_BIT:
				//Not Ready - becoming ready
				pSRB->SrbStatus = SRB_STATUS_BUSY;
				MadDiskAssignSenseData(pDevXtensn, 2, 0x04, 1);
			    break;

			case MAD_STATUS_TIMEOUT_ERROR_BIT:
				//Not Ready - cause not reportable
				pSRB->SrbStatus = SRB_STATUS_TIMEOUT;
				MadDiskAssignSenseData(pDevXtensn, 2, 0x04, 0);
				break;

			case MAD_STATUS_INVALID_IO_BIT:
				//Illegal Request - invalid/unsupported command code
				MadDiskAssignSenseData(pDevXtensn, 5, 0x20, 0);
				break;

           case MAD_STATUS_NO_ERROR_MASK:
		   case MAD_STATUS_GENERAL_ERR_BIT:
			    //Aborted Command - no additional sense code
			    MadDiskAssignSenseData(pDevXtensn, 0x0B, 0x00, 0x00); 
				break;

			default: //If we got here we don't recognize a status register bit *!*
				TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				"MadDiskDpc invalid device status... SerialNo=%d ActiveSrb=%p StatusReg=x%X\n",
							pDevXtensn->SerialNo, 
					        pDevXtensn->pActiveSrb, pMadRegs->Status);
		    }	
	    }

	//Reset device regs
	StorPortAcquireSpinLock(pDevXtensn, StartIoLock, NULL, &pDevXtensn->hDevLock);
	StorPortWriteRegisterUlong(pDevXtensn, &pMadRegs->IntEnable, IntsDisabled);
	StorPortWriteRegisterUlong(pDevXtensn, &pMadRegs->Status, StatusReset);
	StorPortWriteRegisterUlong(pDevXtensn, &pMadRegs->IntID, IntIdReset);
	StorPortWriteRegisterUlong(pDevXtensn, &pMadRegs->Control, ControlReset);
	StorPortReleaseSpinLock(pDevXtensn, &pDevXtensn->hDevLock);

	ULONG StorRc = MadDiskPutScatterGatherList(pDevXtensn,
											   pDevXtensn->pOsSgList,
											   pDevXtensn->bCurWrite);
	pDevXtensn->pOsSgList = NULL;
	PMDL pMDL;
	StorRc = StorPortGetOriginalMdl(pDevXtensn, 
									(PSCSI_REQUEST_BLOCK)pDevXtensn->pActiveSrb,
									&pMDL);
	MmUnlockPages(pMDL);

	BOOLEAN bRC = StorPortReady(pDevXtensn);
	if (!bRC)
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		"MadDiskDpc:StorPortReady returned false!... SerialNo=%d ActiveSrb=%p\n",
		            pDevXtensn->SerialNo, pDevXtensn->pActiveSrb);

	pDevXtensn->pActiveSrb = NULL;
	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDiskDpc... SerialNo=%d issuing SpNotify_ReqComplete\n",
		        pDevXtensn->SerialNo);
	StorPortNotification(RequestComplete, pDevXtensn, pSRB);

	return;
}

VOID MadDiskProcessServiceRequest(_In_ PVOID DeviceExtension, _In_ PVOID Irp)
{
	UNREFERENCED_PARAMETER(DeviceExtension);
	UNREFERENCED_PARAMETER(Irp);
}

VOID MadDiskCompleteServiveIrp(_In_ PVOID DeviceExtension)
{
	UNREFERENCED_PARAMETER(DeviceExtension);
}

VOID MadDiskAssignSenseData(PMAD_HDW_DEVICE_EXTENSION pDevXtensn, 
	                        UCHAR SenseKey, UCHAR SenseASC, UCHAR SenseASCQ)
{
	pDevXtensn->SenseData.ErrorCode                    = 0x70;
	pDevXtensn->SenseData.SenseKey                     = SenseKey;
	pDevXtensn->SenseData.AdditionalSenseCode          = SenseASC;
	pDevXtensn->SenseData.AdditionalSenseCodeQualifier = SenseASCQ;

	return;
}

VOID  MadDiskDmaStarted(_In_ PVOID DeviceExtension)
{
	PMAD_HDW_DEVICE_EXTENSION pDevXtensn = 
		                      (PMAD_HDW_DEVICE_EXTENSION)DeviceExtension;
	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			    "MadDiskDmaStarted SerialNo=%d\n", pDevXtensn->SerialNo);
}

ULONG MadDiskScsiSetErrorSenseData(PMAD_HDW_DEVICE_EXTENSION pDevXtensn, 
	                               PSTORAGE_REQUEST_BLOCK pSRB, UCHAR OpCode)
{
	UCHAR CdbLength8;
	ULONG CdbLength32;
	UCHAR ScsiStatus;
	PVOID pSenseInfoBuffer;
	UCHAR SenseInfoBufferLength;
	PCDB pCDB = SrbGetScsiData(pSRB, &CdbLength8, &CdbLength32, &ScsiStatus,
							   &pSenseInfoBuffer, &SenseInfoBufferLength);
	//UCHAR Lun = pCDB->CDB6GENERIC.LogicalUnitNumber;
	ULONG SpStatus = SRB_STATUS_SUCCESS;
	UNREFERENCED_PARAMETER(pCDB);

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDiskScsiSetErrorModeSenseData... SerialNo=%d pSRB=%p OpCode=x%X\n",
		        pDevXtensn->SerialNo, pSRB, OpCode);

	if ((pSenseInfoBuffer == NULL) || (SenseInfoBufferLength == 0))
        {
		SpStatus = SRB_STATUS_ERROR;
		pSRB->SrbStatus = SRB_STATUS_ERROR;
	    }
	else
	    {
		PSENSE_DATA pSense = (PSENSE_DATA)pSenseInfoBuffer;
		RtlZeroMemory(pSense, SenseInfoBufferLength);
		pSense->ErrorCode = 0x70;
		pSense->Valid = 0;
		pSense->SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;
		pSense->AdditionalSenseLength = 0x15;
		pSense->AdditionalSenseCode = SCSI_ADSENSE_ILLEGAL_COMMAND;
		pSense->AdditionalSenseCodeQualifier = 0;
		ScsiStatus = SCSISTAT_CHECK_CONDITION;
		pSRB->SrbStatus = SRB_STATUS_AUTOSENSE_VALID | SRB_STATUS_ERROR;
	    }

	// Set this to 0 to indicate that no data was transfered because of the error.
	pSRB->DataTransferLength = 0;

	return SpStatus;
}

ULONG MadDiskScsiSetModeSense(PMAD_HDW_DEVICE_EXTENSION pDevXtensn, 
	                          PSTORAGE_REQUEST_BLOCK pSRB, PCDB pCdb,
	                          PMODE_PARAMETER_HEADER pModeHeader)
{
	static UCHAR ModeSense8[] = { 0x00, 0x00 };
	static UCHAR ModeSense1C[] = { 0x00, 0x00 };
	static UCHAR ModeSenseAll[] =
	       { 
		    0x02, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	        0x18, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	        0x19, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	         //0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		     //      0x00, 0x00, 0x00, 0x00,
	         //0x1D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	         //      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	         //0x1E, 0x00, 0x00, 0x00,
	         //0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		     //      0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	        };
	static SIZEOF_MODE_SENSE_ALL = sizeof(ModeSenseAll);

	UCHAR OpCode = pCdb->CDB6GENERIC.OperationCode;
	UCHAR ModeParmHedrLen = 
		  (OpCode == SCSIOP_MODE_SENSE) ? sizeof(PMODE_PARAMETER_HEADER) :
		                                  sizeof(PMODE_PARAMETER_HEADER10);
	UCHAR PageCode = pCdb->MODE_SENSE.PageCode;
	PUCHAR pParmBlock = (PUCHAR)pModeHeader + sizeof(PMODE_PARAMETER_HEADER);

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDiskScsiSetModeSense... SerialNo=%d ModeSensePgCode=x%X\n",
		        pDevXtensn->SerialNo, PageCode);

	pSRB->SrbStatus = SRB_STATUS_SUCCESS;
	pModeHeader->BlockDescriptorLength = 0;
	RtlZeroMemory(pModeHeader, pSRB->DataTransferLength);

	switch (PageCode)
	    {
	    case MODE_SENSE_CURRENT_VALUES:
	        {
		    pModeHeader->ModeDataLength = sizeof(MODE_PARAMETER_HEADER) +
			                              sizeof(MODE_PARAMETER_BLOCK);
		    pModeHeader->MediumType = 0;

		    __try
		        {
			    pModeHeader->DeviceSpecificParameter |=
			    (pDevXtensn->MadVmDisk.bReadOnlyDisk) ? MODE_DSP_WRITE_PROTECT : 0;
			    }

		    __except (EXCEPTION_EXECUTE_HANDLER)
		        {
			    pSRB->SrbStatus = SRB_STATUS_ERROR;
			    break; // goto completeRequest;
		        }

		    pModeHeader->BlockDescriptorLength = sizeof(MODE_PARAMETER_BLOCK);

		    PMODE_PARAMETER_BLOCK pModeBlock =
			(PMODE_PARAMETER_BLOCK)(pModeHeader + sizeof(MODE_PARAMETER_HEADER));

		    RtlZeroMemory(pModeBlock, sizeof(MODE_PARAMETER_BLOCK));
	        }
	        break;

		case MODE_PAGE_CACHING:
		case MODE_PAGE_MEDIUM_TYPES:
		case MODE_PAGE_FAULT_REPORTING:
			TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			"MadDiskScsiSetModeSense... SerialNo=%d ModeSensePgCode not implemented\n",
				        pDevXtensn->SerialNo);
			pSRB->SrbStatus = SRB_STATUS_REQUEST_SENSE_FAILED; // SRB_STATUS_INVALID_REQUEST;
			pSRB->DataTransferLength = 0;
			break;

		case MODE_SENSE_RETURN_ALL:
			pModeHeader->BlockDescriptorLength = 0; // (UCHAR)sizeof(ModeSenseAll);
			pModeHeader->ModeDataLength = (UCHAR)(ModeParmHedrLen +
				                                  SIZEOF_MODE_SENSE_ALL - 1);
			pModeHeader->MediumType = 0; //Direct-access block device 
			StorPortCopyMemory(pParmBlock, ModeSenseAll, SIZEOF_MODE_SENSE_ALL);
			//pModeHeader->ModeDataLength--;
			//pSRB->DataTransferLength = pModeHeader->ModeDataLength;
			pSRB->SrbStatus = SRB_STATUS_SUCCESS;
			break;

	    case MODE_PAGE_CAPABILITIES:
	    default:
		    pModeHeader->ModeDataLength = sizeof(MODE_PARAMETER_HEADER) -
			                              sizeof(pModeHeader->ModeDataLength);
		    pModeHeader->MediumType = 0;
			pModeHeader->BlockDescriptorLength = sizeof(MODE_PARAMETER_BLOCK);
		    __try 
		    {
			pModeHeader->DeviceSpecificParameter |=
			(pDevXtensn->MadVmDisk.bReadOnlyDisk) ? MODE_DSP_WRITE_PROTECT : 0;
			}
		    __except (EXCEPTION_EXECUTE_HANDLER)
		    {
			pSRB->SrbStatus = SRB_STATUS_ERROR;
			break;
		    }
	    }

	return 0;
}

VOID  MadDiskFreeResources(_In_ PVOID DeviceExtension)
{
PMAD_HDW_DEVICE_EXTENSION pDevXtensn = (PMAD_HDW_DEVICE_EXTENSION)DeviceExtension;
	
ULONG j;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDiskFreeResources... pDevXtensn=%p SerialNo=%d\n",
		        pDevXtensn, pDevXtensn->SerialNo);

	if (pDevXtensn->ConnectListEntry.bConnected == TRUE)
	    {
		pDevXtensn->ConnectListEntry.bConnected = FALSE;
		StorPortNotification(BusChangeDetected, pDevXtensn, 0);
	    }

	for (j = 0; j < 3; j++)
	    MadDiskFreeDeviceBase(pDevXtensn, pDevXtensn->VirtBARs[j]);
}

BOOLEAN MadDiskHdwAdapterState(_In_ PVOID DeviceExtension,
	_In_ PVOID Context, _In_ BOOLEAN SaveState)
{
	PMAD_HDW_DEVICE_EXTENSION pDevXtensn =
		(PMAD_HDW_DEVICE_EXTENSION)DeviceExtension;

	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(SaveState);

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDiskHdwAdapterState SerialNo=%d\n", pDevXtensn->SerialNo);

	return TRUE;
}

/*++
Routine Description:
This routine prepares to reinitialize the SIOP.  It pauses the adapter to
block new I/O's, then does a timer routine call to a TimerReinit routine
which calls SynchronizeAccess to a SyncrhonizeISRReinit routine that does
the actual reinitialization.  This is necessary because we need to
synchronize our StartIo and ISR threads during the reinitialization, and
a SynchronizeAccess call cannot be done from the ISR thread.

Arguments:
pDevXtensn - Supplies a pointer to the specific device extension.

Return Value:
None
--*/
VOID ScheduleReinit(IN PMAD_HDW_DEVICE_EXTENSION pDevXtensn)
{
	// pause the adapter to block any new I/Os
	StorPortPause(pDevXtensn, 60);

	// request a timer call to the TimerReinit routine in 100 msec
	StorPortNotification(RequestTimerCall, pDevXtensn,
		/*TimerReinit*/NULL, (ULONG)100000);
}

#ifdef _MAD_SIMULATION_MODE_
VOID inline MadDiskAcquireLock(ULONG SerialNo, STOR_SPINLOCK eSpLockLvl)
{
PMAD_HDW_DEVICE_EXTENSION pDevXtensn =
	                      (PMAD_HDW_DEVICE_EXTENSION)gpDevXtensns[SerialNo];
    UNREFERENCED_PARAMETER(eSpLockLvl);
	StorPortAcquireSpinLock(pDevXtensn, InterruptLock,
		                    NULL, &pDevXtensn->hDevLock);
	return;
}
//
VOID inline MadDiskReleaseLock(ULONG SerialNo)
{
PMAD_HDW_DEVICE_EXTENSION pDevXtensn = 
	                      (PMAD_HDW_DEVICE_EXTENSION)gpDevXtensns[SerialNo];

	StorPortReleaseSpinLock(pDevXtensn, &pDevXtensn->hDevLock);
	return;
}

ULONG
MadDiskBuildScatterGatherList(PVOID HwDeviceExtension,
							  PVOID pMDL, PVOID CurrentVa, ULONG Length,
							  PPOST_SCATTER_GATHER_EXECUTE ExecutionRoutine,
							  PVOID  Context, BOOLEAN WriteToDevice,
							  PVOID  ScatterGatherBuffer,
							  ULONG  ScatterGatherBufferLength)
{
PMAD_HDW_DEVICE_EXTENSION pDevXtensn =
		(PMAD_HDW_DEVICE_EXTENSION)HwDeviceExtension;
PFN_MADSIM_BUILD_SGLIST pMadsimBuildOsSgList =
		pDevXtensn->pMadSimSpIoFunxns->pMadsimSpBuildOsSgList;
PSTOR_SCATTER_GATHER_LIST pSgList =
		(PSTOR_SCATTER_GATHER_LIST)ScatterGatherBuffer;

	UNREFERENCED_PARAMETER(WriteToDevice);
	UNREFERENCED_PARAMETER(CurrentVa);

	ULONG NtStatus = pMadsimBuildOsSgList(pMDL, (PSCATTER_GATHER_LIST)pSgList,
										  Length, ScatterGatherBufferLength);
	ULONG StorRc = MadDiskConvertNtStatus2StorRc(NtStatus);
	if (StorRc != STOR_STATUS_SUCCESS)
		return StorRc;

	KIRQL Irql = KeRaiseIrqlToDpcLevel();
	ExecutionRoutine(NULL, NULL, pSgList, Context);
	KeLowerIrql(Irql);

	return STOR_STATUS_SUCCESS;
}
#endif //_MAD_SIMULATION_MODE_
