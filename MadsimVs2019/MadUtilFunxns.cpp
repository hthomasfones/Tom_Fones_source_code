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
/*  Exe file ID  : MadBus.sys, MadDevice.sys                                   */
/*                                                                             */
/*  Module  NAME : MadUtilFunxns.cpp                                           */
/*                                                                             */
/*  DESCRIPTION  : Utility functions for the drivers                           */
/*                                                                             */
/*******************************************************************************/


#define BYTE UCHAR

//extern "C" {
#include ".\MadBus\MadBus.h"
#define NTSTRSAFE_LIB

#ifdef WPP_TRACING
    #ifdef MADDEVICE
    #include "MadDevice\trace.h"
    #else
    #include "Madbus\trace.h"
    #endif
	#include "MadUtilFunxns.tmh"
#endif
//}

/************************************************************************//**
 * Mad_MapSysAddr2UsrAddr
 *
 * DESCRIPTION:
 *      Given a system logical address, maps this address into a user mode
 *      process's address space
 *      Assume logical address passed to this routine points to a contiguous
 *      buffer large enough to contain the buffer requested.
 *      This routine must be called at DISPATCH level from the context of the
 *      Application thread.  
 *    
 * PARAMETERS: 
 *     @param[in]  pKrnlAddr    pointer to the buffer in kernel addr. space
 *     @param[in]  Len2Map      length to map
 *     @param[in]  ppPhysAddr   pointer to a pointer to the physical address
 *                              of the buffer  
 *     @param[in]  ppMapdMemUsr pointer to the user mode mapped buffer
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS Mad_MapSysAddr2UsrAddr(IN PVOID pKrnlAddr, IN /*ULONG*/ size_t Len2Map,
                                OUT PHYSICAL_ADDRESS* pPhysAddr, OUT PVOID* ppMapdMemUsr)

{
PHYSICAL_ADDRESS   liPhysAddr;
size_t             ViewSize;
size_t             CommitSize;
UNICODE_STRING     UniStrPhysMem;
OBJECT_ATTRIBUTES  ObjAttrs;
HANDLE             hPhysMemory  = NULL;
PVOID              pPhysMemSexn = NULL;
NTSTATUS           NtStatus;
PHYSICAL_ADDRESS   liPhysAddrBase;
PHYSICAL_ADDRESS   liPhysAddrEnd;
PHYSICAL_ADDRESS   liViewBase;
PHYSICAL_ADDRESS   MapdLen;
ULONG_PTR          VirtAddr;
 
    liPhysAddr    = MmGetPhysicalAddress(pKrnlAddr);
    *pPhysAddr    = liPhysAddr;
	*ppMapdMemUsr = NULL;

// Get a pointer to physical memory...
//
// - Create the name
// - Initialize the data to find the object
// - Open a handle to the object and check the status
// - Get a pointer to the object
// - Free the handle
//
    RtlInitUnicodeString(&UniStrPhysMem,  L"\\Device\\PhysicalMemory");
    InitializeObjectAttributes(&ObjAttrs, &UniStrPhysMem, OBJ_CASE_INSENSITIVE, 
			                   (HANDLE)NULL,(PSECURITY_DESCRIPTOR)NULL);

    NtStatus = ZwOpenSection(&hPhysMemory, SECTION_ALL_ACCESS, &ObjAttrs);
    if (!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "Mad_MapSysAddr2UsrAddr:ZwOpenSection returned...x%X\n", NtStatus);
        return NtStatus;
        }

    NtStatus = ObReferenceObjectByHandle(hPhysMemory, SECTION_ALL_ACCESS,
                                         (POBJECT_TYPE)NULL, KernelMode,
                                         &pPhysMemSexn, (POBJECT_HANDLE_INFORMATION)NULL);
    if (!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "Mad_MapSysAddr2UsrAddr:ObReferenceObjectByHandle failed...x%X\n", NtStatus);
        goto close_handle;
        }

// Initialize the physical addresses that will be translated
//
    liPhysAddrBase = liPhysAddr;
    liPhysAddrEnd  = liPhysAddr; 
    liPhysAddrEnd.LowPart += (ULONG)Len2Map;

// Calculate the length of the memory to be mapped
//
    //MapdLen = RtlLargeIntegerSubtract(liPhysAddrEnd, liPhysAddrBase);
    MapdLen.LowPart = liPhysAddrEnd.LowPart - liPhysAddrBase.LowPart;

// If the mappedlength is zero, somthing very weird happened in the HAL
// since the Length was checked against zero.
//
    if (MapdLen.LowPart == 0)
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "Mad_MapSysAddr2UsrAddr...MapdLen.LowPart == 0\n");
        NtStatus = STATUS_UNSUCCESSFUL;
        goto close_handle;
        }

    CommitSize = (size_t)0; //MapdLen.LowPart;
	ViewSize   = (size_t)Len2Map;

// initialize view base that will receive the physical mapped
// address after the MapViewOfSection call.
//
    liViewBase = liPhysAddrBase;

// Let ZwMapViewOfSection pick an address
//
    VirtAddr = NULL;

// Map the section
//
    NtStatus = ZwMapViewOfSection(hPhysMemory,
		                          /*(HANDLE)-1*/ZwCurrentProcess(), //The parent process for this thread  
                                  (PVOID *)&VirtAddr, 
                                  0L, CommitSize, &liViewBase,
                                  (PSIZE_T)&ViewSize, ViewUnmap,
                                  0, (PAGE_READWRITE|PAGE_NOCACHE));
    if (!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "Mad_MapSysAddr2UsrAddr:ZwMapViewOfSection failed...NTSTATUS=x%X liVuBasex%X:%X, MapdLen=%ld\n",
					NtStatus, liPhysAddrBase.HighPart, liPhysAddrBase.LowPart, (ULONG)Len2Map);
        goto close_handle;
        }

// Mapping the section above rounded the physical address down to the
// nearest 64 K boundary. Now return a virtual address that sits where
// we want by adding in the offset from the beginning of the section.
//
    *ppMapdMemUsr =
	(PVOID)(VirtAddr + (ULONG)liPhysAddrBase.LowPart - (ULONG)liViewBase.LowPart);

    //*ppMapdMemUsr = (PVOID)VirtAddr;          

close_handle:
    ZwClose(hPhysMemory);

    return NtStatus;
}

/************************************************************************//**
 * Mad_UnmapUsrAddr 
 *
 * DESCRIPTION:
 *    This function unmaps a user mode virtual address from a process.
 *    
 * PARAMETERS: 
 *     @param[in]  pMapdMemUsr user mode pointer to its mapped buffer
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS Mad_UnmapUsrAddr(IN PVOID pMapdMemUsr)

{
    NTSTATUS NtStatus = ZwUnmapViewOfSection(ZwCurrentProcess(), pMapdMemUsr);
    if (!NT_SUCCESS(NtStatus))
	    {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		            "Mad_UnmapUsrAddr:ZwUnmapViewOfSection failed...NTSTATUS = x%X, UserAddr=%p\n",
                    NtStatus, pMapdMemUsr);
	    }

    return NtStatus;
}

/************************************************************************//**
 * MadDevAnalyzeReqType 
 *
 * DESCRIPTION:
 *    This function determines what type of I/O is completed based on
 *    device registers. In our prototype this will indicate that one request
 *    has completed - as if we have 1 request active for any I-O type.
 *    In reality we have only one I-O request active of any type.
 *    
 * PARAMETERS: 
 *     @param[in]  hQueue      handle to our I/O queue for this device.
 *     @param[in]  hRequest    handle to this I/O request
 *     @param[in]  hDevice     handle to our device 
 *     @param[in]  pMadRegs    pointer to the device registers
 *     
 * RETURNS:
 *    @return      DevReqType  enumerated type of I/O determined
 * 
 ***************************************************************************/
MADDEV_IO_TYPE MadDevAnalyzeReqType(PMADREGS pMadRegs)

{
MADDEV_IO_TYPE  DevReqType = eNoIO; 
ULONG           IntIdReg   = pMadRegs->IntID;
ULONG           ControlReg = pMadRegs->Control;
BOOLEAN         bInput     = ((IntIdReg & MAD_INT_INPUT_MASK) != 0);
BOOLEAN         bOutput    = ((IntIdReg & MAD_INT_OUTPUT_MASK) != 0);

    //BRKPNT;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
    "MBPIT:MadDevAnalyzeReqType... MesgID=x%X IntEnable=x%X Control=x%X Status=x%X DmaCntl=x%X\n",
                pMadRegs->MesgID, pMadRegs->IntEnable, pMadRegs->Control, 
                pMadRegs->Status, pMadRegs->DmaChainItem0.DmaCntl);

    IntIdReg &= ~MAD_INT_STATUS_ALERT_BIT;
    if (bInput)
        {
        switch (IntIdReg)
            {
            case MAD_INT_BUFRD_INPUT_BIT:
                if ((ControlReg & MAD_CONTROL_CACHE_XFER_BIT) != 0)
                    DevReqType = eCachedRead;
                else
                    DevReqType = eBufrdRead;
                break;

            case MAD_INT_DMA_INPUT_BIT:
                if ((ControlReg & MAD_CONTROL_CHAINED_DMA_BIT) != 0)
                    DevReqType = eSgDmaRead;
                else
                    DevReqType = eDmaRead;
                break;

            case MAD_INT_ALIGN_INPUT_BIT:
                DevReqType = eReadAlign;
                break;

            default:
                TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				"MBPIT:MadDevAnalyzeReqType...multiple input id bits set: IntIdReg=x%X\n", 
                            IntIdReg);
                DevReqType = eMltplIO;
            } // end switch
        }

    if (bOutput)
        {
        switch (IntIdReg)
            {
            case MAD_INT_BUFRD_OUTPUT_BIT:
                if ((ControlReg & MAD_CONTROL_CACHE_XFER_BIT) != 0)
                    DevReqType = eCachedWrite;
                else
                    DevReqType = eBufrdWrite;
                break;

            case MAD_INT_DMA_OUTPUT_BIT:
                if ((ControlReg & MAD_CONTROL_CHAINED_DMA_BIT) != 0)
                    DevReqType = eSgDmaWrite;
                else
                    DevReqType = eDmaWrite;
                break;

            case MAD_INT_ALIGN_OUTPUT_BIT:
                DevReqType = eWriteAlign;
                break;

            default:
                TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				"MBPIT:MadDevAnalyzeReqType...multiple output id bits set: IntIdReg=x%X\n", 
                            IntIdReg);
                DevReqType = eMltplIO;
            } // end switch
        }

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MBPIT:MadDevAnalyzeReqType...DevReqType=%d\n", DevReqType);

	return DevReqType;
}


//* Generate an Error Log Entry from passed paramters
//*
void MadWriteEventLogMesg(PDRIVER_OBJECT pDriverObj, NTSTATUS NtStatMC, 
                          ULONG NumStrings, ULONG LenParms, PWSTR pwsInfoStr)

{
static LARGE_INTEGER liZERO = {0, 0};
NTSTATUS NtStatFinl = STATUS_SUCCESS;
PIO_ERROR_LOG_PACKET pErrLogObj;
PWSTR pwsStrParms;

    if (pDriverObj == NULL)
        return;

    pErrLogObj = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(pDriverObj,
                                                               ERROR_LOG_MAXIMUM_SIZE);
    if (pErrLogObj == NULL)
        return;

	//A special case use of status as 4-byte dump data
	//
	if (NumStrings == 1)
		if (LenParms == sizeof(NTSTATUS))
			if (pwsInfoStr != NULL)
			    {
		        NtStatFinl = *(NTSTATUS *)pwsInfoStr;
				RtlMoveMemory(&pErrLogObj->DumpData, pwsInfoStr, LenParms);
     			}

    pErrLogObj->MajorFunctionCode = 0x00; 
    pErrLogObj->RetryCount        = 0; 
	pErrLogObj->DumpDataSize      = (USHORT)LenParms; 
    pErrLogObj->NumberOfStrings   = (USHORT)NumStrings;
    pErrLogObj->StringOffset      = sizeof(IO_ERROR_LOG_PACKET); //+ ulLenParms; 
    pErrLogObj->EventCategory     = 0;
    pErrLogObj->ErrorCode         = NtStatMC; 
    pErrLogObj->UniqueErrorValue  = 0; 
	pErrLogObj->FinalStatus       = NtStatFinl;
    pErrLogObj->SequenceNumber    = 0L;
    pErrLogObj->IoControlCode     = 0L;
    pErrLogObj->DeviceOffset      = liZERO;
    if (LenParms > 0)
        {
        pwsStrParms = (PWSTR)((ULONG_PTR)pErrLogObj + pErrLogObj->StringOffset);
        RtlMoveMemory(pwsStrParms, pwsInfoStr, LenParms);
        }

	//And finally
    IoWriteErrorLogEntry(pErrLogObj); 

    return;
}

/************************************************************************//**
 * MadDev_AllocIrpSend2Parent
 *
 * DESCRIPTION:
 *    This function builds and sends an irp to the lower device object.
 *    It waits for the IRP to complete
 *
 * PARAMETERS:
 *     @param[in]  pPrntDev    pointer to the parent device object.
 *     @param[in]  pDevObj     pointer to our device object
 *     @param[in]  pIoStackLoc pointer to the current stack location
 *
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 *
 ***************************************************************************/
NTSTATUS MadDev_AllocIrpSend2Parent(PVOID  pPrntDev,
                                    PVOID  pDevObj,
                                    PVOID  pvIoStackLoc)
{
    PDEVICE_OBJECT pParentDev = (PDEVICE_OBJECT)pPrntDev;
    PIO_STACK_LOCATION pIoStackLoc = (PIO_STACK_LOCATION)pvIoStackLoc;
    PDEVICE_OBJECT pDeviceObj = (PDEVICE_OBJECT)pDevObj;
    CCHAR              StackSize = pDeviceObj->StackSize;

    NTSTATUS           NtStatus;
    KEVENT             Kevent;
    PIRP               pIRP;
    PIO_STACK_LOCATION pIrpIoStackLoc;

    //TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
    //"MadDevice_AllocIrpSend2Parent pPrntDev=%p, pDevObj=%p\n", pPrntDev, pDevObj)); 

//*** Build the IRP and pass it down to the parent (PDO)
    pIRP = IoAllocateIrp(StackSize, FALSE);
    if (pIRP == NULL)  // LOG it & return false
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                    "MadDev_AllocIrpSend2Parent...allocate IRP failed!\n");
        return STATUS_NO_MEMORY;
        }

    KeInitializeEvent(&Kevent, NotificationEvent, FALSE);
#pragma warning(suppress: 28023)
    IoSetCompletionRoutine(pIRP, (PIO_COMPLETION_ROUTINE)MadDev_CnfgCmpltnRtn,
                           &Kevent, TRUE, TRUE, TRUE);

    //*** Setup IRP Parameters
    pIRP->IoStatus.Status = STATUS_NOT_SUPPORTED; //Throw Driver-verifier a cookie
    pIrpIoStackLoc = IoGetNextIrpStackLocation(pIRP);
    pIrpIoStackLoc->MajorFunction = pIoStackLoc->MajorFunction;
    pIrpIoStackLoc->MinorFunction = pIoStackLoc->MinorFunction;
    RtlCopyMemory(&pIrpIoStackLoc->Parameters,
                  &pIoStackLoc->Parameters, sizeof(pIoStackLoc->Parameters));

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                "MadDev_AllocIrpSend2Parent... Sending PnpIrp Major:Minor=x%X:%X\n",
                pIrpIoStackLoc->MajorFunction, pIrpIoStackLoc->MinorFunction);

    //*** Send the IRP and wait for results
    NtStatus = IoCallDriver(pParentDev, pIRP);
    if (NtStatus == STATUS_PENDING)
        {
        KeWaitForSingleObject(&Kevent, Executive, KernelMode, FALSE, NULL);
        NtStatus = pIRP->IoStatus.Status;
        }
    IoFreeIrp(pIRP);

    if (!NT_SUCCESS(NtStatus)) // LOG it & return 
    { //Brackets around null stmt in the case of Free_build & No WPP_TRACING(KdPrint)
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
                     "MadDev_AllocIrpSend2Parent...SendIrpSync failed! ntstatus=x%X\n",
                     NtStatus);
    }

    return NtStatus;
}

/************************************************************************//**
 * MadDev_CnfgCmpltnRtn
 *
 * DESCRIPTION:
 *    This function is the irp completion routine for the read configuration
 *    irp
 *
 * PARAMETERS:
 *     @param[in]  pDevObj     pointer to our device object
 *     @param[in]  pIRP        pointer to the completing irp
 *     @param[in]  pContext    pointer to the event object to be waited on
 *
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 *
 ***************************************************************************/
NTSTATUS MadDev_CnfgCmpltnRtn(PVOID /*PDEVICE_OBJECT*/ pDevObj, 
                              PVOID /*PIRP*/ pIRP, PVOID pContext)

{
    UNREFERENCED_PARAMETER(pDevObj);
    UNREFERENCED_PARAMETER(pIRP);

    KeSetEvent((PKEVENT)pContext, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED; // Keep this IRP
}
