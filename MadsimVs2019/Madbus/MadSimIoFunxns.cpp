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
/*  Module  NAME : MadSimIoFunxns.cpp                                          */
/*                                                                             */
/*  DESCRIPTION  : Kernel & Storport simulation functions for the              */
/*                simulator-bus driver                                         */
/*                                                                             */
/*******************************************************************************/

#include "MadBus.h"
#include "..\Includes\MadUtilFunxns.h"
#include <ntddscsi.h>
#include <ntdddisk.h>

#ifdef WPP_TRACING
    #include "trace.h"
    #include "MadBusPdo.tmh"
#endif

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

NTSTATUS MadsimBuildOsSgList(IN PMDL pMDL, PSCATTER_GATHER_LIST pSgList,
                             ULONG DataLen, ULONG BufrLen);

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
PMDL             pMDL = NULL;

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

    NtStatus = MadsimBuildOsSgList(pMDL, pSgList, 0,0);

    //Complete the transaction context for XxxDmaTransactionExecute
    pXaxnData->hRequest = hRequest;
    pXaxnData->Direxn   = Direxn;
    pXaxnData->pSgList  = pSgList; 

    return NtStatus;
}

//Build the OS Scatter-Gather List by converting the MDL
NTSTATUS MadsimBuildOsSgList(IN PMDL pMDL, PSCATTER_GATHER_LIST pSgList,
                             ULONG DataLen, ULONG BufrLen)
{
PMDL             pCurrMdlItem;
PMDL             pNextMdlItem;
PVOID            SysAddr;
PHYSICAL_ADDRESS PhysAddr;
ULONG            NumItems = 0;
ULONG            SgDataLen = 0;
ULONG            SglLen = 0;

    if (!(pMDL->MdlFlags & MDL_PAGES_LOCKED))
        {MmProbeAndLockPages(pMDL, KernelMode, IoModifyAccess);}

    for (pCurrMdlItem = pMDL; pCurrMdlItem != NULL; pCurrMdlItem = pNextMdlItem)
        {
        pNextMdlItem = pCurrMdlItem->Next;
        //if (!(pCurrMdlItem->MdlFlags & MDL_PAGES_LOCKED))
        //    {MmProbeAndLockPages(pCurrMdlItem, KernelMode, IoModifyAccess);}

        SysAddr = MmGetMdlVirtualAddress(pCurrMdlItem);
        PhysAddr = MmGetPhysicalAddress(SysAddr);
        pSgList->Elements[NumItems].Address = PhysAddr;
        pSgList->Elements[NumItems].Length = MmGetMdlByteCount(pCurrMdlItem);

        SgDataLen += pSgList->Elements[NumItems].Length;
        SglLen += sizeof(SCATTER_GATHER_ELEMENT);
        NumItems++;
        }
    pSgList->NumberOfElements = NumItems;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadsimBuildOsSgList: NumItems=%d DmaTotal=%d SglLen=%d\n",
                NumItems, SgDataLen, SglLen);

    if (DataLen > 0)
        if (SgDataLen > DataLen)
            return STATUS_INSUFFICIENT_RESOURCES;
    
    if (BufrLen > 0)
        if (SglLen > BufrLen)
            return STATUS_BUFFER_TOO_SMALL;

    return STATUS_SUCCESS;
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
size_t            BytesXferd = (size_t)pMadRegs->DmaChainItem0.DTBC; //Updated upon completion by our hardware

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

//Populate a table of function pointers for KMDF-DMA simulation functions to be passed to the device driver 
void MadSimInitDmaFunxnTable(MADSIM_KMDF_DMA_FUNXNS* pMadSimDmaFunxns)
{
    pMadSimDmaFunxns->pDmaEnablerCreate =
        (PFN_DMA_ENABLER_CREATE)&MadSimDmaEnablerCreate;
    pMadSimDmaFunxns->pDmaSetMaxSgElems =
        (PFN_DMA_ENABLER_SET_MAX_SG_ELEMS)&MadSimDmaEnablerSetMaximumScatterGatherElements;
    pMadSimDmaFunxns->pDmaXaxnCreate =
        (PFN_DMA_TRANSACTION_CREATE)&MadSimDmaTransactionCreate;
    pMadSimDmaFunxns->pDmaXaxnInitFromReq =
        (PFN_DMA_TRANSACTION_INIT_FROM_REQ)&MadSimDmaTransactionInitializeUsingRequest;
    pMadSimDmaFunxns->pDmaXaxnExecute =
        (PFN_DMA_TRANSACTION_EXECUTE)&MadSimDmaTransactionExecute;
    pMadSimDmaFunxns->pDmaXaxnGetRequest =
        (PFN_DMA_TRANSACTION_GET_REQUEST)&MadSimDmaTransactionGetRequest;
    pMadSimDmaFunxns->pDmaXaxnGetBytesXferd =
        (PFN_DMA_TRANSACTION_GET_BYTES_XFERD)&MadSimDmaTransactionGetBytesTransferred;
    pMadSimDmaFunxns->pDmaXaxnCompleted =
        (PFN_DMA_TRANSACTION_DMA_COMPLETED)&MadSimDmaTransactionDmaCompleted;
    //pMadSimDmaFunxns->pDmaXaxnGetDevice     = (PFN_DMA_TRANSACTION_GET_DEVICE)&MadSimDmaTransactionGetDevice;
}

PVOID MadSimMapDeviceBase(_In_ STOR_PHYSICAL_ADDRESS IoAddress,
                          _In_ ULONG NumBytes)
{
    PVOID pVoid = MmMapIoSpaceEx(IoAddress, NumBytes, 
                                 (PAGE_READWRITE | PAGE_NOCACHE));

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadSimMapDeviceBase... x%X:%X --> %p Length=%ld\n",
                IoAddress.HighPart, IoAddress.LowPart, pVoid, NumBytes);
    return pVoid;
}

VOID MadSimUnmapMemory(PVOID VirtAddr, _In_ ULONG NumBytes)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadSimUnmapMemory... %p Length=%ld\n",
                 VirtAddr, NumBytes);

    MmUnmapIoSpace(VirtAddr, NumBytes);
}

STOR_PHYSICAL_ADDRESS MadSimGetPhysicalAddress(PVOID VirtAddr, 
                                               _Out_ ULONG* pLength)
{
    UNREFERENCED_PARAMETER(pLength);
    ASSERT(VirtAddr != NULL);
    LARGE_INTEGER liPhysAddr = MmGetPhysicalAddress(VirtAddr);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadSimGetPhysicalAddress... %p --> x%X:%X\n",
                VirtAddr, liPhysAddr.HighPart, liPhysAddr.LowPart);

    return (STOR_PHYSICAL_ADDRESS)liPhysAddr;
}

BOOLEAN MadDevIssueLoadMedia(PVOID pPrntDev, PVOID pDevObj)
{
    IO_STACK_LOCATION IoStackLoc;

    //*** Setup read-config IRP Parameters
    RtlZeroMemory(&IoStackLoc, sizeof(IO_STACK_LOCATION));
    IoStackLoc.MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IoStackLoc.Parameters.DeviceIoControl.IoControlCode = IOCTL_STORAGE_LOAD_MEDIA;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadSimIssueLoadMedia... Parent=%p MediaDev=%p\n",
                pPrntDev, pDevObj);

    NTSTATUS NtStatus = 
    MadDev_AllocIrpSend2Parent(pPrntDev, pDevObj, &IoStackLoc);

    return (NtStatus == STATUS_SUCCESS);
}

VOID MadSimInitStorPortFunxnTable(PMADSIM_STORPORT_IO_FUNXNS pMadSimSpIoFunxns)
{
    pMadSimSpIoFunxns->pMadsimSpMapDeviceBase = &MadSimMapDeviceBase;
    pMadSimSpIoFunxns->pMadsimSpUnmapMemory = &MadSimUnmapMemory;
    pMadSimSpIoFunxns->pMadsimSpGetPhysicalAddress = &MadSimGetPhysicalAddress;
    pMadSimSpIoFunxns->pMadDevIssueLoadMedia = &MadDevIssueLoadMedia;
    pMadSimSpIoFunxns->pMadsimSpBuildOsSgList = &MadsimBuildOsSgList;
}
