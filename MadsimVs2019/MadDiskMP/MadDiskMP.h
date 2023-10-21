/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : MAD Device Simulation Framework                             */
/*  COPYRIGHT    : (c) 2014 HTF Consulting                                     */
/*                                                                             */
/* This source code is provided by exclusive license for non-commercial use to */
/* SOC Limited                                                                 */
/*                                                                             */
/*******************************************************************************/
/*                                                                             */
/*  Exe file ID  : MadDiskMP.sys                                               */
/*                                                                             */
/*  Module  NAME : MadDiskMP.h                                                 */
/*                                                                             */
/*  DESCRIPTION  : Definitions of structures and function prototypes for       */
/*                 MadDevice.sys                                               */
/*                 Derived from WDK\src\storage\StorportMiniports\lsi_u3       */
/*                                                                             */
/*******************************************************************************/
#ifndef _MADDISK_H_
#define _MADDISK_H_

//extern "C" {
#pragma warning(disable : 4115 4201 4214 4127 4702)
#include <ntifs.h>
#include <storport.h>
#include <ntddscsi.h>
#include <ntddstor.h>
#include <ntdddisk.h>
#include <ntstrsafe.h>
#include <srbhelper.h>
#include <ata.h>

#pragma warning(default : 4115 4201 4214 4127 4702)
//};

//We can't include ntddk.h (duplicate defines in ntdef.h) so ...
#ifndef FILE_DEVICE_VIRTUAL_DISK
#define FILE_DEVICE_VIRTUAL_DISK   0x00000024 //ntddk.h
#define FILE_DEVICE_DISK           0x00000007 //ntddk.h
#endif

#define   MADDISK_POOL_TAG    (ULONG)'ddaM'

#include "..\Includes\MadDefinition.h"
#define MADDISK_NUM_PLATTERS           1 
#define MADDISK_NUM_TRACKS_PER_PLATTER 16 
#define MADDISK_NUM_SECTORS_PER_TRACK   \
        (MAD_DEVICE_MAX_SECTORS / MADDISK_NUM_TRACKS_PER_PLATTER)

//#include "MadDiskVer.h"
// version numbers for VER_FILEVERSION, keep in sync with version string
#define MAD_VERSION_MAJOR       5
#define MAD_VERSION_MINOR       9
#define MAD_VERSION_BUILD      10
#define MAD_VERSION_REV         5
#define MAD_CMN_VERSION_NUMBER  "5.09.10.05"
#define MAD_VERSION_LABEL       "MadDiskMP_" MAD_CMN_VERSION_NUMBER

#ifdef _MAD_SIMULATION_MODE_
#define MAD_VDISK_INQUIRY_VENDOR_ID           "HTF-Consulting"
#define MAD_VDISK_INQUIRY_PRODUCT_ID          "MadVDisk"
#define MAD_VDISK_INQUIRY_PRODUCT_REVISION    "V2.0"
#define MAD_VDISK_INQUIRY_VENDOR_SPECIFIC     "Htfc_MadVDiskx"
#endif

//#include "scr_u3m.h"    // memory mapped scripts
#include "Madsiop.h"
//#include "Madnvm.h"
#include "Madsvdt.h"
// headers for DMI support
//#include "Maddmi.h"

#ifdef MAD_VIRTUAL_MINIPORT
    #define MAD_VIRTUAL_DEVICE_BOOL TRUE
#else
    #define MAD_VIRTUAL_DEVICE_BOOL FALSE
#endif

#define MADDISK_MAJOR_REVISION    1
#define MADDISK_MINOR_REVISION    1
#define MADDISK_FIRMWARE_REVISION  \
		"\0x30, \0x30, \0x30, \0x30+MADDISK_MAJOR_REVISION, \0x30, \0x30, \0x30, \0x30+MADDISK_MINOR_REVISION"
#define MADDISK_VENDOR_SIGNATURE 0x4854 //"HT"

#ifndef GUID_NULL
    DEFINE_GUID(GUID_NULL, 0L, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
#endif

//Definitions from the Kernel/WDM/WDF world to compile "MadSimIoFunxns.h" in the Miniport
//#ifdef _MADDISK_H_

typedef USHORT UINT;

#ifndef WDFDEVICE
#define WDFDEVICE HANDLE
#endif

#ifndef WDFINTERRUPT
#define WDFINTERRUPT HANDLE
#endif

#ifndef WDFDREQUEST
#define WDFREQUEST HANDLE
#endif

#ifndef WDFDMATRANSACTION
#define WDFDMATRANSACTION HANDLE
#endif

#ifndef WDFDMAENABLER
#define WDFDMAENABLER HANDLE
#endif

#ifndef WDFSPINLOCK
#define WDFSPINLOCK  HANDLE
#endif

#ifndef PFN_WDF_PROGRAM_DMA
#define PFN_WDF_PROGRAM_DMA PVOID
#endif

#ifndef  PWDF_DMA_ENABLER_CONFIG
#define PWDF_DMA_ENABLER_CONFIG PVOID
#endif

#ifndef  PWDF_OBJECT_ATTRIBUTES
#define PWDF_OBJECT_ATTRIBUTES  PVOID
#endif

#ifndef  PFN_WDF_INTERRUPT_ISR
#define PFN_WDF_INTERRUPT_ISR  PVOID
#endif

#ifndef  PKEVENT
#define  PKEVENT  PVOID
#endif

#ifndef  WDF_DMA_DIRECTION
typedef BOOLEAN  WDF_DMA_DIRECTION;
#endif

#ifndef WDF_OBJECT_ATTRIBUTES
typedef  struct _WDF_OBJECT_ATTRIBUTES { PVOID pVoid; } WDF_OBJECT_ATTRIBUTES;
#endif

#ifndef PFN_DMA_TRANSACTION_CREATE
typedef NTSTATUS   
        DMA_TRANSACTION_CREATE(IN WDFDMAENABLER DmaEnabler,
	                           IN OPTIONAL WDF_OBJECT_ATTRIBUTES* Attributes,
	                           OUT WDFDMATRANSACTION* DmaTransaction);
typedef   DMA_TRANSACTION_CREATE* PFN_DMA_TRANSACTION_CREATE;
#endif
typedef struct _PARTITION_ENTRY
{
	UCHAR  Status;
	UCHAR  CHS0[3];
	UCHAR  PartType;
	UCHAR  CHSF[3];
	ULONG  NumSectors;
	ULONG  LBAZero;
} PARTITION_ENTRY;

typedef struct _MASTER_BOOT_RECORD
{
	UCHAR            BootCode[446];
	PARTITION_ENTRY  PartEntry1;
	PARTITION_ENTRY  PartEntry2;
	PARTITION_ENTRY  PartEntry3;
	PARTITION_ENTRY  PartEntry4;
	UCHAR            BootSignature[2];
}  MASTER_BOOT_RECORD, *PMASTER_BOOT_RECORD;
//#endif //MADDISK

#include "..\Includes\MadUtilFunxns.h"
#include  "..\Includes\MadBusConfig.h"
#include  "..\Includes\MadSubSysID.h"

#ifdef  MAD_VIRTUAL_MINIPORT
    #define MADDISK_NUM_ACCESS_RANGES  0
    #define MADDISK_DEV_CNFG_SIZE      64
#else
    #define MADDISK_NUM_ACCESS_RANGES  MADDEV_NUM_MEMORY_RANGES
    #define MADDISK_DEV_CNFG_SIZE      MADBUS_DEV_CNFG_SIZE
#endif

#define MAD_MP_Q_LIMIT           1

#ifndef TRACE_LEVEL_NONE
#define TRACE_LEVEL_NONE        0   // Tracing is not on
#define TRACE_LEVEL_FATAL       1   // Abnormal exit or termination
#define TRACE_LEVEL_ERROR       2   // Severe errors that need logging
#define TRACE_LEVEL_WARNING     3   // Warnings such as allocation failure
#define TRACE_LEVEL_INFORMATION 4   // Includes non-error cases(e.g.,Entry-Exit)
#define TRACE_LEVEL_VERBOSE     5   // Detailed traces from intermediate steps
#define TRACE_LEVEL_RESERVED6   6
#define TRACE_LEVEL_RESERVED7   7
#define TRACE_LEVEL_RESERVED8   8
#define TRACE_LEVEL_RESERVED9   9
#endif // TRACE_LEVEL_INFORMATION

// Define the SRB Extension.
typedef struct _SRB_EXTENSION {
	UCHAR PhysBreakCount;                       // physical break count
	UCHAR SrbExtFlags;                          // negot & other flags
	UCHAR autoReqSns;                           // autoReqSns flag
	UCHAR trackEntry;                           // I/O track array entry
	PSVARS_DESCRIPTOR_TABLE svdt;               // pointer to this svdt
	SVARS_DESCRIPTOR_TABLE svarsDescriptorTable; // descriptor table
	UCHAR AlignPad[4];
}SRB_EXTENSION, *PSRB_EXTENSION;

#define SRB_EXT(x) ((PSRB_EXTENSION)(x->pSrbXtensn))

// Define the noncached extension.  Data items are placed in the noncached
// extension because they are accessed via DMA.
typedef struct _HW_NONCACHED_EXTENSION {
	//ULONG       ScsiScripts[sizeof(SCRIPT) / 4];
	SVARS_DESCRIPTOR    NegotMsgBufDesc[SYM_MAX_TARGETS];
	NEGOT_BUF   NegotMsg[SYM_MAX_TARGETS];
	ULONG       dataXferParms[SYM_MAX_TARGETS];
	START_QUEUE_ENTRY  startQueue[START_Q_DEPTH];
	START_QUEUE_ENTRY  caQueue[CA_Q_DEPTH];
	DONE_QUEUE_ENTRY   doneQueue[DONE_Q_DEPTH];
	NEXUS_PTR   ITLnexusPtrs[256];
	UCHAR       alignPad[4];
} HW_NONCACHED_EXTENSION, *PHW_NONCACHED_EXTENSION;

typedef struct _CONNECTION_LIST_ENTRY
{
	LIST_ENTRY                  ListEntry;
	//struct _USER_INSTANCE_INFO* PIInfo;
	ULONG                       BusIndex;
	ULONG                       TargetIndex;
	ULONG                       LunIndex;
	BOOLEAN                     bConnected;
	BOOLEAN                     ContainingMediaRemoved;
	ULONG						IdentifierIndex;
	BOOLEAN						bClosing;  /* Indicates connection is closing */
	PVOID                       DiskBaseAddress;
	ULONG                       DiskSize;
	//CONNECT_IN                  ConnectionInfo;
} CONNECTION_LIST_ENTRY, * PCONNECTION_LIST;

typedef struct _MAD_VM_DISK
{
	//ULONG				MagicNumber;
	LIST_ENTRY			ListEntry;
	//PVOID				PUserLocalInformation;
	ULONG				PathId;
	ULONG				TargetId;
	ULONG				Lun;
	PINQUIRYDATA		pInquiryData;
	BOOLEAN				bReadOnlyDisk;
	BOOLEAN				bMissing;
	PVOID				pDevExt;
	LONG				OutstandingIoCount;
	BOOLEAN				bReportedMissing;
} MAD_VM_DISK, *PMAD_VM_DISK;

typedef struct _MAD_DISK_LU_EXTENSION
{
	UCHAR					DeviceType;
	ULONG					PathId;
	ULONG					TargetId;
	ULONG					Lun;
	BOOLEAN					Missing;
	PVOID					PDevExt;
	PMAD_VM_DISK            pVmDisk;
} MAD_DISK_LU_EXTENSION, *PMAD_DISK_LU_EXTENSION;

typedef struct _SIM_STOR_SCATTER_GATHER_LIST
{
	ULONG NumberOfElements;
	ULONG_PTR Reserved;
	STOR_SCATTER_GATHER_ELEMENT List[1];
} SIM_STOR_SCATTER_GATHER_LIST, * PSIM_STOR_SCATTER_GATHER_LIST;

typedef struct _MAD_DISK_SCATTER_GATHER_LIST
{
	SIM_STOR_SCATTER_GATHER_LIST  SgList;
	STOR_SCATTER_GATHER_ELEMENT   SgElems[MAD_DMA_MAX_SECTORS];
} MAD_DISK_SCATTER_GATHER_LIST, *PMAD_DISK_SCATTER_GATHER_LIST;

// Define the LSI_U3 Device Extension structure
typedef struct _MAD_HDW_DEVICE_EXTENSION {
	PHW_NONCACHED_EXTENSION NonCachedExtension; // pointer to noncached device extension

	ULONG   *dxp;                       // ptr to data xfer table, all devices

	USHORT  DeviceFlags;        // bus specific flags

	BOOLEAN StopAdapter;            // flag to indicate HBA is shutdown

	ULONG   ResetActive;        // non-zero indicates a device reset is active

	PORT_CONFIGURATION_INFORMATION   PortConfigInfo;
	GUID                             DiskGUID;

	LIST_ENTRY			   DeviceList;
	INQUIRYDATA            InquiryData;
	IDENTIFY_DEVICE_DATA   IdDeviceData;
	CONNECTION_LIST_ENTRY  ConnectListEntry;
	MAD_VM_DISK            MadVmDisk;

	PMAD_DISK_LU_EXTENSION  pLuExt;
	MAD_DISK_LU_EXTENSION   MadLuExt;

	ULONG                   SerialNo;
	ULONG                   CurLogicalBlock;
	ULONG                   CurXferBlocks;
	BOOLEAN                 bCurWrite;
	PVOID                   pOsSgList;

	STOR_DPC       MadIsrDpc;

	BOOLEAN     WmiPowerDeviceEnableRegistered;

	//MADDEVICE_WMI_DATA    MadDevWmiData;

	//SYSTEM_POWER_STATE    CurrSysPowerState;   // The general system power state
	//DEVICE_POWER_STATE    CurrDevPowerState;   // The state of the device(D0 or D3)
	ULONG                 DevStatus;
	NTSTATUS              DevNtStatus;

	//Resource parameters
	//KIRQL                     Irql;
	//ULONG                     IDTindx;
	KAFFINITY                 IntAffinity;
	STOR_LOCK_HANDLE          hDevLock;
	PHYSICAL_ADDRESS          BARs[6];
	PVOID                     VirtBARs[6];
	PVOID                     pActiveSrb;
	SENSE_DATA                SenseData;

	//MADBUS_DEVICE_CONFIG_DATA DevCnfgData;
	MASTER_BOOT_RECORD        MBR;

	PMADREGS                  pMadRegs;
	PVOID                     pMadPioRead;
	PVOID                     pMadPioWrite;
	PVOID                     pThisDevObj;
	PVOID                     pParentDevObj;
	PVOID                     pLowerDevObj;

	//Chained DMA
	PHYSICAL_ADDRESS            liCDPP; //Physical addr of devices's SG-List... will point to below 
	MAD_DMA_CHAIN_ELEMENT       HdwSgDmaList[MAD_DMA_MAX_SECTORS + 1]; //prototype: static size 
	MAD_DISK_SCATTER_GATHER_LIST MadDiskSgList;

#ifdef _MAD_SIMULATION_MODE_
	PVENDOR_SPECIFIC_DATA          pVndrSpecData;
	PMAD_SIMULATION_INT_PARMS      pMadSimIntParms; //Used for communicating with the simulation Interrupt thread
	PMADSIM_STORPORT_IO_FUNXNS     pMadSimSpIoFunxns;
	//STOR_LOCK_HANDLE               hOurSimLock;
#endif //_MAD_SIMULATION_MODE_
} MAD_HDW_DEVICE_EXTENSION, *PMAD_HDW_DEVICE_EXTENSION;

// MADDISK miniport driver function declarations.
ULONG         DriverEntry(IN PVOID DriverObject, IN PVOID Argument2);
typedef VOID  DRIVER_UNLOAD(PDRIVER_OBJECT pDriverObj); //From wdm.h - not defined for miniports
VOID          MadDiskDriverUnload(PDRIVER_OBJECT pDriverObj);
BOOLEAN       MadDiskHdwInitialize(IN PVOID Context);
BOOLEAN       MadDiskStartIo(IN PVOID Context, IN PSCSI_REQUEST_BLOCK pSRB);
BOOLEAN       MadDiskStartIoPnp(PMAD_HDW_DEVICE_EXTENSION pDevXtensn,
	                            IN PSTORAGE_REQUEST_BLOCK pSRB);
BOOLEAN       MadDiskISR(IN PVOID Context);
VOID         MadDiskProcessServiceRequest(_In_ PVOID DeviceExtension,
	                                      _In_ PVOID Irp);
VOID         MadDiskCompleteServiveIrp(_In_ PVOID DeviceExtension);
PHW_COMPLETE_SERVICE_IRP MadDiskCompleteServiceIrp;
HW_DPC_ROUTINE MadDiskDpc;

#ifdef  MAD_VIRTUAL_MINIPORT
    ULONG MadDiskFindAdapter(_In_ PVOID DeviceExtension, _In_ PVOID HwContext,
	                         _In_ PVOID BusInformation, _In_ PVOID LowerDevice,
	                         _In_ PCHAR ArgumentString,
	                         _Inout_ PPORT_CONFIGURATION_INFORMATION ConfigInfo,
	                         _In_ PBOOLEAN Again);
#else
    ULONG     MadDiskFindAdapter(__in PVOID Context,
	                             __in PVOID Reserved1, __in PVOID Reserved2,
		                         __in PCSTR ArgumentString,
	                             __inout PPORT_CONFIGURATION_INFORMATION ConfigInfo,
		                         __in PUCHAR Reserved3);
#endif

BOOLEAN       MadDiskBusReset(IN PVOID pDevXtensn, IN ULONG PathId);
SCSI_ADAPTER_CONTROL_STATUS 
MadDiskAdapterControl(IN PVOID pDevExt, 
	                  IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
	                  IN PVOID Parameters);
BOOLEAN       MadDiskBuildIo(IN PVOID Context, IN PSCSI_REQUEST_BLOCK pSRB);
VOID          MadDiskFreeResources(_In_ PVOID DeviceExtension);
VOID          MadDiskDmaStarted(_In_ PVOID DeviceExtension);
BOOLEAN       MadDiskHdwAdapterState(_In_ PVOID DeviceExtension,
	                                 _In_ PVOID Context, _In_ BOOLEAN SaveState);

///////////////////////////////////////////////////////////////////////
BOOLEAN MadDiskExecSrbIoControl(PMAD_HDW_DEVICE_EXTENSION pDevXtensn,
	                            PSTORAGE_REQUEST_BLOCK pSRB, 
	                            //PSRB_IO_CONTROL pSrbIoCtrl,
	                            BOOLEAN* bRequestDone);
BOOLEAN MadDiskInitScsiRequest(PMAD_HDW_DEVICE_EXTENSION pDevXtensn, 
	                           PSTORAGE_REQUEST_BLOCK pSRB, BOOLEAN* bRequestDone);
BOOLEAN MadDiskScsiProcess(PMAD_HDW_DEVICE_EXTENSION pDevXtensn,
	                       PSTORAGE_REQUEST_BLOCK pSRB, PCDB pCDB);
BOOLEAN MadDiskScsiProcessCDB6(PMAD_HDW_DEVICE_EXTENSION pDevXtensn, 
	                       PSTORAGE_REQUEST_BLOCK pSRB, PCDB pCDB, UCHAR OpCode);
BOOLEAN  MadDiskScsiProcessCDB10(PMAD_HDW_DEVICE_EXTENSION pDevXtensn, 
	                         PSTORAGE_REQUEST_BLOCK pSRB, PCDB pCDB, UCHAR OpCode);
BOOLEAN  MadDiskScsiProcessCDB12(PMAD_HDW_DEVICE_EXTENSION pDevXtensn, 
	                         PSTORAGE_REQUEST_BLOCK pSRB, PCDB pCDB, UCHAR OpCode);
BOOLEAN  MadDiskScsiProcessCDB16(PMAD_HDW_DEVICE_EXTENSION pDevXtensn, 
	                         PSTORAGE_REQUEST_BLOCK pSRB, PCDB pCDB, UCHAR OpCode);
BOOLEAN MadDiskScsiInitDmaIo(PMAD_HDW_DEVICE_EXTENSION pDevXtensn, 
	                     PSTORAGE_REQUEST_BLOCK pSRB,
	                     ULONG LogicalBlock, ULONG XferBlocks, BOOLEAN bWrite);
VOID MadDiskScsiDmaExecRtn(IN PVOID* DeviceObject, IN PVOID* Irp,
						   PSTOR_SCATTER_GATHER_LIST pSgList, PVOID Context);
/*
MadDiskBuildDeviceChainedDmaListFromOsSgList(PMAD_HDW_DEVICE_EXTENSION pDevXtensn, 
	                                         PSTORAGE_REQUEST_BLOCK pSRB,
	                                         PSTOR_SCATTER_GATHER_LIST pSgList, 
	                                         ULONG64 DevOffset,
                                             BOOLEAN bWrite); */

ULONG MadDiskBuildDevSgListFromOsSgList(PMAD_HDW_DEVICE_EXTENSION pDevXtensn,
										PSTOR_SCATTER_GATHER_LIST pSgList,
										ULONG64 DevOffset,
										BOOLEAN bWrite);
VOID MadDiskAssignSenseData(PMAD_HDW_DEVICE_EXTENSION pDevXtensn, UCHAR SenseKey, 
	                        UCHAR SenseASC, UCHAR SenseASCQ);
ULONG MadDiskScsiSetErrorSenseData(PMAD_HDW_DEVICE_EXTENSION pDevXtensn,
	                               PSTORAGE_REQUEST_BLOCK pSRB, UCHAR OpCode);
ULONG MadDiskScsiSetModeSense(PMAD_HDW_DEVICE_EXTENSION pDevXtensn,
	                         PSTORAGE_REQUEST_BLOCK pSRB, PCDB pCdb,
	                         PMODE_PARAMETER_HEADER pModeHeader);
BOOLEAN MadDiskIssueLoadMedia(PMAD_HDW_DEVICE_EXTENSION pDevXtensn,
	                          PVOID pPrntDev, PVOID pDevObj);

VOID MadDiskInitDefaultMBR(PMAD_HDW_DEVICE_EXTENSION pDevXtensn,
	                       PMASTER_BOOT_RECORD pMBR);
VOID ScheduleReinit(IN PMAD_HDW_DEVICE_EXTENSION pDevXtensn);

STOR_PHYSICAL_ADDRESS 
MadDiskGetPhysicalAddress(_In_ PVOID Context, _In_opt_ PSTORAGE_REQUEST_BLOCK pSRB,
	                      _In_ PVOID VirtualAddress,
	                      _Out_ ULONG* pLength);

VOID inline MadDiskInitDevExtension(PMAD_HDW_DEVICE_EXTENSION pDevXtensn, 
	                                GUID DiskGUIDs[], 
	                                PIDENTIFY_DEVICE_DATA pIdDeviceData)
{
	InitializeListHead(&pDevXtensn->DeviceList);

	StorPortCopyMemory(&pDevXtensn->IdDeviceData, pIdDeviceData,
		               sizeof(IDENTIFY_DEVICE_DATA));

	StorPortCopyMemory(&pDevXtensn->DiskGUID,
		               &DiskGUIDs[pDevXtensn->SerialNo], sizeof(GUID));

	StorPortCopyMemory(&pDevXtensn->IdDeviceData.SerialNumber,
		               &pDevXtensn->DiskGUID, sizeof(GUID));

	MadDiskInitDefaultMBR(pDevXtensn, &pDevXtensn->MBR);

	PVOID pVoid = &pDevXtensn->MadDiskSgList;
	ULONG Len = sizeof(MAD_DMA_CHAIN_ELEMENT);
	pDevXtensn->liCDPP = 
	MadDiskGetPhysicalAddress(pDevXtensn, NULL, pVoid, &Len);
}

UCHAR inline MadDiskConvertStorRc2SrbStatus(ULONG StorRc)
	{
	switch (StorRc)
		{
		case STOR_STATUS_SUCCESS:
			return SRB_STATUS_SUCCESS;

		case STOR_STATUS_UNSUCCESSFUL:
		case STOR_STATUS_INVALID_IRQL:
			return SRB_STATUS_ERROR;

		case STOR_STATUS_ACCESS_DENIED:
			return SRB_STATUS_INVALID_PATH_ID;

		case STOR_STATUS_INVALID_PARAMETER:
		case STOR_STATUS_BUFFER_TOO_SMALL:
			return SRB_STATUS_INVALID_PARAMETER;
		
		case STOR_STATUS_INVALID_DEVICE_REQUEST:
			return SRB_STATUS_INVALID_REQUEST;

		case STOR_STATUS_NOT_IMPLEMENTED:
			return SRB_STATUS_INVALID_REQUEST;

		case STOR_STATUS_INSUFFICIENT_RESOURCES:
			return SRB_STATUS_INSUFFICIENT_RESOURCES;

		case STOR_STATUS_BUSY:
			return SRB_STATUS_BUSY;

		case STOR_STATUS_THROTTLED_REQUEST:
			return SRB_STATUS_THROTTLED_REQUEST;

		default:
			return SRB_STATUS_BAD_FUNCTION;
		}
}

ULONG inline MadDiskConvertNtStatus2StorRc(ULONG NtStatus)
	{
	switch (NtStatus)
		{
		case STATUS_SUCCESS:
			return STOR_STATUS_SUCCESS;

		case STATUS_ACCESS_DENIED:
			return STOR_STATUS_ACCESS_DENIED;

		case STATUS_INVALID_PARAMETER:
			return STOR_STATUS_INVALID_PARAMETER;

		case STATUS_INVALID_DEVICE_REQUEST:
			return STOR_STATUS_INVALID_DEVICE_REQUEST;

		case STATUS_NOT_IMPLEMENTED:
			return STOR_STATUS_NOT_IMPLEMENTED;

		case STATUS_INSUFFICIENT_RESOURCES:
			return STOR_STATUS_INSUFFICIENT_RESOURCES;

		case STATUS_BUFFER_TOO_SMALL:
		case STATUS_DATA_OVERRUN:
			return STOR_STATUS_BUFFER_TOO_SMALL;

		case STATUS_DEVICE_BUSY:
			return STOR_STATUS_BUSY;

		case STATUS_UNSUCCESSFUL:
		default:
			return STOR_STATUS_UNSUCCESSFUL; 
		}
	}

#ifdef _MAD_SIMULATION_MODE_
//Initialize a default inquiry data struct
VOID MadDiskInitDefaultInquiryData(PINQUIRYDATA pInquiryData);
VOID MadDiskInitDefaultIdDeviceData(PIDENTIFY_DEVICE_DATA pIdDeviceData);
BOOLEAN MadDiskAttachVDisk(PMAD_HDW_DEVICE_EXTENSION pDevXtensn);
PVOID MadDiskSpCreateScsiVDisk(PMAD_HDW_DEVICE_EXTENSION pDevXtensn,
	                           IN ULONG PathId, IN ULONG Lun,
	                           IN BOOLEAN BReadOnlyDevice,
	                           ULONG ExtraStackLocations);
VOID MadDiskAcquireLock(ULONG SerialNo, STOR_SPINLOCK eSpLockLvl);
VOID MadDiskReleaseLock(ULONG SerialNo);

VOID inline MadDiskExchangeSimParms(PMAD_HDW_DEVICE_EXTENSION pDevXtensn,
	                                PPCI_COMMON_CONFIG pPciConf)
{
	PVOID pTempVar = MadSetConfigTempVarLoc(pPciConf);
	PVENDOR_SPECIFIC_DATA pVndrSpecData;
	RtlMoveMemory(&pVndrSpecData, pTempVar, sizeof(PVOID));

	pDevXtensn->pVndrSpecData = pVndrSpecData;
	pDevXtensn->pMadSimSpIoFunxns = pVndrSpecData->pMadSimSpIoFunxns;
	pDevXtensn->pMadSimIntParms = 
		        (PMAD_SIMULATION_INT_PARMS)pVndrSpecData->pMadSimIntParms;

	pDevXtensn->SerialNo = pVndrSpecData->BusSlotNum;
	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDiskExchangeSimParms... SerialNo=%d\n", 
		        pDevXtensn->SerialNo);

	//Plug our interrupt parms for the simulator into the vendor-specific-data 
	pDevXtensn->pMadSimIntParms->u.StorIntParms.pDevXtensn = pDevXtensn;
	pDevXtensn->pMadSimIntParms->u.StorIntParms.pMadDiskISR = &MadDiskISR;
	pDevXtensn->pMadSimIntParms->u.StorIntParms.pAcquireLockFunxn = &MadDiskAcquireLock;
	pDevXtensn->pMadSimIntParms->u.StorIntParms.pReleaseLockFunxn = &MadDiskReleaseLock;
}
#endif //_MAD_SIMULATION_MODE_

///////////////////////////////////////////////////////////////////////////////////////

#ifdef _MAD_REAL_HARDWARE_MODE_
// Aliases for Storport functions
    #define  MadDiskGetDeviceBase           StorPortGetDeviceBase
    #define  MadDiskGetPhysicalAddress      StorPortGetPhysicalAddress
    #define  MadDiskFreeDeviceBase          StorPortFreeDeviceBase
    #define  MadDisktBuildScatterGatherList StorPortBuildScatterGatherList
    #define  MadDiskPutScatterGatherList    StorportPutScatterGatherList
    #define  MadFreeQuantum(x)       

#else 
// Storport simulation replacement functions

inline PVOID MadDiskGetDeviceBase(_In_ PVOID Context,
	                              _In_ INTERFACE_TYPE BusType,
	                              _In_ ULONG SystemIoBusNumber,
	                              _In_ STOR_PHYSICAL_ADDRESS IoAddress,
	                              _In_ ULONG NumberOfBytes,
	                              _In_ BOOLEAN InIoSpace)
{
	UNREFERENCED_PARAMETER(BusType);
	UNREFERENCED_PARAMETER(SystemIoBusNumber);
	UNREFERENCED_PARAMETER(InIoSpace);

	PMAD_HDW_DEVICE_EXTENSION pDevXtensn = (PMAD_HDW_DEVICE_EXTENSION)Context;

	PFN_MADSIM_SP_MAP_DEVICE_BASE pMadsimSpMapDeviceBase =
	pDevXtensn->pMadSimSpIoFunxns->pMadsimSpMapDeviceBase;

	return pMadsimSpMapDeviceBase(IoAddress, NumberOfBytes);
}

inline VOID MadDiskFreeDeviceBase(_In_ PVOID HwDeviceExtension,
	                              _In_ PVOID MappedAddress)
{
	PMAD_HDW_DEVICE_EXTENSION pDevXtensn = 
		(PMAD_HDW_DEVICE_EXTENSION)HwDeviceExtension;

	ULONG UnmapSize = 
	(MappedAddress == pDevXtensn->VirtBARs[3]) ? MAD_DEFAULT_DATA_EXTENT : 
		                                         MAD_SECTOR_SIZE;

	PFN_MADSIM_SP_UNMAP_MEMORY pMadsimSpUnmapMemory =
		pDevXtensn->pMadSimSpIoFunxns->pMadsimSpUnmapMemory;

	pMadsimSpUnmapMemory(MappedAddress, UnmapSize);
}

inline STOR_PHYSICAL_ADDRESS 
MadDiskGetPhysicalAddress(_In_ PVOID Context,
                          _In_opt_ PSTORAGE_REQUEST_BLOCK pSRB,
	                      _In_ PVOID VirtualAddress,
	                      _Out_ ULONG* pLength)
{
	UNREFERENCED_PARAMETER(pSRB);

	PMAD_HDW_DEVICE_EXTENSION pDevXtensn = (PMAD_HDW_DEVICE_EXTENSION)Context;
	PFN_MADSIM_SP_GET_PHYSICAL_ADDRESS pMadsimSpGetPhysicalAddress =
	pDevXtensn->pMadSimSpIoFunxns->pMadsimSpGetPhysicalAddress;

	return pMadsimSpGetPhysicalAddress(VirtualAddress, pLength);
}

inline BOOLEAN MadDiskIssueLoadMedia(PMAD_HDW_DEVICE_EXTENSION pDevXtensn,
	                                 PVOID pPrntDev, PVOID pDevObj)
{
	PFN_MADDEV_ISSUE_LOAD_MEDIA pMadDevIssueLoadMedia =
		pDevXtensn->pMadSimSpIoFunxns->pMadDevIssueLoadMedia;

	return pMadDevIssueLoadMedia(pPrntDev, pDevObj);
}

inline ULONG MadDiskPutScatterGatherList(PVOID HwDeviceExtension,
								         PVOID pOsSgList, BOOLEAN bCurWrite)
{
	PMAD_HDW_DEVICE_EXTENSION pDevXtensn = 
		                      (PMAD_HDW_DEVICE_EXTENSION)HwDeviceExtension;
	UNREFERENCED_PARAMETER(bCurWrite);

	ULONG StorRc = StorPortFreePool(pDevXtensn, pOsSgList);
	if (StorRc != STOR_STATUS_SUCCESS)
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		"MadDiskPutScatterGatherList:StorPortFreePool... SerialNo=%d rc=x%X\n",
					pDevXtensn->SerialNo, StorRc);
	ASSERT(StorRc == STOR_STATUS_SUCCESS);

	return StorRc;
}

ULONG
MadDiskBuildScatterGatherList(PVOID HwDeviceExtension,
							  PVOID pMDL, PVOID CurrentVa, ULONG Length,
							  PPOST_SCATTER_GATHER_EXECUTE ExecutionRoutine,
							  PVOID  Context, BOOLEAN WriteToDevice,
							  PVOID  ScatterGatherBuffer,
							  ULONG  ScatterGatherBufferLength);
#endif //REAL vs SIMULATION
#endif //_MADDISK_H_
