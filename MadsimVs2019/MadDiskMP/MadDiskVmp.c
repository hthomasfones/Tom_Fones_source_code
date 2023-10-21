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
/*  Module  NAME : MadDiskVmp.c                                                */
/*                                                                             */
/*  DESCRIPTION  : Definitions of structures and function prototypes for       */
/*                 the simulation mode virtual disk implementation in          */
/*                 MadDiskMP.sys                                               */
/*                 Derived from OSR's Ram-drive Virtual-Miniport sample driver */
/*                                                                             */
/*******************************************************************************/

// include files used by the Miniport
#include "MadDiskMP.h"

//Compile nothing in here in real-hardwarew mode
#ifdef _MAD_SIMULATION_MODE_

extern INQUIRYDATA  gInquiryData;
extern GUID         gDiskGUIDs[];

VOID MadDiskInitDefaultIdDeviceData(PIDENTIFY_DEVICE_DATA pIdDeviceData)
{
	static char FirmwareRev[] = MADDISK_FIRMWARE_REVISION;

	RtlZeroMemory(pIdDeviceData, sizeof(IDENTIFY_DEVICE_DATA));
	pIdDeviceData->GeneralConfiguration.FixedDevice = 1;
	pIdDeviceData->GeneralConfiguration.RemovableMedia = 0;
	pIdDeviceData->VendorUnique1[3] = MAD_VENDOR_ID;

	pIdDeviceData->NumHeads           = MADDISK_NUM_PLATTERS;
	pIdDeviceData->NumCylinders       = MADDISK_NUM_TRACKS_PER_PLATTER;
	pIdDeviceData->NumSectorsPerTrack = MADDISK_NUM_SECTORS_PER_TRACK;

	//pIdDeviceData->NumberOfCurrentCylinders = MADDISK_NUM_TRACKS_PER_PLATTER;
	//pIdDeviceData->NumberOfCurrentHeads     = MADDISK_NUM_PLATTERS;
	//pIdDeviceData->CurrentSectorsPerTrack   = MADDISK_NUM_SECTORS_PER_TRACK;
	//pIdDeviceData->CurrentSectorCapacity    = MAD_DEVICE_MAX_SECTORS;
	pIdDeviceData->UserAddressableSectors   = (MAD_DEVICE_MAX_SECTORS - 4);

	StorPortCopyMemory(pIdDeviceData->SerialNumber,
		               &gDiskGUIDs[0], sizeof(GUID));

	StorPortCopyMemory(pIdDeviceData->FirmwareRevision,
		               FirmwareRev, sizeof(FirmwareRev));

	pIdDeviceData->Capabilities.DmaSupported = 1;
	pIdDeviceData->AdditionalSupported.WriteBufferDmaSupported = 1;
	pIdDeviceData->AdditionalSupported.ReadBufferDmaSupported = 1;
	pIdDeviceData->QueueDepth    = 1;
	pIdDeviceData->MajorRevision = MADDISK_MAJOR_REVISION;
	pIdDeviceData->MinorRevision = MADDISK_MINOR_REVISION;
	pIdDeviceData->Signature = MADDISK_VENDOR_SIGNATURE;
	pIdDeviceData->CommandSetSupport.RemovableMediaFeature = 0;
	pIdDeviceData->CommandSetActive.RemovableMediaFeature = 0;
}

VOID MadDiskInitDefaultInquiryData(PINQUIRYDATA pInquiryData)
{
	RtlZeroMemory(pInquiryData, sizeof(INQUIRYDATA));

	pInquiryData->DeviceType = DIRECT_ACCESS_DEVICE;
	pInquiryData->DeviceTypeQualifier = DEVICE_CONNECTED;
	pInquiryData->DeviceTypeModifier = 0;
	pInquiryData->RemovableMedia = FALSE;
	pInquiryData->Versions = 2;             // SCSI-2 support
	pInquiryData->ResponseDataFormat = 2;   // Same as Version?? according to SCSI book
	pInquiryData->Wide32Bit = TRUE;         // 32 bit wide transfers
	pInquiryData->Synchronous = TRUE;       // Synchronous commands
	pInquiryData->CommandQueue = FALSE;     // Does not support tagged commands
	pInquiryData->AdditionalLength = INQUIRYDATABUFFERSIZE; // -5;  // Amount of data we are returning
	pInquiryData->LinkedCommands = FALSE;   // No Linked Commands
}

VOID MadDiskInitDefaultMBR(PMAD_HDW_DEVICE_EXTENSION pDevXtensn, 
	                       PMASTER_BOOT_RECORD pMBR)
{
	static UCHAR BootSig[] = { 0x55, 0xAA };
	static USHORT MadVendorSig = MADDISK_VENDOR_SIGNATURE;

	RtlZeroMemory(pMBR, sizeof(MASTER_BOOT_RECORD));
	RtlCopyMemory((PUSHORT)&pMBR->BootCode[440], (PUSHORT)&MadVendorSig, 2);
	pMBR->BootCode[442] = (UCHAR)pDevXtensn->SerialNo;
	RtlCopyMemory(pMBR->BootSignature, BootSig, 2);

	pMBR->PartEntry1.CHS0[1]    = 1;
	pMBR->PartEntry1.LBAZero    = 1;
	pMBR->PartEntry1.CHSF[0]    = 0;
	pMBR->PartEntry1.CHSF[1]    = 0x3F; //High six bit value
	pMBR->PartEntry1.CHSF[2]    = (UCHAR)(MADDISK_NUM_TRACKS_PER_PLATTER - 1);
	pMBR->PartEntry1.NumSectors = MAD_DEVICE_MAX_SECTORS - 1;
}

//Create a virtual disk for a virtual miniport.
//We say attach because the 'hardware' is already there
// ONE disk per Scsi adapter
BOOLEAN MadDiskAttachVDisk(PMAD_HDW_DEVICE_EXTENSION pDevXtensn)
{
	BOOLEAN bRC = TRUE;
	ULONG Len = 0;

	//Start by initializing the inquiry-data
	StorPortCopyMemory(&pDevXtensn->InquiryData, &gInquiryData, 
		               sizeof(INQUIRYDATA));

	RtlCopyMemory((PUCHAR)&pDevXtensn->InquiryData.VendorId[0],
		           MAD_VDISK_INQUIRY_VENDOR_ID, 
		           strlen(MAD_VDISK_INQUIRY_VENDOR_ID));

	RtlCopyMemory((PUCHAR)&pDevXtensn->InquiryData.ProductId[0], 
		           MAD_VDISK_INQUIRY_PRODUCT_ID,
		           strlen(MAD_VDISK_INQUIRY_PRODUCT_ID));

	RtlCopyMemory((PUCHAR)&pDevXtensn->InquiryData.ProductRevisionLevel[0], 
		          MAD_VDISK_INQUIRY_PRODUCT_REVISION,
		          strlen(MAD_VDISK_INQUIRY_PRODUCT_REVISION));

	Len = (ULONG)strlen(MAD_VDISK_INQUIRY_VENDOR_SPECIFIC);
	RtlCopyMemory((PUCHAR)&pDevXtensn->InquiryData.VendorSpecific[0],
		          MAD_VDISK_INQUIRY_VENDOR_SPECIFIC, Len);

	pDevXtensn->InquiryData.VendorSpecific[Len-1] = 
		                    (0x30 + (CHAR)pDevXtensn->SerialNo);

	pDevXtensn->ConnectListEntry.DiskSize = MAD_DEFAULT_DATA_EXTENT;
	pDevXtensn->ConnectListEntry.DiskBaseAddress = pDevXtensn->VirtBARs[3];
	pDevXtensn->ConnectListEntry.BusIndex = 0;
	pDevXtensn->ConnectListEntry.TargetIndex = (UCHAR)pDevXtensn->SerialNo;
	pDevXtensn->ConnectListEntry.LunIndex = 0;

	PVOID pVDisk = MadDiskSpCreateScsiVDisk(pDevXtensn, 0, 0, FALSE, 0);
	if (pVDisk == NULL)
		return FALSE;

	//Put an MBR at sector Zero of the disk
	RtlCopyMemory(pDevXtensn->ConnectListEntry.DiskBaseAddress, 
		          &pDevXtensn->MBR, sizeof(MASTER_BOOT_RECORD));

	StorPortNotification(BusChangeDetected, pDevXtensn, 0);

	pDevXtensn->ConnectListEntry.bConnected = TRUE;
	TraceEvents(TRACE_LEVEL_ERROR, MYDRIVER_ALL_INFO,
	"MadDiskAttachVDisk... SerialNo=%d  Path:Target:Lun=%d:%d:%d rc=%d\n",
		        pDevXtensn->SerialNo, pDevXtensn->ConnectListEntry.BusIndex,
	            pDevXtensn->ConnectListEntry.TargetIndex, 
	            pDevXtensn->ConnectListEntry.LunIndex,bRC);
	return bRC;
}

PVOID MadDiskSpCreateScsiVDisk(PMAD_HDW_DEVICE_EXTENSION pDevXtensn, 
	                           IN ULONG PathId, IN ULONG Lun,
	                           IN BOOLEAN bReadOnlyDevice,
	                           ULONG ExtraStackLocations)
{
	UCHAR TargetId = 0; //(UCHAR)pDevXtensn->SerialNo;
	//KIRQL		lockHandle;
	PMAD_DISK_LU_EXTENSION pLuExt;

	//OsrTracePrint(TRACE_LEVEL_VERBOSE, OSRVMINIPT_DEBUG_FUNCTRACE, (__FUNCTION__": Entered\n"));
	//OSR_DEVEXT_VALID(pDevExt);

	UNREFERENCED_PARAMETER(ExtraStackLocations);

	// Get the address of the logical unit for this device.
	pLuExt = (PMAD_DISK_LU_EXTENSION)StorPortGetLogicalUnit(pDevXtensn,
		                                                    (UCHAR)PathId,
		                                                    TargetId,
		                                                    (UCHAR)Lun);
	if (pLuExt != NULL)
	    {
		TraceEvents(TRACE_LEVEL_ERROR, MYDRIVER_ALL_INFO,
			        ": LuExt %p exists for %d:%d:%d\n",
					pLuExt, PathId, TargetId, Lun);
		return NULL;
	    }

	pDevXtensn->MadLuExt.pVmDisk = &pDevXtensn->MadVmDisk;
	RtlZeroMemory(&pDevXtensn->MadVmDisk, sizeof(MAD_VM_DISK));
	pDevXtensn->MadVmDisk.PathId = PathId;
	pDevXtensn->MadVmDisk.TargetId = TargetId;
	pDevXtensn->MadVmDisk.Lun = Lun;
	pDevXtensn->MadVmDisk.pInquiryData = &pDevXtensn->InquiryData;
	pDevXtensn->MadVmDisk.bReadOnlyDisk = bReadOnlyDevice;
	pDevXtensn->MadVmDisk.bMissing = FALSE;
	pDevXtensn->MadVmDisk.pDevExt = (PVOID)pDevXtensn;
	//pOsrDev->MagicNumber = OSR_VM_DEVICE_MAGIC;

	//OsrAcquireSpinLock(&pDevExt->DeviceListLock, &lockHandle);
	InsertTailList(&pDevXtensn->DeviceList, &pDevXtensn->MadVmDisk.ListEntry);
	//OsrReleaseSpinLock(&pDevExt->DeviceListLock, lockHandle);

	return &pDevXtensn->MadVmDisk;
}
#endif //_MAD_SIMULATION_MODE_