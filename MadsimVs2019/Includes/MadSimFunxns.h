/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : MAD Device Simulation Framework                             */
/*  COPYRIGHT    : (c) 2013 HTF Consulting                                     */
/*                                                                             */
/* This source code is provided by exclusive license for non-commercial use to */
/* XYZ Company                                                                 */
/*                                                                             */ 
/*******************************************************************************/
/*                                                                             */
/*  Exe file ID  : MadBus.sys, MadDevice.sys                                   */ 
/*                                                                             */
/*                                                                             */
/*  Module  NAME : MadBusConfig.h                                              */
/*                                                                             */
/*  DESCRIPTION  : Definitions and structures defining the configuration space */
/*                 for the Mad Bus                                             */
/*                                                                             */
/*******************************************************************************/

#ifndef _MADBUS_CONFIG_H_
#define _MADBUS_CONFIG_H_

//#define MAD_DISK_FULL_HARDWARE_ID      L"PCI\\VEN_9808&2001&SUBSYS_00000001&REV_01\0" //Keep this in sync with above
//#define MAD_DISK_FULL_HARDWARE_ID_LEN  40
#define MAD_DISK_FULL_HARDWARE_ID      L"MadBus2\\HTFC_MadDisk_2001&SUBSYS_00000001\0" //Keep this in sync with above
#define MAD_DISK_FULL_HARDWARE_ID_LEN  (sizeof(MAD_DISK_FULL_HARDWARE_ID) / sizeof(WCHAR))

//Our config space is just an alias of good old PCI - plus extra data
#define MADBUS_WHICHSPACE_CONFIG     PCI_WHICHSPACE_CONFIG

//The set of PNP minor IRPs for our generic device
#define MAD_GENERIC_PNP_MINORS  \
        {IRP_MN_START_DEVICE, IRP_MN_READ_CONFIG,  \
         IRP_MN_QUERY_LEGACY_BUS_INFORMATION, 0xFF}

//The set of PNP minor IRPs for our disk device
#define MAD_DISK_PNP_MINORS  \
        {IRP_MN_START_DEVICE, IRP_MN_READ_CONFIG, \
         IRP_MN_QUERY_LEGACY_BUS_INFORMATION, \
         IRP_MN_QUERY_RESOURCE_REQUIREMENTS, \
         IRP_MN_QUERY_INTERFACE, IRP_MN_QUERY_RESOURCES, 0xFF}

//This typedef assumes the 1st installed & loaded instance of the bus presents the generic MadDevices
// & the 2nd instance presents the Scsi adapters - MadDisks
typedef enum _MAD_BUS_DEVICE_TYPE 
             {eNoBusType=0, eGenericBus, eScsiDiskBus, eMaxBusType}
             MAD_BUS_DEVICE_TYPE;

//Define an interrupt-related parm area for the simulation interrupt thread 
//won't compile w/out WDK/Kmdf
#ifndef _MADDISK_H_ 
typedef struct _MAD_SIMULATION_INT_PARMS
    {
	WDFINTERRUPT              hInterrupt;
	WDFSPINLOCK               hSpinlock;
	KIRQL                     Irql;
	KAFFINITY                 IntAffinity;
	PFN_WDF_INTERRUPT_ISR     pMadEvtIsrFunxn;
	KEVENT                    evDevPowerUp;
	KEVENT                    evDevPowerDown;
	PKEVENT                   pEvIntThreadExit;
    } MAD_SIMULATION_INT_PARMS, *PMAD_SIMULATION_INT_PARMS;

#ifdef MAD_KERNEL_WONT_CREATE_DMA_ADAPTER 
typedef NTSTATUS DMA_ENABLER_CREATE(IN WDFDEVICE Device, 
                                    IN PWDF_DMA_ENABLER_CONFIG Config,
                                    IN OPTIONAL PWDF_OBJECT_ATTRIBUTES Attributes, 
                                    OUT WDFDMAENABLER* DmaEnablerHandle);
typedef DMA_ENABLER_CREATE          *PFN_DMA_ENABLER_CREATE;
//
typedef VOID DMA_ENABLER_SET_MAX_SG_ELEMS(IN WDFDMAENABLER DmaEnabler,
                                          IN size_t MaximumFragments);
typedef      DMA_ENABLER_SET_MAX_SG_ELEMS *PFN_DMA_ENABLER_SET_MAX_SG_ELEMS;
//
typedef NTSTATUS   DMA_TRANSACTION_CREATE(IN WDFDMAENABLER DmaEnabler, 
                                          IN OPTIONAL WDF_OBJECT_ATTRIBUTES *Attributes,
                                          OUT WDFDMATRANSACTION *DmaTransaction);
typedef   DMA_TRANSACTION_CREATE       *PFN_DMA_TRANSACTION_CREATE;
//
typedef NTSTATUS   DMA_TRANSACTION_INIT_FROM_REQUEST(IN WDFDMATRANSACTION DmaTransaction,
                                                     IN WDFREQUEST Request, 
	                                                 IN PFN_WDF_PROGRAM_DMA EvtProgramDmaFunction,
                                                     IN WDF_DMA_DIRECTION DmaDirection);
typedef  DMA_TRANSACTION_INIT_FROM_REQUEST *PFN_DMA_TRANSACTION_INIT_FROM_REQ;
//
typedef NTSTATUS   DMA_TRANSACTION_EXECUTE(IN WDFDMATRANSACTION DmaTransaction,
                                           IN OPTIONAL PVOID Context);
typedef            DMA_TRANSACTION_EXECUTE   *PFN_DMA_TRANSACTION_EXECUTE;
//
typedef WDFREQUEST DMA_TRANSACTION_GET_REQUEST(IN WDFDMATRANSACTION DmaTransaction);
typedef            DMA_TRANSACTION_GET_REQUEST *PFN_DMA_TRANSACTION_GET_REQUEST;
//
typedef size_t     DMA_TRANSACTION_GET_BYTES_XFERD(IN WDFDMATRANSACTION DmaTransaction);
typedef            DMA_TRANSACTION_GET_BYTES_XFERD *PFN_DMA_TRANSACTION_GET_BYTES_XFERD;
//
typedef BOOLEAN    DMA_TRANSACTION_DMA_COMPLETED(IN WDFDMATRANSACTION DmaTransaction,
                                                 OUT NTSTATUS *Status);
typedef  DMA_TRANSACTION_DMA_COMPLETED  *PFN_DMA_TRANSACTION_DMA_COMPLETED;

typedef WDFDEVICE  DMA_TRANSACION_GET_DEVICE(IN WDFDMATRANSACTION  DmaTransaction);
typedef            DMA_TRANSACION_GET_DEVICE *PFN_DMA_TRANSACTION_GET_DEVICE;

typedef struct _MADSIM_KMDF_DMA_FUNXNS
    {
	PFN_DMA_ENABLER_CREATE                           pDmaEnablerCreate;
	PFN_DMA_ENABLER_SET_MAX_SG_ELEMS                 pDmaSetMaxSgElems;
	PFN_DMA_TRANSACTION_CREATE                       pDmaXaxnCreate;
	PFN_DMA_TRANSACTION_INIT_FROM_REQ                pDmaXaxnInitFromReq;
	PFN_DMA_TRANSACTION_EXECUTE                      pDmaXaxnExecute;
	PFN_DMA_TRANSACTION_GET_REQUEST                  pDmaXaxnGetRequest;
	PFN_DMA_TRANSACTION_GET_BYTES_XFERD              pDmaXaxnGetBytesXferd;
	PFN_DMA_TRANSACTION_DMA_COMPLETED                pDmaXaxnCompleted;
	//PFN_DMA_TRANSACTION_GET_DEVICE                  pDmaXaxnGetDevice;
    } MADSIM_KMDF_DMA_FUNXNS, *PMADSIM_KMDF_DMA_FUNXNS;
#endif //MAD_KERNEL_WONT_CREATE_DMA_ADAPTER 
#endif //not _MADDISK_H_

//NTSTATUS MadsimBuildOsSgList
NTSTATUS MadsimBuildOsSgList(IN PMDL pMDL, PSCATTER_GATHER_LIST pSgList);

#ifdef  _MADDISK_H_
//Define a WDF-ism for the Miniport driver
typedef enum _DEVICE_POWER_STATE
{
    PowerDeviceUnspecified = 0,
    PowerDeviceD0, PowerDeviceD1, PowerDeviceD2, PowerDeviceD3,
    PowerDeviceMaximum
} DEVICE_POWER_STATE, * PDEVICE_POWER_STATE;
#endif

#ifndef _MADDEVICE_H_
#ifdef _MADBUS_H_
typedef enum _STOR_SPINLOCK { DpcLock = 1, StartIoLock, InterruptLock } STOR_SPINLOCK;
#endif

typedef VOID BUS_SET_POWER_UP(ULONG SerialNo);
typedef BUS_SET_POWER_UP* PFN_BUS_SET_POWER_UP;
//
typedef VOID BUS_SET_POWER_DOWN(ULONG SerialNo);
typedef BUS_SET_POWER_DOWN* PFN_BUS_SET_POWER_DOWN;
//
typedef BOOLEAN HW_INTERRUPT(_In_ PVOID DeviceExtension);
typedef HW_INTERRUPT* PFN_SPMP_INTERRUPT;
//
typedef VOID HW_ACQUIRE_LOCK(ULONG SerialNo, STOR_SPINLOCK eSpLockLvl);
typedef HW_ACQUIRE_LOCK* PFN_SPMP_ACQURE_LOCK;
//
typedef VOID HW_RELEASE_LOCK(ULONG SerialNo);
typedef HW_RELEASE_LOCK* PFN_SPMP_RELEASE_LOCK;
//
typedef struct _MAD_SIMULATION_INT_PARMS_SCSI
{
    PVOID                     pDevXtensn;
    PFN_SPMP_INTERRUPT        pMadDiskISR;
    PFN_SPMP_ACQURE_LOCK      pAcquireLockFunxn;
    PFN_SPMP_RELEASE_LOCK     pReleaseLockFunxn;

    //We need these to be pointers because a miniport doesn't know about event objects
    PFN_BUS_SET_POWER_UP      pSetPowerUpEvFunxn;
    PFN_BUS_SET_POWER_DOWN    pSetPowerDownEvFunxn;
    PVOID                     pIntThreadExitEv;
} MAD_SIMULATION_INT_PARMS_SCSI, * PMAD_SIMULATION_INT_PARMS_SCSI;

#ifndef STOR_PHYSICAL_ADDRESS
#define STOR_PHYSICAL_ADDRESS PHYSICAL_ADDRESS
#endif
typedef struct _MADSIM_STORPORT_IO_FUNXNS
{
    PVOID (*pStorPortGetDeviceBase)(_In_ PVOID pDevXtensn,
                                    _In_ INTERFACE_TYPE BusType,
                                    _In_ ULONG SystemIoBusNumber,
                                    _In_ STOR_PHYSICAL_ADDRESS IoAddress,
                                    _In_ ULONG NumberOfBytes,
                                    _In_ BOOLEAN InIoSpace);
} MADSIM_STORPORT_IO_FUNXNS, *PMADSIM_STORPORT_IO_FUNXNS;

#endif // ! _MADDEVICE_H_

typedef struct _VENDOR_SPECIFIC_DATA
               {
			   ULONG                     BusSlotNum;
			   MAD_DEV_INT_MODE          IntMode;
               PHYSICAL_ADDRESS          liLoAddr;
               PHYSICAL_ADDRESS          liHiAddr;
			   KIRQL                     Irql;
               DEVICE_POWER_STATE        DevPowerState;
               PVOID /*PMAD_SIMULATION_INT_PARMS*/ pMadSimIntParms;
               
               #ifdef MAD_KERNEL_WONT_CREATE_DMA_ADAPTER 
			   PVOID /*(PMADSIM_KMDF_DMA_FUNXNS*/ pMadSimDmaFunxns;
               #endif

               PMADSIM_STORPORT_IO_FUNXNS pMadSimStorportIoFunxns;
               PVOID pDevXtensn; //Only needed w/ the miniport but the structure definition must be consistent 
               } VENDOR_SPECIFIC_DATA, *PVENDOR_SPECIFIC_DATA;
#define  VENDOR_SPEC_DATA_SIZE    sizeof(VENDOR_SPECIFIC_DATA)
//
typedef struct _MADBUS_DEVICE_CONFIG_DATA
               {
               PCI_COMMON_CONFIG    LegacyPci; 
               // 
               VENDOR_SPECIFIC_DATA VndrSpecData;
               //
               UCHAR                Filler[MAD_SECTOR_SIZE - sizeof(PCI_COMMON_CONFIG) - VENDOR_SPEC_DATA_SIZE];       
               }  MADBUS_DEVICE_CONFIG_DATA, *PMADBUS_DEVICE_CONFIG_DATA;

#define  MADBUS_DEV_CNFG_SIZE       sizeof(MADBUS_DEVICE_CONFIG_DATA)

// CONFIG_SPACE default init funxn to be compiled only once & only in the Bus driver
#ifdef _MADBUS_MAIN /////////////////////////////////////////////////////////////////////
void Dflt_Init_Config_Space(PMADBUS_DEVICE_CONFIG_DATA pMadDevCnfg)

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
    pMadDevCnfg->LegacyPci.u.type0.SubVendorID      = MAD_SUBVENDOR_ID; 
    pMadDevCnfg->LegacyPci.u.type0.SubSystemID      = MAD_SUBSYSTEM_ID; 
    //        ULONG   ROMBaseAddress;
    //        UCHAR   CapabilitiesPtr;
    //        UCHAR   Reserved1[3];
    //        ULONG   Reserved2;
    pMadDevCnfg->LegacyPci.u.type0.InterruptLine    = 0x0A;
    pMadDevCnfg->LegacyPci.u.type0.InterruptPin     = 0x01;
    //       UCHAR   MinimumGrant;       
    //       UCHAR   MaximumLatency;     

    return;
}

#endif //_MADBUS_MAIN /////////////////////////////////////////////////////////////////
#endif //_MADBUS_CONFIG_H_