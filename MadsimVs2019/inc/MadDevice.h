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
/*  Exe file ID  : MadDevice.sys                                               */
/*                                                                             */
/*  Module  NAME : MadDevice.h                                                 */
/*                                                                             */
/*  DESCRIPTION  : Definitions of structures and function prototypes for       */
/*                 MadDevice.sys                                               */
/*                 Derived from WDK-Toaster\func                               */
/*                                                                             */
/*******************************************************************************/

#if !defined(_MADDEVICE_H_)
#define _MADDEVICE_H_

#define CREATE_SYMBOLIC_LINK       //Create a symbolic link

#include <ntddk.h>
#include <wdf.h>
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include <initguid.h>


#include "..\Includes\MadDefinition.h"
#include "..\Includes\MadGUIDs.h"
//#include "wmilib.h"
#include "..\Includes\MadBusConfig.h"
#include "..\Includes\MadDevIoctls.h"
#include "..\Includes\MadUtilFunxns.h"
#include "MadDeviceMof.h"
#include "..\MadDevice\MadDeviceMC\MadDevice.h"

extern "C" {
#include "..\Includes\MadRegFunxns.h"
}
#include "MadBusDdiQi.h"

#define MAD_DEV_POOL_TAG (ULONG) 'DdaM'
#define MOFRESOURCENAME L"MadDevWMI" //Must match the resource name specified in the .RC file

// These define below is for creating object names in the object data base 
// The symbolic link name will serve open statements like this:
// hDevice = CreateFile("\\?\MadDevice1", ...);
//
#define MADDEV_DOS_DEVICE_NAME_WSTR            L"\\DosDevices\\MadDeviceX"
#define MADDEV_DOS_DEVICE_NAME_UNITID_INDX    21 //Index of the X above ^
//
#define MADDEV_NT_DEVICE_NAME_WSTR             L"\\Device\\NtMadDeviceX"
#define MADDEV_NT_DEVICE_NAME_UNITID_INDX     19 //Index of this X    ^

//Use this define to force the test app to know the interface guid (for N = 2..9)
//#define FORCE_TESTAPP_INTERFACE_GUID_AWARE 

//This is our ISR-DPC & work item context structure
//
typedef enum _MAD_ISR_DPC_WI_TAG_TYPE {eIsrDpc, eBufrdIoWI, eDmaWI, eDevErrWI} MAD_ISR_DPC_WI_TAG;
//
typedef struct _ISR_DPC_WI_CONTEXT
    {
	MAD_ISR_DPC_WI_TAG IsrDpcWiTag;
    MADREGS            MadRegs;
	PVOID              pFdoData;
	WDFREQUEST         hRequest;
	MADDEV_IO_TYPE     DevReqType;
	NTSTATUS           NtStatus;
	ULONG_PTR          InfoLen;
    } ISR_DPC_WI_CONTEXT, *PISR_DPC_WI_CONTEXT;
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(ISR_DPC_WI_CONTEXT, MadGetIsrDpcWiContext)
//
#define MAD_SET_CONTEXT_DATA_FOR_WORKITEM      \
	    pDpcWiData->MadRegs = pIsrDpcData->MadRegs; pDpcWiData->pFdoData = pIsrDpcData->pFdoData; \
        pDpcWiData->hRequest=hRequest;     \
        pDpcWiData->DevReqType=DevReqType; \
        pDpcWiData->NtStatus=NtStatus;
//
#define MAD_INIT_AND_ENQUEUE_DMA_WORKITEM      \
        pDpcWiData = MadGetIsrDpcWiContext(pFdoData->hDmaWI); \
        MAD_SET_CONTEXT_DATA_FOR_WORKITEM; \
        WdfWorkItemEnqueue(pFdoData->hDmaWI);
//
#define MAD_INIT_AND_ENQUEUE_BUFRD_WORKITEM(X)  \
        pDpcWiData = MadGetIsrDpcWiContext(pFdoData->hBufrdIoWI); \
        MAD_SET_CONTEXT_DATA_FOR_WORKITEM; \
        pDpcWiData->InfoLen = X; \
        WdfWorkItemEnqueue(pFdoData->hBufrdIoWI);

//Definiton(s) for Windows Management Instrumentation (WBEM) data
typedef struct _MADDEVICE_WMI_DATA
    {
	//Room for other WMI data-block(s) / classes
	MadDevWmiInfo      InfoClassData;
	//Room for other WMI data-block(s) / classes
    } MADDEVICE_WMI_DATA, *PMDDEVICE_WMI_DATA;


//Our Driver object context
typedef struct _DRIVER_DATA
{
    WDFDRIVER         hDriver;
    PDRIVER_OBJECT    pDriverObj;
    UNICODE_STRING    RegistryPath;
    BOOLEAN           bDmaEnabled; // = MAD_DFLT_DMA_ENABLED;
 }  DRIVER_DATA,      *PDRIVER_DATA;
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DRIVER_DATA, MadDevDriverGetContextData)

// The device extension for the device object
typedef struct _FDO_DATA
{
    ULONG           SerialNo;
	WDFDEVICE       hDevice;
    PDEVICE_OBJECT  pPhysDevObj;
    PDRIVER_OBJECT  pDriverObj;
    PDRIVER_DATA    pDriverData;
    WDFWMIINSTANCE  hMadDevWmiArrivalEvent;

    BOOLEAN     WmiPowerDeviceEnableRegistered;

    MADDEVICE_INTERFACE_STANDARD BusInterface;

	MADDEVICE_WMI_DATA    MadDevWmiData;

    SYSTEM_POWER_STATE    CurrSysPowerState;   // The general system power state
	DEVICE_POWER_STATE    CurrDevPowerState;   // The state of the device(D0 or D3)
    ULONG                 DevStatus;
    NTSTATUS              DevNtStatus;

#ifdef MAD_DEVICE_CONTROL_POWER_MNGT
    WDFTIMER              hPowrChekTimr;
    LONGLONG              PowrChekTimrPeriod;
#endif

    //Resource parameters
    //
    //PDEVICE_OBJECT            pDevObj4Pnp;
    KIRQL                     Irql; 
    ULONG                     IDTindx;
    KAFFINITY                 IntAffinity;
    ULONG                     BARs[10];
    MADBUS_DEVICE_CONFIG_DATA DevCnfgData;

    PMADREGS                  pMadRegs;
    PVOID                     pMadPioRead;
    PVOID                     pMadPioWrite;

    //Interrupt support objects
    WDFINTERRUPT              hInterrupt;
    WDFSPINLOCK               hIntSpinlock;
    WDFWORKITEM               hDmaWI;
	WDFWORKITEM               hBufrdIoWI;
	WDFWORKITEM               hDevErrorWI;

    //DMA variables
	WDFDMAENABLER             hDmaEnabler;
	WDFDMATRANSACTION         hDmaXaxn;

    //Chained DMA
    PHYSICAL_ADDRESS          liCDPP; //Physical addr of devices's SG-List... will point to below 
    MAD_DMA_CHAIN_ELEMENT     HdwSgDmaList[MAD_DMA_MAX_SECTORS+1]; //prototype: static size 

	// This array is a set of pending requests.
	// We make the assumption of one posible pending request per request type, 
	// even though there can be only one pending request as long as we have one IoQueue
	// defined as WdfIoQueueDispatchSequential.
    //
    WDFREQUEST                hPendingReqs[eMltplIO];

    #ifdef _MAD_SIMULATION_MODE_
	PMAD_SIMULATION_INT_PARMS pMadSimIntParms; //Used for communicating with the simulation Interrupt thread

        //#ifdef MAD_KERNEL_WONT_CREATE_DMA_ADAPTER 
	    PMADSIM_KMDF_DMA_FUNXNS pMadSimDmaFunxns;
        //#endif
    #endif //_MAD_SIMULATION_MODE_
    }  FDO_DATA, *PFDO_DATA;
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_DATA, MadDeviceFdoGetData)

#define MAD_DEVICE_INDICATES_ERROR   ((IntIdReg & MAD_INT_STATUS_ALERT_BIT) != 0)     

// Connector Types
//
#define MADBUS_MODEL_ZERO  0
#define MADBUS_MODEL_ONE   1

extern "C" {
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD     MadEvtDriverUnload;
EVT_WDF_DRIVER_DEVICE_ADD MadEvtAddDevice;

//EVT_WDF_DEVICE_CONTEXT_CLEANUP MadEvtContextCleanup;
VOID MadEvtContextCleanup(IN WDFDEVICE Device);
EVT_WDF_DEVICE_D0_ENTRY         MadEvtD0Entry;
EVT_WDF_DEVICE_D0_EXIT          MadEvtD0Exit;
EVT_WDF_DEVICE_PREPARE_HARDWARE MadEvtPrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE MadEvtReleaseHardware;
EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT MadEvtSelfManagedIoInit;

// Io events callbacks.
//
EVT_WDF_IO_QUEUE_IO_READ   MadEvtIoBufrdRead;
EVT_WDF_IO_QUEUE_IO_WRITE  MadEvtIoBufrdWrite;
EVT_WDF_IO_QUEUE_IO_READ   MadEvtIoDmaRead;
EVT_WDF_IO_QUEUE_IO_WRITE  MadEvtIoDmaWrite;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL MadEvtIoDeviceControl;
EVT_WDF_DEVICE_FILE_CREATE MadEvtFileCreate;
EVT_WDF_FILE_CLOSE         MadEvtFileClose;
EVT_WDF_INTERRUPT_ISR      MadEvtISR;
EVT_WDF_INTERRUPT_DPC      MadEvtDPC;
EVT_WDF_INTERRUPT_ENABLE   MadEvtIntEnable;
EVT_WDF_INTERRUPT_DISABLE  MadEvtIntDisable;
EVT_WDF_WORKITEM           MadEvtBufrdIoWorkItem;
EVT_WDF_WORKITEM           MadEvtDmaWorkItem;
EVT_WDF_WORKITEM           MadEvtErrorWorkItem;
EVT_WDF_PROGRAM_DMA        MadEvtProgramDma;

// Power events callbacks
EVT_WDF_DEVICE_ARM_WAKE_FROM_S0       MadEvtArmWakeFromS0;
EVT_WDF_DEVICE_ARM_WAKE_FROM_SX       MadEvtArmWakeFromSx;
EVT_WDF_DEVICE_DISARM_WAKE_FROM_S0    MadEvtDisarmWakeFromS0;
EVT_WDF_DEVICE_DISARM_WAKE_FROM_SX    MadEvtDisarmWakeFromSx;
EVT_WDF_DEVICE_WAKE_FROM_S0_TRIGGERED MadEvtWakeFromS0Triggered;
EVT_WDF_DEVICE_WAKE_FROM_SX_TRIGGERED MadEvtWakeFromSxTriggered;

//WMI support functions .............................................................
//
NTSTATUS  MadDevWmiFireArrivalEvent(__in WDFDEVICE hDevice);
NTSTATUS  MadDeviceWmiRegistration(__in WDFDEVICE Device);

// WMI event callbacks
//
EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE  MadDevWmiEvtQueryInfoInstanceData;
EVT_WDF_WMI_INSTANCE_SET_INSTANCE    MadDevWmiEvtSetInfoInstanceData;
EVT_WDF_WMI_INSTANCE_SET_ITEM        MadDevWmiEvtSetInfoDataItem;
//
EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE  MadDevWmiEvtInstanceQueryDevControl;
EVT_WDF_WMI_INSTANCE_SET_INSTANCE    MadDevWmiEvtInstanceSetDevControl;
EVT_WDF_WMI_INSTANCE_SET_ITEM        MadDevWmiEvtInstanceSetDevControlItem;
EVT_WDF_WMI_INSTANCE_EXECUTE_METHOD  MadDevWmiEvtInstance_DevCntlExecMethod;

//Additions to the WDF Model-Abstract-Demo device 
//Event callbacks
//
EVT_WDF_DEVICE_FILTER_RESOURCE_REQUIREMENTS  MadEvtAddDeviceResrcReqs;
EVT_WDF_DEVICE_FILTER_RESOURCE_REQUIREMENTS  MadEvtRemoveResrcReqs;
//
NTSTATUS MadDev_ProcessConfig(PDEVICE_OBJECT pPrntDev, PDEVICE_OBJECT pDevObj, PFDO_DATA pFdoData);
NTSTATUS 
MadDev_GetCnfgDataFromPdo(PDEVICE_OBJECT pPrntDev, PDEVICE_OBJECT pDevObj, PMADBUS_DEVICE_CONFIG_DATA pMadDevCnfg);
//NTSTATUS
//MadDev_AllocIrpSend2Parent(PDEVICE_OBJECT pPrntDev, PDEVICE_OBJECT pDevObj, PIO_STACK_LOCATION pIoStackLoc);
NTSTATUS MadDev_ReadRegParms(IN PDRIVER_OBJECT  pDriverObj, IN PUNICODE_STRING pRegPath, OUT PBOOLEAN pbDmaEnabled);

//NTSTATUS MadDev_CnfgCmpltnRtn(PDEVICE_OBJECT pDevObj, PIRP pIRP, PVOID pContext);
//
//IO_COMPLETION_ROUTINE MadDev_CnfgCmpltnRtn;

#ifdef MAD_DEVICE_CONTROL_POWER_MNGT
EVT_WDF_TIMER  MadEvtPowerCheckTimer;
#endif

#ifdef POWER_MANAGEMENT_ENABLED
void    MadDev_IssuePowerNotice(PFDO_DATA pFdoData, DEVICE_POWER_STATE  PowerState, POWER_ACTION SysPowerAxn);
BOOLEAN Write_Power_Notice(WCHAR wcNotifyFile[], PVOID pPowerNotify, ULONG Len);
#endif

NTSTATUS MadDev_MapViews(PFDO_DATA pFdoData, PMADDEV_MAP_VIEWS pMadDevMapViews, ULONG SerialNo);

NTSTATUS       MadDevWdfCreateInt(WDFDEVICE hDevice, WDFCMRESLIST hResrcsRaw, WDFCMRESLIST hResrcsXlatd);
BOOLEAN        MadDevWdmISR(PKINTERRUPT pKINT, PVOID Context);
VOID           MadDevInitFDO(PFDO_DATA pFdoData);
BOOLEAN        MadDevVerifyIoReady(PFDO_DATA pFdoData, WDFREQUEST hRequest);
VOID           MadCompleteAnyPendingIOs(PFDO_DATA pFdoData, NTSTATUS NtStatus);
VOID           MadProgramBufrdIo(PFDO_DATA pFdoData,
                                 ULONG ControlReg, ULONG IntEnableReg, ULONG64 IoAddr);  

VOID     Mad_BuildDeviceChainedDmaListFromWinSgList(PFDO_DATA pFdoData, PSCATTER_GATHER_LIST pSgList, BOOLEAN bWrite);

#ifndef _MAD_SIMULATION_MODE_  //Real Mode
     //In real-mode these will just be aliases for the WDF functions
     #define MadInterruptAcquireLock  WdfInterruptAcquireLock
     #define MadInterruptReleaseLock  WdfInterruptReleaseLock
#else //_MAD_SIMULATION_MODE_
    //Prototypes for defined functions which will replace the equivalent WDF function
    VOID MadInterruptAcquireLock(IN WDFINTERRUPT hInterrupt);
    VOID MadInterruptReleaseLock(IN WDFINTERRUPT hInterrupt);
#endif //_MAD_SIMULATION_MODE_

#ifndef MAD_KERNEL_WONT_CREATE_DMA_ADAPTER //On a 32-bit OS these will just be aliases for the WDF functions because they still work on our non-PCI bus
	                                 //In real mode these will just be aliases for the WDF functions because we have a DMA-capable bus
    #define MadDmaEnablerCreate                           WdfDmaEnablerCreate
    #define MadDmaEnablerSetMaximumScatterGatherElements  WdfDmaEnablerSetMaximumScatterGatherElements
    #define MadDmaTransactionCreate                       WdfDmaTransactionCreate
    #define MadDmaTransactionInitializeUsingRequest       WdfDmaTransactionInitializeUsingRequest
    #define MadDmaTransactionExecute                      WdfDmaTransactionExecute
    #define MadDmaTransactionGetRequest                   WdfDmaTransactionGetRequest
    #define MadDmaTransactionGetBytesTransferred          WdfDmaTransactionGetBytesTransferred
    #define MadDmaTransactionDmaCompleted                 WdfDmaTransactionDmaCompleted
//
#else 	// On a 64-bit OS in simulation_mode these are prototypes for defined functions which will replace the equivalent WDF function 
//
	NTSTATUS   MadDmaEnablerCreate(IN WDFDEVICE Device, IN PWDF_DMA_ENABLER_CONFIG Config, 
		                           IN OPTIONAL PWDF_OBJECT_ATTRIBUTES Attributes,  OUT WDFDMAENABLER* DmaEnablerHandle);
	VOID       MadDmaEnablerSetMaximumScatterGatherElements(IN WDFDMAENABLER DmaEnabler,  IN size_t MaximumFragments);
	NTSTATUS   MadDmaTransactionCreate(WDFDEVICE hDevice, //We are violating the WDF function signature...I hate this!
		                               IN WDFDMAENABLER DmaEnabler, IN OPTIONAL WDF_OBJECT_ATTRIBUTES *Attributes, 
									   OUT WDFDMATRANSACTION *DmaTransaction);
	NTSTATUS   MadDmaTransactionInitializeUsingRequest(IN WDFDMATRANSACTION DmaTransaction, IN WDFREQUEST Request, 
		                                               IN PFN_WDF_PROGRAM_DMA EvtProgramDmaFunction, IN WDF_DMA_DIRECTION DmaDirection);
	NTSTATUS   MadDmaTransactionExecute(IN WDFDMATRANSACTION DmaTransaction, IN OPTIONAL PVOID Context);
	WDFREQUEST MadDmaTransactionGetRequest(WDFDEVICE hDevice, //We are violating the WDF function signature...I hate this too!
		                                   IN WDFDMATRANSACTION DmaTransaction);
	size_t     MadDmaTransactionGetBytesTransferred(WDFDEVICE hDevice, //We are violating the WDF function signature...
		                                            IN WDFDMATRANSACTION DmaTransaction);
	BOOLEAN    MadDmaTransactionDmaCompleted(WDFDEVICE hDevice, //We are violating the WDF function signature...
		                                     IN WDFDMATRANSACTION DmaTransaction, OUT NTSTATUS *Status);
#endif //MAD_KERNEL_WONT_CREATE_DMA_ADAPTER 

//How to reset device register bits set by the device
#ifdef XOR_REGBITS_SET_BY_DEVICE //======================================================
    #ifdef _MAD_SIMULATION_MODE_ //We clear 1 bits by Not:ANDing
    //This requires that the mask is equal to the set of bits to be reset
    inline VOID    MadResetStatusReg(PMADREGS pMadRegs, ULONG StatusMask){StatusMask &= ~StatusMask; WRITE_REGISTER_ULONG(&pMadRegs->Status, StatusMask); return;}
    inline VOID    MadResetIntIdReg(PMADREGS pMadRegs, ULONG IntIdMask) {IntIdMask &= ~IntIdMask; WRITE_REGISTER_ULONG(&pMadRegs->IntID, IntIdMask); return;}
	//
    #else  // ~_MAD_SIMULATION_MODE_: We assume that we clear 1 bits by eXclusive-ORing
    //
    //This requires that the mask is equal to the set of bits to be reset
    inline VOID    MadResetStatusReg(PMADREGS pMadRegs, ULONG StatusMask){WRITE_REGISTER_ULONG(&pMadRegs->Status, StatusMask); return;}
    inline VOID    MadResetIntIdReg(PMADREGS pMadRegs, ULONG IntIdMask) {WRITE_REGISTER_ULONG(&pMadRegs->IntID, IntIdMask); return;}
    #endif //~_MAD_SIMULATION_MODE_
//
#else  //~XOR_REGBITS_SET_BY_DEVICE //====================================================
//
    //Normal write to the register
    inline VOID    MadResetStatusReg(PMADREGS pMadRegs, ULONG StatusReg) {UNREFERENCED_PARAMETER(StatusReg);WRITE_REGISTER_ULONG(&pMadRegs->Status, 0); return;}
    inline VOID    MadResetIntIdReg(PMADREGS pMadRegs, ULONG IntIdReg) {UNREFERENCED_PARAMETER(IntIdReg);WRITE_REGISTER_ULONG(&pMadRegs->IntID, 0); return;}
#endif //~XOR_REGBITS_SET_BY_DEVICE 

inline VOID    MadResetControlReg(PMADREGS pMadRegs, ULONG ControlReg) \
               {UNREFERENCED_PARAMETER(ControlReg); WRITE_REGISTER_ULONG(&pMadRegs->Control, MAD_CONTROL_RESET_STATE); return;}

inline VOID MadCompleteXferReqWithInfo(WDFREQUEST hRequest, NTSTATUS NtStatus, size_t IoCount);

inline WDFREQUEST 
MadDevDetermineRequest(PFDO_DATA pFdoData, PMADREGS pMadRegs, MADDEV_IO_TYPE* pDevReqType, BOOLEAN* pbSrbDone);

PCHAR DbgDevicePowerString(IN WDF_POWER_DEVICE_STATE Type);
} //End extern "C"

inline VOID 
MadDeviceSetPnpPowerCallbacks(WDF_PNPPOWER_EVENT_CALLBACKS* pPnpPowerCallbacks)
{
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(pPnpPowerCallbacks);

    // Register PNP callbacks.
    pPnpPowerCallbacks->EvtDevicePrepareHardware = MadEvtPrepareHardware;
    pPnpPowerCallbacks->EvtDeviceReleaseHardware = MadEvtReleaseHardware;
    //pPnpPowerCallbacks.->EvtDeviceSelfManagedIoInit = MadEvtSelfManagedIoInit;

    // Register Power callbacks.
    pPnpPowerCallbacks->EvtDeviceD0Entry = MadEvtD0Entry;
    pPnpPowerCallbacks->EvtDeviceD0Exit = MadEvtD0Exit;
}

inline VOID
MadDeviceSetPowerPolicyCallbacks(WDF_POWER_POLICY_EVENT_CALLBACKS* pPowerPolicyCallbacks)
{
    WDF_POWER_POLICY_EVENT_CALLBACKS_INIT(pPowerPolicyCallbacks);

    // This group of three callbacks allows this sample driver to manage arming the device
    // for wake from the S0 or Sx state. We don't really differentiate between S0 and Sx state..
    pPowerPolicyCallbacks->EvtDeviceArmWakeFromS0 = MadEvtArmWakeFromS0;
    pPowerPolicyCallbacks->EvtDeviceDisarmWakeFromS0 = MadEvtDisarmWakeFromS0;
    pPowerPolicyCallbacks->EvtDeviceWakeFromS0Triggered = MadEvtWakeFromS0Triggered;
    pPowerPolicyCallbacks->EvtDeviceArmWakeFromSx = MadEvtArmWakeFromSx;
    pPowerPolicyCallbacks->EvtDeviceDisarmWakeFromSx = MadEvtDisarmWakeFromSx;
    pPowerPolicyCallbacks->EvtDeviceWakeFromSxTriggered = MadEvtWakeFromSxTriggered;
}
#endif  // _MADDEVICE_H_

