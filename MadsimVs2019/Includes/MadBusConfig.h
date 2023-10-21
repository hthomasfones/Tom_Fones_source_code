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

//Keep these hardware-ids == to the hardware ids in the install (.inx) files 
// *AND* the vendor-id:device-id(s) in MadDefinition.h
#ifdef _MAD_SIMULATION_MODE_
    //Use the simulation plugin hardware ids
    #define MAD_DEVICE_HARDWARE_ID   MADSIM_HARDWARE_ID1 
    #define MAD_DISK_HARDWARE_ID     MADSIM_HARDWARE_ID2 
#else
    //Use the real PCI vendor-device hardware ids
    #define MAD_DEVICE_HARDWARE_ID  L"PCI\\VEN_9808&DEV_1001\0" 
    #define MAD_DISK_HARDWARE_ID    L"PCI\\VEN_9808&DEV_2001\0" 
#endif

#define MAD_DEVICE_HARDWARE_ID_LEN  (sizeof(MAD_DEVICE_HARDWARE_ID) / sizeof(WCHAR))
#define MAD_DISK_HARDWARE_ID_LEN  (sizeof(MAD_DISK_HARDWARE_ID) / sizeof(WCHAR))

//Our config space is just an alias of good old PCI - plus extra data
#define MADBUS_WHICHSPACE_CONFIG     PCI_WHICHSPACE_CONFIG

//The set of PNP minor IRPs for our generic device
#define MADSIM_PNP_MINORS  \
        {IRP_MN_START_DEVICE, IRP_MN_READ_CONFIG,  \
         IRP_MN_QUERY_LEGACY_BUS_INFORMATION, 0xFF}

#if 0
//The set of PNP minor IRPs for our disk device
#define MADSIM_PNP_MINORS_SCSI  \
        {IRP_MN_START_DEVICE, IRP_MN_READ_CONFIG, \
         IRP_MN_QUERY_LEGACY_BUS_INFORMATION, \
         IRP_MN_QUERY_RESOURCE_REQUIREMENTS, \
         IRP_MN_QUERY_INTERFACE, IRP_MN_QUERY_RESOURCES, 0xFF}
#endif

//This typedef assumes the 1st installed & loaded instance of the bus presents the generic MadDevices
// & the 2nd instance presents the Scsi adapters - MadDisks
typedef enum _MAD_BUS_DEVICE_TYPE 
             {eNoBusType=0, eGenericBus, eScsiDiskBus, eMaxBusType}
             MAD_BUS_DEVICE_TYPE;

#ifndef STOR_PHYSICAL_ADDRESS
#define STOR_PHYSICAL_ADDRESS PHYSICAL_ADDRESS
             typedef ULONG STOR_SPINLOCK;
#define DpcLock  1
#define StartIoLock 2
#define InterruptLock 3
#endif

typedef VOID(BUS_SET_POWER_UP)(ULONG SerialNo);
typedef BUS_SET_POWER_UP* PFN_BUS_SET_POWER_UP;

typedef VOID(BUS_SET_POWER_DOWN)(ULONG SerialNo);
typedef BUS_SET_POWER_DOWN* PFN_BUS_SET_POWER_DOWN;

typedef BOOLEAN HW_INTERRUPT(_In_ PVOID DeviceExtension);
typedef HW_INTERRUPT* PFN_SPMP_INTERRUPT;

typedef VOID HW_ACQUIRE_LOCK(ULONG SerialNo, STOR_SPINLOCK eSpLockLvl);
typedef HW_ACQUIRE_LOCK* PFN_SPMP_ACQURE_LOCK;

typedef VOID HW_RELEASE_LOCK(ULONG SerialNo);
typedef HW_RELEASE_LOCK* PFN_SPMP_RELEASE_LOCK;

typedef struct _MAD_SIMULATION_INT_PARMS
        {
        union
            {
            struct WDF_INT_PARMS
                 {
                 WDFINTERRUPT           hInterrupt;
                 WDFSPINLOCK            hSpinlock;
                 KIRQL                  Irql;
                 KAFFINITY              IntAffinity;
                 PFN_WDF_INTERRUPT_ISR  pMadEvtIsrFunxn;
                 } WdfIntParms;

            struct _STOR_INT_PARMS
                  {
                  PVOID                     pDevXtensn;
                  PFN_SPMP_INTERRUPT        pMadDiskISR;
                  PFN_SPMP_ACQURE_LOCK      pAcquireLockFunxn;
                  PFN_SPMP_RELEASE_LOCK     pReleaseLockFunxn;
                  PFN_BUS_SET_POWER_UP      pSetPowerUpEvFunxn;
                  PFN_BUS_SET_POWER_DOWN    pSetPowerDownEvFunxn;
                 } StorIntParms;
        } u;
        
        KEVENT                    evDevPowerUp;
        KEVENT                    evDevPowerDown;
        PKEVENT                   pEvIntThreadExit;
        } MAD_SIMULATION_INT_PARMS, * PMAD_SIMULATION_INT_PARMS;

//MAD_KERNEL_WONT_CREATE_DMA_ADAPTER
#ifdef MAD_KERNEL_WONT_CREATE_DMA_ADAPTER 
typedef NTSTATUS DMA_ENABLER_CREATE(IN WDFDEVICE Device,
                 IN PWDF_DMA_ENABLER_CONFIG Config,
                 IN OPTIONAL PWDF_OBJECT_ATTRIBUTES Attributes,
                 OUT WDFDMAENABLER* DmaEnablerHandle);
typedef DMA_ENABLER_CREATE* PFN_DMA_ENABLER_CREATE;

typedef VOID DMA_ENABLER_SET_MAX_SG_ELEMS(IN WDFDMAENABLER DmaEnabler,
                 IN size_t MaximumFragments);
typedef      DMA_ENABLER_SET_MAX_SG_ELEMS* PFN_DMA_ENABLER_SET_MAX_SG_ELEMS;

typedef NTSTATUS   DMA_TRANSACTION_CREATE(IN WDFDMAENABLER DmaEnabler,
                 IN OPTIONAL WDF_OBJECT_ATTRIBUTES* Attributes,
                 OUT WDFDMATRANSACTION* DmaTransaction);
typedef   DMA_TRANSACTION_CREATE* PFN_DMA_TRANSACTION_CREATE;

typedef NTSTATUS   DMA_TRANSACTION_INIT_FROM_REQUEST(IN WDFDMATRANSACTION DmaTransaction,
                 IN WDFREQUEST Request,
                 IN PFN_WDF_PROGRAM_DMA EvtProgramDmaFunction,
                 IN WDF_DMA_DIRECTION DmaDirection);
typedef  DMA_TRANSACTION_INIT_FROM_REQUEST* PFN_DMA_TRANSACTION_INIT_FROM_REQ;

typedef NTSTATUS   DMA_TRANSACTION_EXECUTE(IN WDFDMATRANSACTION DmaTransaction,
                 IN OPTIONAL PVOID Context);
typedef            DMA_TRANSACTION_EXECUTE* PFN_DMA_TRANSACTION_EXECUTE;

typedef WDFREQUEST DMA_TRANSACTION_GET_REQUEST(IN WDFDMATRANSACTION DmaTransaction);
typedef            DMA_TRANSACTION_GET_REQUEST* PFN_DMA_TRANSACTION_GET_REQUEST;

typedef size_t     DMA_TRANSACTION_GET_BYTES_XFERD(IN WDFDMATRANSACTION DmaTransaction);
typedef            DMA_TRANSACTION_GET_BYTES_XFERD* PFN_DMA_TRANSACTION_GET_BYTES_XFERD;

typedef BOOLEAN    DMA_TRANSACTION_DMA_COMPLETED(IN WDFDMATRANSACTION DmaTransaction,
                 OUT NTSTATUS* Status);
typedef  DMA_TRANSACTION_DMA_COMPLETED* PFN_DMA_TRANSACTION_DMA_COMPLETED;

typedef WDFDEVICE  DMA_TRANSACION_GET_DEVICE(IN WDFDMATRANSACTION  DmaTransaction);
typedef            DMA_TRANSACION_GET_DEVICE* PFN_DMA_TRANSACTION_GET_DEVICE;

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
             //PFN_DMA_TRANSACTION_GET_DEVICE                   pDmaXaxnGetDevice;
             } MADSIM_KMDF_DMA_FUNXNS, * PMADSIM_KMDF_DMA_FUNXNS;

void MadSimInitDmaFunxnTable(MADSIM_KMDF_DMA_FUNXNS* pMadSimDmaFunxns);
#endif //MAD_KERNEL_WONT_CREATE_DMA_ADAPTER 


//Definitions so that the miniport driver will compile
#ifdef _MADDISK_H_
//Define a WDF-ism for the Miniport driver
//typedef enum _DEVICE_POWER_STATE
//{
//    PowerDeviceUnspecified = 0,
//    PowerDeviceD0, PowerDeviceD1, PowerDeviceD2, PowerDeviceD3,
//    PowerDeviceMaximum
//} DEVICE_POWER_STATE, * PDEVICE_POWER_STATE;
//#endif

typedef BOOLEAN HW_INTERRUPT(_In_ PVOID DeviceExtension);
typedef HW_INTERRUPT* PFN_SPMP_INTERRUPT;
//
typedef VOID HW_ACQUIRE_LOCK(ULONG SerialNo, STOR_SPINLOCK eSpLockLvl);
typedef HW_ACQUIRE_LOCK* PFN_SPMP_ACQURE_LOCK;
//
typedef VOID HW_RELEASE_LOCK(ULONG SerialNo);
typedef HW_RELEASE_LOCK* PFN_SPMP_RELEASE_LOCK;
//
#endif //_MADDISK_H_

//Storport simulation-replacement functions
typedef PVOID (MadSimSpMapDeviceBase)(_In_ STOR_PHYSICAL_ADDRESS IoAddress,
                                      _In_ ULONG NumberOfBytes);
typedef MadSimSpMapDeviceBase* PFN_MADSIM_SP_MAP_DEVICE_BASE;

typedef VOID(MadSimSpUnmapMemory)(PVOID VirtAddr, _In_ ULONG NumberOfBytes);
typedef MadSimSpUnmapMemory* PFN_MADSIM_SP_UNMAP_MEMORY;

typedef STOR_PHYSICAL_ADDRESS(MadSimSpGetPhysicalAddress)
                              (PVOID VirtAddr, _Out_ ULONG* pLength);
typedef MadSimSpGetPhysicalAddress* PFN_MADSIM_SP_GET_PHYSICAL_ADDRESS;

typedef BOOLEAN(MadDevIssueLoadMediaFn)(PVOID pPrntDev, PVOID pDevObj);
typedef  MadDevIssueLoadMediaFn* PFN_MADDEV_ISSUE_LOAD_MEDIA;

typedef  NTSTATUS(MadsimBuildOsSgList_type)
                  (IN PMDL pMDL, PSCATTER_GATHER_LIST pSgList,
                   ULONG DataLen, ULONG BufrLen);
typedef  MadsimBuildOsSgList_type* PFN_MADSIM_BUILD_SGLIST;

typedef struct _MADSIM_STORPORT_IO_FUNXNS
         {
         PFN_MADSIM_SP_MAP_DEVICE_BASE      pMadsimSpMapDeviceBase;
         PFN_MADSIM_SP_UNMAP_MEMORY         pMadsimSpUnmapMemory;
         PFN_MADSIM_SP_GET_PHYSICAL_ADDRESS pMadsimSpGetPhysicalAddress;
         PFN_MADDEV_ISSUE_LOAD_MEDIA        pMadDevIssueLoadMedia;
         PFN_MADSIM_BUILD_SGLIST            pMadsimSpBuildOsSgList;
         } MADSIM_STORPORT_IO_FUNXNS, *PMADSIM_STORPORT_IO_FUNXNS;

VOID 
MadSimInitStorPortFunxnTable(PMADSIM_STORPORT_IO_FUNXNS pMadSimSpIoFunxns);

typedef struct _VENDOR_SPECIFIC_DATA
               {
			   ULONG                      BusSlotNum;
			   MAD_DEV_INT_MODE           IntMode;
               //ULONG                     IDTindx;
               ULONG                     IntVector;
               KIRQL                     Irql;
               PHYSICAL_ADDRESS           liLoAddr;
               PHYSICAL_ADDRESS           liHiAddr;
			   //KIRQL                      Irql;
               DEVICE_POWER_STATE         DevPowerState;
               PMAD_SIMULATION_INT_PARMS  pMadSimIntParms;
               PVOID                      pDevXtensn; //Only needed w/ the miniport but the structure definition must be consistent 
               PMADSIM_STORPORT_IO_FUNXNS pMadSimSpIoFunxns;

               #ifdef MAD_KERNEL_WONT_CREATE_DMA_ADAPTER 
			   PMADSIM_KMDF_DMA_FUNXNS pMadSimDmaFunxns;
               //PMADSIM_STORPORT_IO_FUNXNS pMadSimSpIoFunxns;
               #endif
               } VENDOR_SPECIFIC_DATA, *PVENDOR_SPECIFIC_DATA;
#define  VENDOR_SPEC_DATA_SIZE    sizeof(VENDOR_SPECIFIC_DATA)

typedef struct _MADBUS_DEVICE_CONFIG_DATA
               {
               PCI_COMMON_CONFIG    LegacyPci; 
               // 
               VENDOR_SPECIFIC_DATA VndrSpecData;
               //
               UCHAR                Filler[MAD_SECTOR_SIZE - sizeof(PCI_COMMON_CONFIG) - VENDOR_SPEC_DATA_SIZE];       
               }  MADBUS_DEVICE_CONFIG_DATA, *PMADBUS_DEVICE_CONFIG_DATA;

#define  MADBUS_DEV_CNFG_SIZE       sizeof(MADBUS_DEVICE_CONFIG_DATA)

//Passing back specifically the address of the reserved files as a useful place
//for a temp pointer for the vendor-spec-data
inline PVOID MadSetConfigTempVarLoc(PPCI_COMMON_CONFIG pPciConfig)
{
    PVOID  pTemp = &pPciConfig->u.type0.Reserved2;
    return pTemp;
}

// CONFIG_SPACE default init function to be compiled only once & only in the Bus driver
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