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
/*  Module  NAME : MadBus.h                                                    */
/*                                                                             */
/*  DESCRIPTION  : Definition of structures and function prototypes            */
/*                 Derived from WDK-Toaster\bus\bus.h                          */
/*                                                                             */
/*******************************************************************************/

#ifndef _MADBUS_H_
#define _MADBUS_H_

#include <ntddk.h>
#include <wdf.h>
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include <ntintsafe.h>
#include <initguid.h>
#include <wdmguid.h>

#define  _MAD_SIMULATION_MODE_ //Always true for the simulator - required for some common defines
#include "..\Includes\MadDefinition.h"
#include "..\Includes\MadGUIDs.h"
#include "..\Includes\MadSubSysID.h"

#include "..\Includes\MadBusIoctls.h"
#include "..\Includes\MadBusConfig.h"
#include "..\Includes\MadUtilFunxns.h"
#include "..\Includes\MadBusUsrIntBufrs.h"
#include "MadBusMof.h"
#include ".\MadBus\MadBusMC\MadBus.h"

extern "C" {
#include "..\Includes\MadRegFunxns.h"
}

//Step up the IRP:Io_stack
#define  GET_PREV_IOSTACK_LOCATION(X)  \
         (PIO_STACK_LOCATION)((ULONG_PTR)(sizeof(IO_STACK_LOCATION)) + (ULONG_PTR)X)

#define   MADBUS_POOL_TAG    (ULONG)'bdaM'
#define  BUSRESOURCENAME     L"MadBusWMI" //This must match the named MOFDATA item in the .RC file
#define MAX_INSTANCE_ID_LEN  80

extern ULONG BusEnumDebugLevel;

//Our Driver object context
typedef struct _DRIVER_DATA
{
    UNICODE_STRING    gRegistryPath; 
    WDFDRIVER         hDriver;
    PDRIVER_OBJECT    pDriverObj;
    UNICODE_STRING    RegistryPath;
    //
    ULONG             DevIRQL;
    ULONG             IdtBaseDX;
    BOOLEAN           bAffinityOn;
    ULONG             NumFilters;
    ULONG             NumBusFDOs;
    ULONG             NumAllocDevices;
    ULONG             MadDataXtent;
    PVOID             pDeviceSetBase; // = NULL;
    //
    MADBUS_DEVICE_CONFIG_DATA    MadDevDfltCnfg;
    WDFDEVICE         hDevFDO[eMaxBusType];
    GUID              QueryIntfGuids[20];

    #ifdef _MAD_SIMULATION_MODE_ //MAD_KERNEL_WONT_CREATE_DMA_ADAPTER //We need to generate a table of pointers to WDF-DMA simulation functions
    MADSIM_KMDF_DMA_FUNXNS    MadSimDmaFunxns;
    MADSIM_STORPORT_IO_FUNXNS MadSimSpIoFunxns;
    #endif

} DRIVER_DATA, * PDRIVER_DATA;
//WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DRIVER_DATA, MadBusDeviceObjectGetData)

typedef struct _MADBUS_WMI_DATA
    {
    //Room for other WMI data-block(s) / classes
    MadBusWmiInfo      InfoClassData;
    ////Room for other WMI data-block(s) / classes
    } MADBUS_WMI_DATA, *PMDBUS_WMI_DATA;

typedef struct _PDO_DEVICE_OIDS
    {
    PDEVICE_OBJECT  pPhysDevObj;
    WDFDEVICE       hPhysDevice;
    } PDO_DEVICE_OIDS, *PPDO_DEVICE_OIDS;
#define PDO_DEVICE_OIDS_DEFINED

// The goal of the identification and address description abstractions is that enough
// information is stored for a discovered device so that when it appears on the bus,
// the framework (with the help of the driver writer) can determine if it is a new or
// existing device.  The identification and address descriptions are opaque structures
// to the framework, they are private to the driver writer.  The only thing the framework
// knows about these descriptions is what their size is.
// The identification contains the bus specific information required to recognize
// an instance of a device on its the bus.  The identification information usually
// contains device IDs along with any serial or slot numbers.
// For some buses (like USB and PCI), the identification of the device is sufficient to
// address the device on the bus; in these instances there is no need for a separate
// address description.  Once reported, the identification description remains static
// for the lifetime of the device.  For example, the identification description that the
// PCI bus driver would use for a child would contain the vendor ID, device ID,
// subsystem ID, revision, and class for the device. This sample uses only identification
// description.
// On other busses (like 1394 and auto LUN SCSI), the device is assigned a dynamic
// address by the hardware (which may reassigned and updated periodically); in these
// instances the driver will use the address description to encapsulate this dynamic piece
// of data.    For example in a 1394 driver, the address description would contain the
// device's current generation count while the identification description would contain
// vendor name, model name, unit spec ID, and unit software version.
//
typedef struct _PDO_IDENTIFICATION_DESCRIPTION
    {
    WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Header; // should contain this header

    // Unique serail number of the device on the bus
     ULONG SerialNo;

    size_t CchHardwareIds;
     __field_bcount(CchHardwareIds) PWCHAR HardwareIds;
    } PDO_IDENTIFICATION_DESCRIPTION, *PPDO_IDENTIFICATION_DESCRIPTION;

typedef struct _MADSIM_DMA_ENABLER_CONTEXT
    {
    WDFDEVICE             hFDO;
    } MADSIM_DMA_ENABLER_CONTEXT, *PMADSIM_DMA_ENABLER_CONTEXT;
//
WDF_DECLARE_CONTEXT_TYPE(MADSIM_DMA_ENABLER_CONTEXT)

typedef struct _MADSIM_DMA_TRANSACTION_CONTEXT
    {
    WDFDEVICE             hFDO;
    WDFDMAENABLER         hDmaEnabler;
    WDFREQUEST            hRequest;
    WDF_DMA_DIRECTION     Direxn;
    PSCATTER_GATHER_LIST  pSgList;
    } MADSIM_DMA_TRANSACTION_CONTEXT, *PMADSIM_DMA_TRANSACTION_CONTEXT;
//
WDF_DECLARE_CONTEXT_TYPE(MADSIM_DMA_TRANSACTION_CONTEXT)

// These defines below are for creating object names in the object data base 
//
#define MADBUS_NT_DEVICE_NAME_WSTR             L"\\Device\\MadBusPDOx"
#define MADBUS_NT_DEVICE_NAME_UNITID_INDX     17 //Index of this X  ^

// The device extension of the bus itself.  From whence the PDO's are born.
typedef struct _FDO_DEVICE_DATA
{
    ULONG                        SerialNo;
    PDRIVER_DATA                 pDriverData;
    MADBUS_WMI_DATA              MadBusWmiData;
    MADBUS_DEVICE_CONFIG_DATA    BusInstanceConfigData; ; // DfltCnfgData;
    PVOID                        pDeviceSetBase;
    PHYSICAL_ADDRESS             liDeviceSetBase;
    ULONG                        OurVector;
    KIRQL                        OurIRQL;
    KAFFINITY                    OurAffinity;
    //
    ULONG                        CurrNumPDOs;
    PDEVICE_OBJECT               pDevObjs[4];
    PDO_DEVICE_OIDS              PdoDevOidsList[MAD_MAX_DEVICES + 1]; //Skip serial# zero
} FDO_DEVICE_DATA, *PFDO_DEVICE_DATA;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_DEVICE_DATA, MadBusFdoGetData)

//IO_RESOURCE_LIST with a defined size for the descriptors array
typedef struct _MAD_IO_RESRC_LIST
{
    USHORT                 Version;
    USHORT                 Revision;
    ULONG                  Count;
    IO_RESOURCE_DESCRIPTOR Descriptors[MADDEV_TOTAL_CM_RESOURCES];
} MAD_IO_RESRC_LIST, * PMAD_IO_RESRC_LIST;

//IO_RESOURCE_REQUIREMENTS LIST with a defined size for the resource list
typedef struct _MAD_IO_RESRC_REQ_LIST
{
    ULONG             ListSize;
    INTERFACE_TYPE    InterfaceType;
    ULONG             BusNumber;
    ULONG             SlotNumber;
    ULONG             Reserved[3];
    ULONG             AlternativeLists;
    MAD_IO_RESRC_LIST MadIoResrcList[MAD_NUM_RESRC_LISTS];
} MAD_IO_RESRC_REQ_LIST, * PMAD_IO_RESRC_REQ_LIST;

// This is PDO device-extension.
typedef struct _PDO_DEVICE_DATA
    {
    // Unique serial number of the device on the bus
    ULONG               SerialNo;
    WDFDEVICE           hDevice;
    PFDO_DEVICE_DATA    pFdoData;

    PNP_BUS_INFORMATION            PnpBusInfo;
    BUS_INTERFACE_STANDARD         BusInterfaceStd;
    PNP_LOCATION_INTERFACE         PnpLocInterface;
    PNP_EXTENDED_ADDRESS_INTERFACE PnpXtendedAddrIntf;
    MAD_IO_RESRC_REQ_LIST          MadIoResrcReqList;
    REENUMERATE_SELF_INTERFACE_STANDARD ReEnumSelfStd;

    INTERFACE                DfltInterface;

    //Device interrupt thread Parameters
    HANDLE              hDevIntThread;
    KEVENT              evIntThreadExit;
    PETHREAD            pIntThreadObj;

    //Resource parameters
     ULONG                     IDTindx;
    //ULONG                     IntVector;
    KIRQL                     Irql;
    KAFFINITY                 IntAffinity;
    PHYSICAL_ADDRESS          liDevBase;
    PHYSICAL_ADDRESS          liDevPioRead;
    PHYSICAL_ADDRESS          liDevPioWrite;
    PHYSICAL_ADDRESS          liDeviceData;
    PMADREGS                  pMadRegs;
    PVOID                     pPioRead;
    PVOID                     pPioWrite;
    PVOID                     pReadCache;
    PVOID                     pWriteCache;
    PVOID                     pDeviceData;

    MADBUS_DEVICE_CONFIG_DATA      BusPdoCnfgData;
    MAD_SIMULATION_INT_PARMS       MadSimIntParms;    
    WDFDMAENABLER             hDmaEnabler;
    PDMA_ADAPTER              pDmaAdapterRd;
    PDMA_ADAPTER              pDmaAdapterWr;

    //#ifdef _MAD_SIMULATION_MODE_ //We need to generate a table of pointers to WDF-DMA simulation functions
    PMADSIM_KMDF_DMA_FUNXNS    pMadSimDmaFunxns;
    PMADSIM_STORPORT_IO_FUNXNS pMadSimSpIoFunxns;
    PFN_WDF_PROGRAM_DMA        pEvtProgramDmaFunxn;

    UCHAR SgList[sizeof(SCATTER_GATHER_ELEMENT) * MAD_DMA_MAX_SGLIST_SIZE + 20];
    //#endif

    #ifdef TTIOD2PNP
    CM_PARTIAL_RESOURCE_DESCRIPTOR  CmPartlResrcDescs[MADDEV_TOTAL_CM_RESOURCES];
    IO_RESOURCE_DESCRIPTOR          IoResrcDescs[MADDEV_TOTAL_CM_RESOURCES];
    WDFIORESLIST                    hIoResrcList;
    #endif

    UCHAR               ResrcList[1000];     //TBD Plenty of space inline - need to optimize
    UCHAR               ResrcListXlatd[1000];
    } PDO_DEVICE_DATA, *PPDO_DEVICE_DATA;
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PDO_DEVICE_DATA, MadBusPdoGetData)
#define PDO_DEVICE_DATA_DEF

#include "..\Inc\MadBusDdiQi.h"

// Prototypes of functions
extern "C" {
    DRIVER_INITIALIZE          DriverEntry;
    EVT_WDF_DRIVER_UNLOAD      MadBusEvtDriverUnload;
    EVT_WDF_DRIVER_DEVICE_ADD  MadBusEvtDeviceAdd;
    EVT_WDF_DEVICE_FILE_CREATE MadBusEvtFileCreate;
    EVT_WDF_FILE_CLOSE         MadBusEvtFileClose;

    EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL MadBusEvtIoDeviceControl;

    EVT_WDF_CHILD_LIST_CREATE_DEVICE                        MadBusEvtDeviceListCreatePdo;
    EVT_WDF_CHILD_LIST_IDENTIFICATION_DESCRIPTION_COMPARE   MadBusEvtChildListIdentificationDescriptionCompare;
    EVT_WDF_CHILD_LIST_IDENTIFICATION_DESCRIPTION_CLEANUP   MadBusEvtChildListIdentificationDescriptionCleanup;
    EVT_WDF_CHILD_LIST_IDENTIFICATION_DESCRIPTION_DUPLICATE MadBusEvtChildListIdentificationDescriptionDuplicate;
    EVT_WDF_DEVICE_PROCESS_QUERY_INTERFACE_REQUEST          MadBusPdoEvtProcessQueryInterfaceRequest;

    NTSTATUS MadBus_PlugInDevice(__in WDFDEVICE Device, __in PWCHAR HardwareIds, 
                                 __in size_t CchHardwareIds, __in ULONG SerialNo);
    NTSTATUS MadBus_UnPlugDevice(WDFDEVICE Device, ULONG SerialNo);
    NTSTATUS MadBus_EjectDevice(WDFDEVICE Device, ULONG SerialNo);
    NTSTATUS MadBus_CreatePdo(__in WDFDEVICE Device, __in PWDFDEVICE_INIT ChildInit, 
                              __in PWCHAR HardwareIds, __in ULONG SerialNo);
    NTSTATUS MadBus_DoStaticEnumeration(IN WDFDEVICE Device);
    VOID     MadBusPdoFreeResrcs(WDFDEVICE hChild, ULONG SerialNo);
    NTSTATUS MadBusFdo_QueryDeviceRelations(PFDO_DEVICE_DATA pFdoData, PIRP pIRP,
                                            PIO_STACK_LOCATION pIoStackLoc);

    // Interface functions
    //BUS_INTERFACE_STANDARD
    TRANSLATE_BUS_ADDRESS MadBusPdoXlateBusAddr;
    GET_DMA_ADAPTER       MadBusPdoGetDmaAdapter;
    GET_SET_DEVICE_DATA   MadBusPdoSetConfigData;
    GET_SET_DEVICE_DATA   MadBusPdoGetConfigData;

    BOOLEAN MadBus_GetPowerLevel(IN   WDFDEVICE ChildDevice, OUT  PUCHAR Level);
    BOOLEAN MadBus_SetPowerLevel(IN   WDFDEVICE ChildDevice, OUT  UCHAR Level);
    BOOLEAN MadBus_IsSafetyLockEnabled(WDFDEVICE ChildDevice);
    //PGET_LOCATION_STRING  RasBusPdoGetLocString;
    NTSTATUS MadBusPdoGetLocString(_Inout_  PVOID Context, _Out_ PWCHAR* LocationStrings);

    // Defined in MadBusWmi.c
    EVT_WDF_WMI_INSTANCE_SET_ITEM        MadBusWmiEvtSetInfoInstanceDataItem;
    EVT_WDF_WMI_INSTANCE_SET_INSTANCE    MadBusWmiEvtSetInfoInstanceData;
    EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE  MadBusWmiEvtQueryInfoInstanceData;
    EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE  MadBusWmiEvtInstanceQueryDevControl;
    EVT_WDF_WMI_INSTANCE_EXECUTE_METHOD  MadBusWmiEvtInstance_DevCntlExecMethod;
    NTSTATUS                             MadBus_WmiRegistration(WDFDEVICE hDevice);

    //Additions to WDF-Toaster framework
    KSTART_ROUTINE   MadPdoIntThread;
    KSTART_ROUTINE   MadPdoScsiIntThread;
    VOID MadBusPdoSetPowerUp(ULONG SerialNo);
    VOID MadBusPdoSetPowerDown(ULONG SerialNo);
    MADDEV_IO_TYPE   MadDevDetermineIo(PPDO_DEVICE_DATA pPdoData);
    VOID             MadBusPdoProcessDevIo(PPDO_DEVICE_DATA pPdoData, MADDEV_IO_TYPE DevReqType);
    VOID             MadBusPdoProcessDevSgDma(PPDO_DEVICE_DATA pPdoData, MADDEV_IO_TYPE DevReqType);
    NTSTATUS         MadBus_SetPowerState(WDFDEVICE hDevice, PMADBUS_SET_POWER_STATE pSetPowerState);
    //VOID             MadBus_PowerIrpCompletion(PDEVICE_OBJECT pDevObj, UCHAR MinorFunction,
    //                                           POWER_STATE PowerState, PVOID pContext, PIO_STATUS_BLOCK pIoStatus); 
    void     MadBus_InitDriverContextAndGlobals(PDRIVER_DATA pDriverData,
                                               WDFDRIVER hDriver, PDRIVER_OBJECT pDriverObj,
                                               PUNICODE_STRING pRegistryPath);
    void     MadBus_InitConfigSpaceDflts(PMADBUS_DEVICE_CONFIG_DATA pMadDevCnfg);

    NTSTATUS MadBusEvtDevCntl_DevSimUsrInt(IN WDFQUEUE hQueue,
                                           IN WDFREQUEST hRequest,
                                           IN size_t  OutBufrLen,
                                           IN size_t InBufrLen,
                                           IN ULONG  IoControlCode);
    NTSTATUS MadBus_MapWholeDevice(PFDO_DEVICE_DATA pFdoData,
                                   PMADBUS_MAP_WHOLE_DEVICE pMapWholeDevice,
                                   ULONG SerialNo);

    PVOID MadBus_AllocDeviceSet(IN PDRIVER_OBJECT pDriverObj,
                                IN PMADBUS_DEVICE_CONFIG_DATA pCnfgData,
                                PULONG pNumDevices);
    void  MadBusPdo_PlugCmResrcsIntoFdoStackLoc(PPDO_DEVICE_DATA pPdoData, PIRP pIRP,
                                                IN PIO_STACK_LOCATION pIoStackLoc);
    void MadBusPdo_ProcessDeviceControlIoctl(PPDO_DEVICE_DATA pPdoData, PIRP pIRP,
                                             IN PIO_STACK_LOCATION pIoStackLoc);
    BOOLEAN MadBusPdo_ProcessQueryInterface(PPDO_DEVICE_DATA pPdoData,
                                            PIO_STACK_LOCATION pIoStackLoc,
                                            PGUID pInterfaceType);
    NTSTATUS 
    MadBusPdoProcessQueryInterfaceRequest(PPDO_DEVICE_DATA  pPdoData,
                                          PDO_QUERY_INTF_TYPE eIntfQ,
                                          _Inout_ PINTERFACE pExposedInterface,
                                          _Inout_opt_ PVOID pInterfaceSpecificData);
    VOID MadBusPdoReEnumerateSelf(PVOID Context);
    //PQUERYEXTENDEDADDRESS QueryExtendedAddress;
    VOID MadBusPdoQueryExtendedAddress(_In_ PVOID Context, 
                                       _Out_ PULONG64 ExtendedAddress);

    //void  SetStaticCmResrcs(PPDO_DEVICE_DATA pPdoData);
    //void  SetStaticIoResrcs(PPDO_DEVICE_DATA pPdoData);

    NTSTATUS
    MadBusReadRegParms(IN PDRIVER_OBJECT pDriverObj,   // Driver object
                       IN PUNICODE_STRING pParmPath,    // base path to keys
                       OUT PULONG pDevIRQL, OUT PULONG pIdtBaseDX,
                       PBOOLEAN pbAffinityOn,
                       OUT PULONG pNumFilters, OUT PULONG pMaxDevices,
                       OUT PULONG pMadDevDataXtent);

    NTSTATUS MadBusReadRegConfig(IN WDFDEVICE hDevice,
                                 IN PUNICODE_STRING pCnfgPath,
                                 OUT PMADBUS_DEVICE_CONFIG_DATA pCnfgData);

    // WDF-prototyped functions;
    EVT_WDF_DEVICE_PREPARE_HARDWARE              MadBusPdoEvtPrepareHardware;
    EVT_WDFDEVICE_WDM_IRP_PREPROCESS             MadBusPdoEvtWdmIrpPreprocess;
    EVT_WDF_DEVICE_D0_ENTRY                      MadBusPdoEvtD0Entry;
    EVT_WDF_DEVICE_D0_EXIT                       MadBusPdoEvtD0Exit;
    EVT_WDF_DEVICE_RESOURCES_QUERY               MadBusPdoEvtResrcsQuery;
    EVT_WDF_DEVICE_RESOURCE_REQUIREMENTS_QUERY   MadBusPdoEvtResrcReqsQuery;

    #ifdef MAD_KERNEL_WONT_CREATE_DMA_ADAPTER 
    NTSTATUS   MadSimDmaEnablerCreate(IN WDFDEVICE Device, IN PWDF_DMA_ENABLER_CONFIG Config, 
                                      IN OPTIONAL PWDF_OBJECT_ATTRIBUTES Attributes,
                                      OUT WDFDMAENABLER* DmaEnablerHandle);
    VOID       MadSimDmaEnablerSetMaximumScatterGatherElements(IN WDFDMAENABLER DmaEnabler,
                                                               IN size_t MaximumFragments);
    NTSTATUS   
    MadSimDmaTransactionCreate(IN WDFDMAENABLER DmaEnabler, 
                               IN OPTIONAL WDF_OBJECT_ATTRIBUTES* Attributes, 
                               OUT WDFDMATRANSACTION* DmaTransaction);
    NTSTATUS  
    MadSimDmaTransactionInitializeUsingRequest(IN WDFDMATRANSACTION DmaTransaction, 
                                               IN WDFREQUEST Request,
                                               IN PFN_WDF_PROGRAM_DMA EvtProgramDmaFunction,
                                               IN WDF_DMA_DIRECTION DmaDirection);
    NTSTATUS   MadSimDmaTransactionExecute(IN WDFDMATRANSACTION DmaTransaction,
                                           IN OPTIONAL PVOID Context);
    WDFREQUEST 
    MadSimDmaTransactionGetRequest(IN WDFDMATRANSACTION DmaTransaction);
    size_t     
    MadSimDmaTransactionGetBytesTransferred(IN WDFDMATRANSACTION DmaTransaction);
    BOOLEAN    
    MadSimDmaTransactionDmaCompleted(IN WDFDMATRANSACTION DmaTransaction,
                                     OUT NTSTATUS* Status);
    WDFDEVICE  MadSimDmaTransactionGetDevice(IN WDFDMATRANSACTION hDmaXaxn);
    #endif //MAD_KERNEL_WONT_CREATE_DMA_ADAPTER
}    // End extern "C"cls

#ifdef PDO_DEVICE_DATA_DEF //Because this header file is included in to many source files

inline VOID MadBusPdoInitPnpBusInfo(PPDO_DEVICE_DATA pPdoData)
{
    pPdoData->PnpBusInfo.BusNumber = pPdoData->pFdoData->SerialNo;
    if (pPdoData->pFdoData->SerialNo == eGenericBus)
        {
        pPdoData->PnpBusInfo.BusTypeGuid   = MAD_GUID_BUS_TYPE; 
        pPdoData->PnpBusInfo.LegacyBusType = MAD_BUS_INTERFACE_TYPE;
        }
    else
        {
        pPdoData->PnpBusInfo.BusTypeGuid   = MAD_GUID_BUS_TYPE_SCSI; 
        pPdoData->PnpBusInfo.LegacyBusType = MAD_BUS_INTERFACE_TYPE_SCSI;
        }
}

inline VOID MadBusPdoInitInterface(PPDO_DEVICE_DATA pPdoData, 
                                   PINTERFACE pInterface, USHORT Size)
{
    pInterface->Size = Size;
    pInterface->Version = 1;
    pInterface->Context = pPdoData;

    // Let the framework handle reference counting.
    pInterface->InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
    pInterface->InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;
}

inline VOID MadBusPdoInitBusInterface(PPDO_DEVICE_DATA pPdoData)
{
    MadBusPdoInitInterface(pPdoData,
                           (PINTERFACE)&pPdoData->BusInterfaceStd,
                           sizeof(BUS_INTERFACE_STANDARD));
    pPdoData->BusInterfaceStd.TranslateBusAddress = MadBusPdoXlateBusAddr;
    pPdoData->BusInterfaceStd.GetDmaAdapter = MadBusPdoGetDmaAdapter;
    pPdoData->BusInterfaceStd.SetBusData = MadBusPdoSetConfigData;
    pPdoData->BusInterfaceStd.GetBusData = MadBusPdoGetConfigData;
}

inline VOID MadBusPdoInitPnpLocInterface(PPDO_DEVICE_DATA pPdoData)
{
    MadBusPdoInitInterface(pPdoData,
                          (PINTERFACE)&pPdoData->PnpLocInterface,
                          sizeof(PNP_LOCATION_INTERFACE));
    pPdoData->PnpLocInterface.GetLocationString = MadBusPdoGetLocString;
}

inline VOID MadBusPdoInitReEnumSelf(PPDO_DEVICE_DATA pPdoData)
{
    MadBusPdoInitInterface(pPdoData,
                           (PINTERFACE)&pPdoData->ReEnumSelfStd,
                           sizeof(REENUMERATE_SELF_INTERFACE_STANDARD));

    //Here we specify the ReEnumerate function - but it's a do-nothing
    pPdoData->ReEnumSelfStd.SurpriseRemoveAndReenumerateSelf =
                            MadBusPdoReEnumerateSelf;
}

inline VOID MadBusPdoInitPnpXtendedAddr(PPDO_DEVICE_DATA pPdoData)
{
    MadBusPdoInitInterface(pPdoData,
                           (PINTERFACE)&pPdoData->PnpXtendedAddrIntf,
                           sizeof(PNP_EXTENDED_ADDRESS_INTERFACE));

    pPdoData->PnpXtendedAddrIntf.QueryExtendedAddress =
                                 MadBusPdoQueryExtendedAddress;
}

inline VOID MadBusPdoSetPnpPowerCallbacks(WDF_PNPPOWER_EVENT_CALLBACKS*  pPnpPowerCallbacks)
{
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(pPnpPowerCallbacks);
    pPnpPowerCallbacks->EvtDevicePrepareHardware = MadBusPdoEvtPrepareHardware;
    //pPnpPowerCallbacks.EvtDeviceReleaseHardware   = MadEvtReleaseHardware;
    //pPnpPowerCallbacks.EvtDeviceSelfManagedIoInit = MadEvtSelfManagedIoInit;

    // Register Power callbacks.
    // If we don't supply any callbacks, the Framework will take appropriate default actions based on whether
    // pDeviceInit is initialized to be an FDO, a PDO or a filter device object.
    pPnpPowerCallbacks->EvtDeviceD0Entry = MadBusPdoEvtD0Entry;
    pPnpPowerCallbacks->EvtDeviceD0Exit = MadBusPdoEvtD0Exit;
}

inline VOID MadBusPdoInitPnpCapabilities(WDF_DEVICE_PNP_CAPABILITIES* pPnpCaps, ULONG sernum)
{
    WDF_DEVICE_PNP_CAPABILITIES_INIT(pPnpCaps);
    pPnpCaps->Removable = WdfTrue;
    pPnpCaps->EjectSupported = WdfTrue;
    pPnpCaps->SurpriseRemovalOK = WdfTrue;
    pPnpCaps->Address = sernum;
    pPnpCaps->UINumber = sernum;
}

inline VOID MadBusPdoInitPowerCapabilities(WDF_DEVICE_POWER_CAPABILITIES* pPowerCaps)
{
    WDF_DEVICE_POWER_CAPABILITIES_INIT(pPowerCaps);
    pPowerCaps->DeviceD1 = WdfTrue;
    pPowerCaps->WakeFromD1 = WdfTrue;
    pPowerCaps->DeviceWake = PowerDeviceD1;
    pPowerCaps->DeviceState[PowerSystemWorking] = PowerDeviceD1;
    pPowerCaps->DeviceState[PowerSystemSleeping1] = PowerDeviceD1;
    pPowerCaps->DeviceState[PowerSystemSleeping2] = PowerDeviceD2;
    pPowerCaps->DeviceState[PowerSystemSleeping3] = PowerDeviceD2;
    pPowerCaps->DeviceState[PowerSystemHibernate] = PowerDeviceD3;
    pPowerCaps->DeviceState[PowerSystemShutdown] = PowerDeviceD3;
}

inline VOID MadBusPdoInitIoResrcs(PPDO_DEVICE_DATA pPdoData)
{
PIO_RESOURCE_REQUIREMENTS_LIST pIoResrcReqList = 
    (PIO_RESOURCE_REQUIREMENTS_LIST)&pPdoData->MadIoResrcReqList;
PIO_RESOURCE_LIST pIoResrcList = 
                 (PIO_RESOURCE_LIST)&pPdoData->MadIoResrcReqList.MadIoResrcList[0];

    //pIoResrcReqList->ListSize       = sizeof(MAD_IO_RESRC_REQ_LIST);
    pIoResrcReqList->ListSize         = sizeof(IO_RESOURCE_REQUIREMENTS_LIST);
    pIoResrcReqList->BusNumber        = pPdoData->pFdoData->SerialNo; //OUR bus number
    pIoResrcReqList->SlotNumber       = pPdoData->SerialNo; 
    pIoResrcReqList->AlternativeLists = MAD_NUM_RESRC_LISTS;
    pIoResrcReqList->InterfaceType    = pPdoData->PnpBusInfo.LegacyBusType;
    //return;
       
    pIoResrcList->Version  = 1;
    pIoResrcList->Revision = 1;
    pIoResrcList->Count    = MADDEV_TOTAL_CM_RESOURCES;

    pIoResrcList->Descriptors[0].Option = 0; // IO_RESOURCE_PREFERRED;
    pIoResrcList->Descriptors[0].Type   = CmResourceTypeInterrupt; 
    pIoResrcList->Descriptors[0].Flags  = CM_RESOURCE_INTERRUPT_LATCHED;
    pIoResrcList->Descriptors[0].ShareDisposition = 
                                 CmResourceShareDeviceExclusive;

    pIoResrcList->Descriptors[1].Option = 0; // IO_RESOURCE_PREFERRED;
    pIoResrcList->Descriptors[1].Type   = CmResourceTypePort;
    pIoResrcList->Descriptors[1].Flags  = CM_RESOURCE_PORT_MEMORY;
    pIoResrcList->Descriptors[1].ShareDisposition =
                                 CmResourceShareDeviceExclusive;
    pIoResrcList->Descriptors[1].u.Port.Length = PAGE_SIZE;

    pIoResrcList->Descriptors[2].Option = 0; // IO_RESOURCE_PREFERRED;
    pIoResrcList->Descriptors[2].Type   = CmResourceTypeMemory;
    pIoResrcList->Descriptors[2].Flags  = CM_RESOURCE_MEMORY_READ_WRITE;
    pIoResrcList->Descriptors[2].ShareDisposition =
                                 CmResourceShareDeviceExclusive;
    pIoResrcList->Descriptors[2].u.Memory.Length = PAGE_SIZE;

    pIoResrcList->Descriptors[3].Option = 0; // IO_RESOURCE_PREFERRED;
    pIoResrcList->Descriptors[3].Type   = CmResourceTypeMemory;
    pIoResrcList->Descriptors[3].Flags  = CM_RESOURCE_MEMORY_READ_WRITE;
    pIoResrcList->Descriptors[3].ShareDisposition =
                                 CmResourceShareDeviceExclusive;
    pIoResrcList->Descriptors[3].u.Memory.Length = PAGE_SIZE;
}

inline VOID MadBusPdoSetPciDeviceAddresses(PPDO_DEVICE_DATA pPdoData)
{
SIZE_T  TotalDevSize = (MAD_DEVICE_MEM_SIZE_NODATA + 
                       (SIZE_T)pPdoData->pFdoData->pDriverData->MadDataXtent);
ULONG DevOffset      = (ULONG)((pPdoData->SerialNo - 1) * TotalDevSize);
ULONG DeviceBase     = pPdoData->pFdoData->liDeviceSetBase.LowPart + DevOffset;

    pPdoData->BusPdoCnfgData.LegacyPci.u.type0.BaseAddresses[0] = DeviceBase;
    pPdoData->BusPdoCnfgData.LegacyPci.u.type0.BaseAddresses[1] = DeviceBase + MAD_MAPD_READ_OFFSET;
    pPdoData->BusPdoCnfgData.LegacyPci.u.type0.BaseAddresses[2] = DeviceBase + MAD_MAPD_WRITE_OFFSET;
    pPdoData->BusPdoCnfgData.LegacyPci.u.type0.BaseAddresses[3] = DeviceBase + MAD_DEVICE_DATA_OFFSET;
}

inline VOID MadBusPdoSetPciVendorData(PPDO_DEVICE_DATA pPdoData, ULONG sernum)
{
    //Leave device range physical addrs. alone after config registry read
    pPdoData->BusPdoCnfgData.LegacyPci.VendorID = MAD_VENDOR_ID;
    pPdoData->BusPdoCnfgData.LegacyPci.u.type0.SubVendorID = MAD_VENDOR_ID;

    pPdoData->BusPdoCnfgData.LegacyPci.DeviceID =
    (pPdoData->pFdoData->SerialNo == eGenericBus) ? MAD_DEVICE_ID_GENERIC : 
                                                    MAD_DEVICE_ID_DISK;
    pPdoData->BusPdoCnfgData.LegacyPci.u.type0.SubSystemID =
              pPdoData->BusPdoCnfgData.LegacyPci.DeviceID;

    pPdoData->BusPdoCnfgData.VndrSpecData.BusSlotNum = sernum;
    pPdoData->BusPdoCnfgData.VndrSpecData.IntMode = MADDEV_INT_MODE;
    pPdoData->BusPdoCnfgData.VndrSpecData.Irql = pPdoData->Irql;
    pPdoData->BusPdoCnfgData.VndrSpecData.IntVector = pPdoData->IDTindx;
    pPdoData->BusPdoCnfgData.VndrSpecData.DevPowerState = PowerDeviceD0; //Starting powered up

    //So that the device driver/miniport driver can exchange parms in simulation_mode
#ifdef MAD_KERNEL_WONT_CREATE_DMA_ADAPTER
    pPdoData->BusPdoCnfgData.VndrSpecData.pMadSimIntParms = 
               &pPdoData->MadSimIntParms;
    if (pPdoData->pFdoData->SerialNo == eGenericBus)
        pPdoData->BusPdoCnfgData.VndrSpecData.pMadSimDmaFunxns = 
                  &pPdoData->pFdoData->pDriverData->MadSimDmaFunxns;
    else
        pPdoData->BusPdoCnfgData.VndrSpecData.pMadSimSpIoFunxns = 
                  &pPdoData->pFdoData->pDriverData->MadSimSpIoFunxns;
#endif //MAD_KERNEL_WONT_CREATE_DMA_ADAPTER
}

inline VOID MadBusPdoSetDeviceAddresses(PPDO_DEVICE_DATA pPdoData)
{
    pPdoData->liDevBase.LowPart = pPdoData->BusPdoCnfgData.LegacyPci.u.type0.BaseAddresses[0];
    pPdoData->liDevBase.HighPart = 0L;
    pPdoData->liDevPioRead.LowPart = pPdoData->BusPdoCnfgData.LegacyPci.u.type0.BaseAddresses[1];
    pPdoData->liDevPioRead.HighPart = 0L;
    pPdoData->liDevPioWrite.LowPart = pPdoData->BusPdoCnfgData.LegacyPci.u.type0.BaseAddresses[2];
    pPdoData->liDevPioWrite.HighPart = 0L;
    pPdoData->liDeviceData.LowPart = pPdoData->BusPdoCnfgData.LegacyPci.u.type0.BaseAddresses[3];
    pPdoData->liDeviceData.HighPart = 0L;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadBusPdoSetDeviceAddresses... BAR_0=%X:%08X BAR_1=%X:%08X BAR_2=%X:%08X BAR_2=%X:%08X\n",
                pPdoData->liDevBase.HighPart, pPdoData->liDevBase.LowPart,
                pPdoData->liDevPioRead.HighPart, pPdoData->liDevPioRead.LowPart,
                pPdoData->liDevPioWrite.HighPart, pPdoData->liDevPioWrite.LowPart,
                pPdoData->liDeviceData.HighPart, pPdoData->liDeviceData.LowPart);

    //Assign virt addrs for the device
    pPdoData->pMadRegs = (PMADREGS)MmMapIoSpaceEx(pPdoData->liDevBase,
                                                  MAD_REGISTER_BLOCK_SIZE,
                                                  (PAGE_READWRITE | PAGE_NOCACHE));
    pPdoData->pPioRead = MmMapIoSpaceEx(pPdoData->liDevPioRead,
                                        MAD_MAPD_READ_SIZE, 
                                        (PAGE_READWRITE | PAGE_NOCACHE));
    pPdoData->pReadCache = pPdoData->pPioRead;
    pPdoData->pPioWrite = MmMapIoSpaceEx(pPdoData->liDevPioWrite,
                                         MAD_MAPD_WRITE_SIZE, 
                                         (PAGE_READWRITE | PAGE_NOCACHE));
    pPdoData->pWriteCache = pPdoData->pPioWrite;
    pPdoData->pDeviceData = MmMapIoSpaceEx(pPdoData->liDeviceData,
                                           pPdoData->pFdoData->pDriverData->MadDataXtent,
                                           (PAGE_READWRITE | PAGE_NOCACHE));

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadBusPdoSetDeviceAddresses... MadRegs=%p PioRead=%p PioWrite=%p Data=%p\n",
                pPdoData->pMadRegs, pPdoData->pReadCache, 
                pPdoData->pWriteCache, pPdoData->pDeviceData);
}

//Initialize our device thread parameters 
inline NTSTATUS MadBusPdoInitDeviceThreadParms(PPDO_DEVICE_DATA pPdoData)
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    //Initialize our interrupt thread synchronization events 
    KeInitializeEvent(&pPdoData->evIntThreadExit, NotificationEvent, FALSE);
    KeInitializeEvent(&pPdoData->MadSimIntParms.evDevPowerUp, NotificationEvent, FALSE);
    KeInitializeEvent(&pPdoData->MadSimIntParms.evDevPowerDown, NotificationEvent, FALSE);
    pPdoData->MadSimIntParms.pEvIntThreadExit = &pPdoData->evIntThreadExit;

    if (pPdoData->pFdoData->SerialNo == eGenericBus)
        NtStatus = PsCreateSystemThread(&pPdoData->hDevIntThread,
                                        (SYNCHRONIZE | GENERIC_EXECUTE),
                                         NULL, NULL, NULL,
                                         MadPdoIntThread, pPdoData);
    else
        {
        pPdoData->MadSimIntParms.u.StorIntParms.pSetPowerUpEvFunxn =
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
        "MadBus_CreatePdo:PsCreateSystemThread... Bus#:Ser#=%d:%d ntstatus=x%X\n",
                    pPdoData->pFdoData->SerialNo, pPdoData->SerialNo, NtStatus);

        MadWriteEventLogMesg(pPdoData->pFdoData->pDriverData->pDriverObj, 
                             MADBUS_CREATE_CHILD_DEVICE_ERROR, 1,
                             sizeof(NTSTATUS), (PWSTR)&NtStatus); //Minimal payload
        }

    return NtStatus;
}

//We receive the device driver FDO for it's device and detemine
//the PDO context so that we can simulate Driver-Framework DMA functions
inline PPDO_DEVICE_DATA MadBusGetPdoContextFromFdo(WDFDEVICE hFDO)
{
    PDEVICE_OBJECT    pPhysDevObj = WdfDeviceWdmGetPhysicalDevice(hFDO);
    WDFDEVICE         hDevPDO = WdfWdmDeviceGetWdfDeviceHandle(pPhysDevObj);
    PPDO_DEVICE_DATA  pPdoData = MadBusPdoGetData(hDevPDO);
    return pPdoData;
}
#endif //PPDO_DEVICE_DATA_DEF
#endif //_MADBUS_H_