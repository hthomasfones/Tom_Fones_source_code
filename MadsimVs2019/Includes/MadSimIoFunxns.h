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

//Define an interrupt-related parm area for the simulation interrupt thread 
//won't compile w/out WDK/Kmdf
#ifndef _MADSIM_IO_FUNXNS_H_ 
#define _MADSIM_IO_FUNXNS_H_ 

#ifndef STOR_PHYSICAL_ADDRESS
#define STOR_PHYSICAL_ADDRESS PHYSICAL_ADDRESS
typedef ULONG STOR_SPINLOCK;
#define DpcLock  1
#define StartIoLock 2
#define InterruptLock 3
#endif

typedef VOID (BUS_SET_POWER_UP)(ULONG SerialNo);
typedef BUS_SET_POWER_UP* PFN_BUS_SET_POWER_UP;
//
typedef VOID (BUS_SET_POWER_DOWN)(ULONG SerialNo);
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
    } MAD_SIMULATION_INT_PARMS, *PMAD_SIMULATION_INT_PARMS;

//MAD_KERNEL_WONT_CREATE_DMA_ADAPTER
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
	//PFN_DMA_TRANSACTION_GET_DEVICE                   pDmaXaxnGetDevice;
    } MADSIM_KMDF_DMA_FUNXNS, *PMADSIM_KMDF_DMA_FUNXNS;
#endif //MAD_KERNEL_WONT_CREATE_DMA_ADAPTER 
void MadSimInitDmaFunxnTable(MADSIM_KMDF_DMA_FUNXNS* pMadSimDmaFunxns);

typedef struct _MADSIM_STORPORT_IO_FUNXNS
{
    PVOID (*pStorPortGetDeviceBase)(_In_ PVOID pDevXtensn,
                                    _In_ INTERFACE_TYPE BusType,
                                    _In_ ULONG SystemIoBusNumber,
                                    _In_ STOR_PHYSICAL_ADDRESS IoAddress,
                                    _In_ ULONG NumberOfBytes,
                                    _In_ BOOLEAN InIoSpace);
} MADSIM_STORPORT_IO_FUNXNS, *PMADSIM_STORPORT_IO_FUNXNS;

VOID MadSimInitStorportFunxnTable(PMADSIM_STORPORT_IO_FUNXNS pMadSimStorPortIoFunxns);

#endif //_MADSIM_IO_FUNXNS_H_