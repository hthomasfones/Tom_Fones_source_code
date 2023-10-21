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
/*  Exe file ID  : MadDevice.sys                                               */
/*                                                                             */
/*  Module  NAME : MadDevIoFunxns.cpp                                          */
/*                                                                             */
/*  DESCRIPTION  : Definitions of I-O functions & Ioctls for MadDevice.sys     */
/*                 Derived from WDK-Toaster\func\func.c                        */
/*                                                                             */
/*******************************************************************************/

#include "MadDevice.h"
#ifdef WPP_TRACING
    #include "trace.h"
    #include "MadDevIoFunxns.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, MadEvtIoBufrdRead)
#pragma alloc_text (PAGE, MadEvtIoBufrdWrite)
#pragma alloc_text (PAGE, MadEvtIoDmaRead)
#pragma alloc_text (PAGE, MadEvtIoDmaWrite)
#pragma alloc_text (PAGE, MadEvtIoDeviceControl)
#endif

extern ULONG DebugLevel; // = 3; //unused
extern LONG                     gPowrChekTimrMillisecs; 
extern UNICODE_STRING           gRegistryPath;
extern PDRIVER_OBJECT           gpDriverObj;
extern WDFDRIVER                ghDriver;

/************************************************************************//**
 * MadEvtISR
 *
 * DESCRIPTION:
 *    This is the Interrupt-Service-Routine invoked by driver framework.
 *    
 * PARAMETERS: 
 *     @param[in]  hInterrupt  handle to the device interrupt object
 *     @param[in]  MesgID      MSI message id value.
 *                             Not relevant for a legacy interrupt
 *     
 * RETURNS:
 *    @return      BOOLEAN     True or false. Indicating its out INT or not
 * 
 ***************************************************************************/
BOOLEAN MadEvtISR(IN WDFINTERRUPT hInterrupt, IN ULONG MesgID)

{
WDFDEVICE hDevice  = WdfInterruptGetDevice(hInterrupt);
PFDO_DATA pFdoData = MadDeviceFdoGetData(hDevice);
PMADREGS  pMadRegs = pFdoData->pMadRegs;
PISR_DPC_WI_CONTEXT  pIsrDpcData = MadGetIsrDpcWiContext(hInterrupt);
BOOLEAN   bRC; 
KAFFINITY DevAffin = 
          (KAFFINITY)((ULONG_PTR)0x01 << (ULONG_PTR)KeGetCurrentProcessorNumber());

    UNREFERENCED_PARAMETER(DevAffin);

    //Verify these interrupt parameters when they come from the simulator
    SIMULATION_ASSERT(hInterrupt == pFdoData->hInterrupt);
    SIMULATION_ASSERT(KeGetCurrentIrql() == pFdoData->Irql);
    SIMULATION_ASSERT((DevAffin & pFdoData->IntAffinity) != 0);
    SIMULATION_ASSERT(pMadRegs->MesgID == MesgID);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtISR...SerialNo=%d device regs:MesgID=%d Control=x%X Status=x%X IntEnable=x%X IntID=x%X\n", 
                pFdoData->SerialNo, MesgID, pMadRegs->Control,
                pMadRegs->Status, pMadRegs->IntEnable,  pMadRegs->IntID);

    if ((MesgID < 1) || MesgID > 8)
	    { //Brackets around null stmt in the case of Free_build & No WPP_TRACING(KdPrint)
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		            "MadEvtISR...invalid MesgID detected: SerialNo=%d, MesgID=x%X\n", 
                    pFdoData->SerialNo, MesgID);
	    }

	//Set up the INT/DPC/WI context for the DPC & work item(s)
	//
	pIsrDpcData->pFdoData = pFdoData;

    //Take a snapshot of the registers for the DPC function
    READ_REGISTER_BUFFER_UCHAR((PUCHAR)pMadRegs, 
                               (PUCHAR)&pIsrDpcData->MadRegs, sizeof(MADREGS));
	//
    bRC = MadDevWdmISR((PKINTERRUPT)NULL, (PVOID)pFdoData);
    pMadRegs->MesgID = 0;
    if (!bRC) 
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, "MadEvtISR...extraneous interrupt!\n");
        return FALSE; //Not our INT
        }

    bRC = WdfInterruptQueueDpcForIsr(hInterrupt);
    ASSERT(bRC); //That we Q'd a new DPC & no previous DPC is Q'd 

    return TRUE;
}


/************************************************************************//**
 * MadDevWdmISR
 *
 * DESCRIPTION:
 *    This function is the ISR helper function.
 *    It's prototype/signature is the same as a WDM ISR  but we don't use
 *    it that way. This can serve as an example of a helper function which
 *    is generic accross multiple OS platforms.
 *    
 * PARAMETERS: 
 *     @param[in]  pKINT       pointer to the IoConnectISR interrupt object.
 *                             (Not used) 
 *     @param[in]  Context     pointer to the framework device extension
 *     
 * RETURNS:
 *    @return      BOOLEAN     True or false. Indicating its out INT or not
 * 
 ***************************************************************************/
BOOLEAN MadDevWdmISR(PKINTERRUPT pKINT, PVOID Context)

{
static char cdstr[] = "(chained)\0";     
static char cxstr[] = "(cache xfer)\0"; 
PFDO_DATA pFdoData   = (PFDO_DATA)Context;
ULONG     SerialNo   = pFdoData->SerialNo;
PMADREGS  pMadRegs   = pFdoData->pMadRegs;
ULONG     IntID      = pMadRegs->IntID;
ULONG     ControlReg = pMadRegs->Control;
char      XtraText[20] = "";

    UNREFERENCED_PARAMETER(pKINT);
	UNREFERENCED_PARAMETER(SerialNo);

    //Weed out an invalid interrupt
    if (IntID == 0)
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadDevWdmISR...no IntID bit set! Invalid/not our interrupt. SerialNo=%d\n", SerialNo);
        return FALSE;
        }

    // 1st clear any unexpected interrupts
    //
    if ((IntID & MAD_INT_ALL_INVALID_MASK) != 0)
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadDevWdmISR unexpected IntID bit(s) set! No action defined ... IntID=x%X, SerialNo=%d,\n",
			        IntID, SerialNo);
 
		pMadRegs->IntID &= MAD_INT_ALL_VALID_MASK;
		pFdoData->MadDevWmiData.InfoClassData.ErrorCount++; //Count this as an error
        }

    //Process the defined interrupt conditions
    //
    if (IntID & MAD_INT_STATUS_ALERT_BIT) 
	    { //Brackets around null stmt in the case of Free_build & No WPP_TRACING(KdPrint)
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadDevWdmISR...status alert interrupt detected! SerialNo=%d\n", SerialNo);
	    }

    if (IntID & MAD_INT_ALIGN_OUTPUT_BIT) 
	    { //Brackets around null stmt in the case of Free_build & No WPP_TRACING(KdPrint)
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadDevWdmISR...align write cache interrupt detected, SerialNo=%d\n", SerialNo);
	    }

    if (IntID & MAD_INT_DMA_OUTPUT_BIT)
        {
		if (ControlReg & MAD_CONTROL_CHAINED_DMA_BIT)
			RtlCopyMemory(XtraText, cdstr, 10); //Works like strcpy

		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadDevWdmISR...%s dma output interrupt detected, SerialNo=%d\n",
					XtraText, SerialNo);
        } 

    if (IntID & MAD_INT_BUFRD_OUTPUT_BIT)
        {
		if (ControlReg & MAD_CONTROL_CACHE_XFER_BIT)
			RtlCopyMemory(XtraText, cxstr, 13); //Works like strcpy

        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadDevWdmISR...%s buffered output interrupt detected, SerialNo=%d\n",
					XtraText, SerialNo);
        } 

    if (IntID & MAD_INT_ALIGN_INPUT_BIT)
	     { //Brackets around null stmt in the case of Free_build & No WPP_TRACING(KdPrint)
         TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadDevWdmISR...align read cache interrupt detected, SerialNo=%d\n",
					SerialNo);
	     }

    if (IntID & MAD_INT_DMA_INPUT_BIT)
        {
		if (ControlReg & MAD_CONTROL_CHAINED_DMA_BIT)
			RtlCopyMemory(XtraText, cdstr, 10); //Works like memcpy

        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadDevWdmISR...%s dma input interrupt detected, SerialNo=%d\n",
					XtraText, SerialNo);
        } 

    if (IntID & MAD_INT_BUFRD_INPUT_BIT)
        {
		if (ControlReg & MAD_CONTROL_CACHE_XFER_BIT)
			RtlCopyMemory(XtraText, cxstr, 13); //Works like memcpy

        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadDevWdmISR...%s buffered input interrupt detected, SerialNo=%d\n",
					XtraText, SerialNo);
        } 

    //Disable all INTs until we have processed this one
    WRITE_REGISTER_ULONG(&pMadRegs->IntEnable, MAD_ALL_INTS_DISABLED);

    //Reset device regs
    MadResetStatusReg(pMadRegs,  pMadRegs->Status);
    MadResetIntIdReg(pMadRegs,   pMadRegs->IntID);
    MadResetControlReg(pMadRegs, pMadRegs->Control);

    return TRUE;
}


/************************************************************************//**
 * MadEvtDPC
 *
 * DESCRIPTION:
 *    This function is the DPC dispatched after being enqueued by the ISR.
 *    
 * PARAMETERS: 
 *     @param[in]  hInterrupt  handle to the device interrupt object
 *     @param[in]  hObjAssoc   handle to the parent devicee I/O request
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
VOID MadEvtDPC(IN WDFINTERRUPT hInterrupt, IN WDFOBJECT hObjAssoc)

{
static ULONG64  Zero64 = 0;
WDFDEVICE      hDevice     = (WDFDEVICE)hObjAssoc;
PFDO_DATA      pFdoData    = MadDeviceFdoGetData(hDevice);
PISR_DPC_WI_CONTEXT   pIsrDpcData = MadGetIsrDpcWiContext(hInterrupt);  //Retrieve the ISR's snapshot of the device regs from the interrupt-dpc context
PMADREGS              pMadRegs    = &pIsrDpcData->MadRegs;
ULONG                 IntID       = pMadRegs->IntID;;
ULONG                 IntIdReg    = IntID;
//
NTSTATUS              NtStatus = STATUS_SUCCESS;
WDFREQUEST            hRequest = NULL;
MADDEV_IO_TYPE        DevReqType = eNoIO;
BOOLEAN               bFatalErr = FALSE;
BOOLEAN               bSectorIO = FALSE;
BOOLEAN               bIoComplete = FALSE;
ULONG_PTR             IoCount = 0; 
PISR_DPC_WI_CONTEXT   pDpcWiData;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtDPC...SerialNo=%d\n", pFdoData->SerialNo);

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL); //Confirm that this is a proper DPC

	hRequest = MadDevDetermineRequest(pFdoData, pMadRegs, &DevReqType, &bIoComplete); 

	if (MAD_DEVICE_INDICATES_ERROR) //We have an error condition
        {
        //Assign an NtStatus value for whatever errror indication from the device
		//Most of these status values are somewhat arbitrary
		//
        ULONG StatusReg = pMadRegs->Status & ~MAD_STATUS_RW_COUNT_MASK; //Mask out the i/o count bits before examining the error status bits
		switch (StatusReg)
		    {
		    case MAD_STATUS_NO_ERROR_MASK:
                NtStatus = STATUS_DEVICE_NOT_READY; //Arbitrary...hoping the condition is temporary
			    break;

		    case MAD_STATUS_GENERAL_ERR_BIT:
		    	NtStatus = STATUS_IO_DEVICE_ERROR;
		    	break;

		    case MAD_STATUS_OVER_UNDER_ERR_BIT:
				bSectorIO = (DevReqType >= eReadAlign);
				if (bSectorIO)
					NtStatus = STATUS_NONEXISTENT_SECTOR;
				else
				    {
                    if (IntIdReg & MAD_INT_INPUT_MASK) //It must be a read error
                        NtStatus = STATUS_NO_DATA_DETECTED; //ntstatus.h: ...the end of the data written is reached.
			        else
                        NtStatus = STATUS_DATA_OVERRUN; 
		            }
		       break;

		    case MAD_STATUS_DEVICE_BUSY_BIT:
		    	NtStatus = STATUS_DEVICE_BUSY;
			    break;

			case MAD_STATUS_INVALID_IO_BIT:
		    	NtStatus = STATUS_INVALID_PARAMETER; //Software error
			    break;

            #ifdef _MAD_SIMULATION_MODE_ //This should only occur when the simulator can't get resources from the OS
			case MAD_STATUS_BUS_ERROR_BIT:
			//case STATUS_RESOURCE_ERROR_BIT:
				NtStatus = STATUS_HARDWARE_MEMORY_ERROR;
			    break;
            #endif //_MAD_SIMULATION_MODE_

			case MAD_STATUS_TIMEOUT_ERROR_BIT:
		    	NtStatus = STATUS_TIMEOUT; 
			    break;

			case MAD_STATUS_DEVICE_FAILURE_BIT:
		    	NtStatus = STATUS_ADAPTER_HARDWARE_ERROR; //Maybe needs a firmware reload
			    bFatalErr = TRUE;
			    break;

		    case MAD_STATUS_DEAD_DEVICE_MASK:
				ASSERT(pMadRegs->Status == REGISTER_MASK_ALL_BITS_HIGH);//Confirm that the device is all "Bits-Up"
		    	ASSERT(IntID == REGISTER_MASK_ALL_BITS_HIGH);           //same as above
                NtStatus = STATUS_DEVICE_POWER_FAILURE;
                bFatalErr = TRUE;

			    //We can't trust the determination of reqtype & request handle so we reset them to unknown
			    //before we enqueue the device error work-item
			    DevReqType = eNoIO;
			    hRequest   = NULL;
 			    break;	

		    default:
				GENERIC_SWITCH_DEFAULTCASE_ASSERT; //Don't break
                NtStatus = STATUS_INVALID_DEVICE_STATE; //arbitrary assigned value when we don't recognize the device status
		    } //end switch	

		if (bFatalErr) //Fatal device error needs work-item completion processsing
		    {
			ASSERT(pFdoData->hDevErrorWI != NULL);
			pDpcWiData = MadGetIsrDpcWiContext(pFdoData->hDevErrorWI);
			MAD_SET_CONTEXT_DATA_FOR_WORKITEM;
			WdfWorkItemEnqueue(pFdoData->hDevErrorWI);

			goto FinishDPC; //Near the end below
            }

        if (hRequest == NULL) // The device hasn't indicated any pending i/o 
            {
            TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				        "MadEvtDPC...device error indicated by Status; no i/o indicated by IntID *!*\n");
            TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				        "MadEvtDPC...SerialNo=%d, StatusReg=x%X, assigned NtStatus=x%X\n",
                        pFdoData->SerialNo, StatusReg, NtStatus);

            pFdoData->DevStatus   = StatusReg;
            pFdoData->DevNtStatus = NtStatus;
            MadCompleteAnyPendingIOs(pFdoData, NtStatus);
            }
		else //Report the error & complete the indicated pending IO
            {
            TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				        "MadEvtDPC I/O error completion...hRequest=%p, SerialNo=%d, StatusReg=x%X, assigned NtStatus=x%X\n",
                        hRequest, pFdoData->SerialNo, StatusReg, NtStatus);

			if (DevReqType < eDmaRead) //Buffered I/O
			    {MAD_INIT_AND_ENQUEUE_BUFRD_WORKITEM(IoCount);}
			else  
			    {MAD_INIT_AND_ENQUEUE_DMA_WORKITEM;}
            } 

		pFdoData->MadDevWmiData.InfoClassData.ErrorCount++; 
	    }
    else // Status-Alert bit is clear - normal i/o completion
        {
        switch (DevReqType)
            {
            case eBufrdRead: 
				//Calculate the i-o count
                IoCount = (pMadRegs->Status & MAD_STATUS_READ_COUNT_MASK); //Isolate N bits as a number
                IoCount = (IoCount >> MAD_STATUS_READ_COUNT_SHIFT);        //Shift to low-order bits
                IoCount++; //0..N-1 --> 1..N
                IoCount = (IoCount * MAD_BYTE_COUNT_MULT); //Apply the multiplier
				
				MAD_INIT_AND_ENQUEUE_BUFRD_WORKITEM(IoCount);
                break;

            case eBufrdWrite:
				//Calculate the i-o count
                IoCount = (pMadRegs->Status & MAD_STATUS_WRITE_MASK); //Isolate N bits as a number
                IoCount = (IoCount >> MAD_STATUS_WRITE_SHIFT);        //Shift to low-order bits
                IoCount++; //0..N-1 --> 1..N
                IoCount = (IoCount * MAD_BYTE_COUNT_MULT); //Apply the multiplier

				MAD_INIT_AND_ENQUEUE_BUFRD_WORKITEM(IoCount);
                break;

            case eCachedRead: 
				MAD_INIT_AND_ENQUEUE_BUFRD_WORKITEM(MAD_CACHE_SIZE_BYTES);
                break;

            case eCachedWrite: 
				MAD_INIT_AND_ENQUEUE_BUFRD_WORKITEM(MAD_CACHE_SIZE_BYTES);
                break;

			case eReadAlign: 
                WdfRequestComplete(hRequest, NtStatus);
                pFdoData->hPendingReqs[eReadAlign] = NULL;
                break;

            case eWriteAlign: 
                WdfRequestComplete(hRequest, NtStatus);
                pFdoData->hPendingReqs[eWriteAlign] = NULL;
                break;

			case eDmaRead:
			case eDmaWrite:
			case eSgDmaRead:
            case eSgDmaWrite:
				MAD_INIT_AND_ENQUEUE_DMA_WORKITEM;
                break;

			default:
				GENERIC_SWITCH_DEFAULTCASE_ASSERT;
                TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
					        "MadEvtDPC...undefined i/o completion returned from device - StatusReg=x%X, IntIdReg=x%X !\n",
                            pMadRegs->Status, pMadRegs->IntID);
            } // end switch
	    }

FinishDPC:;
	//Re-enable all ints... - 
    //
    pMadRegs = pFdoData->pMadRegs; //Point to the real device registers - not the ISR copy
	MadInterruptAcquireLock(pFdoData->hInterrupt);
	WRITE_REGISTER_ULONG(&pMadRegs->IntEnable, MAD_ALL_INTS_ENABLED_BITS);
	WRITE_REGISTER_BUFFER_UCHAR((PUCHAR)&pFdoData->pMadRegs->DmaChainItem0.HostAddr,
		                        (PUCHAR)&Zero64, sizeof(ULONG64));
	MadInterruptReleaseLock(pFdoData->hInterrupt);

    return;
}


inline WDFREQUEST 
MadDevDetermineRequest(PFDO_DATA pFdoData, PMADREGS pMadRegs, MADDEV_IO_TYPE* pDevReqType, BOOLEAN* pbSrbDone)

{
	WDFREQUEST hRequest;

    *pDevReqType = MadDevAnalyzeReqType(pMadRegs);
	*pbSrbDone = TRUE; //Always in the prototype

	if ((*pDevReqType == eNoIO) || (*pDevReqType == eInvalidIO))
         hRequest = (WDFREQUEST)NULL;
	else
	    { 
		hRequest = pFdoData->hPendingReqs[*pDevReqType];
		ASSERT(hRequest != NULL);
	    }

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDevDetermineRequest...SerialNo=%d, hRequest=%p, DevReqType=%d\n",
		        pFdoData->SerialNo, hRequest, *pDevReqType);

	return hRequest;
}

/************************************************************************//**
* MadEvtBufrdIoWorkItem
*
* DESCRIPTION:
*    This function is the Buffered-io processing work-item enqueued in the
*    DPC function. We complete the parent request.
*
* PARAMETERS:
*     @param[in]  hBufrdWI      handle to the buffered io work item.
*
* RETURNS:
*    @return      void        nothing returned
*
***************************************************************************/
VOID MadEvtBufrdIoWorkItem(IN WDFWORKITEM  hBufrdWI)
{
PISR_DPC_WI_CONTEXT   pIsrDpcWiData = MadGetIsrDpcWiContext(hBufrdWI);
PFDO_DATA             pFdoData = (PFDO_DATA)pIsrDpcWiData->pFdoData;

    ASSERT(pIsrDpcWiData->IsrDpcWiTag == eBufrdIoWI);
	
	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	            "MadEvtDmaBufrdIoWorkItem...SerialNo=%d, Status=x%X, Infolen=%ld\n",
				pFdoData->SerialNo, pIsrDpcWiData->NtStatus, (ULONG)pIsrDpcWiData->InfoLen);

    MadCompleteXferReqWithInfo(pIsrDpcWiData->hRequest, pIsrDpcWiData->NtStatus, pIsrDpcWiData->InfoLen);

	pFdoData->hPendingReqs[pIsrDpcWiData->DevReqType] = NULL;

	return;
}

/************************************************************************//**
 * MadEvtDmaWorkItem
 *
 * DESCRIPTION:
 *    This function is the DMA-processing work-item enqueued in the DPC
 *    function. We complete a DMA transaction and the parent request.
 *    Continuing an incomplete DMA transaction is not implemented.
 *    
 * PARAMETERS: 
 *     @param[in]  hDmaWI      handle to the DMA work item.
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
VOID MadEvtDmaWorkItem(IN WDFWORKITEM  hDmaWI)

{
PISR_DPC_WI_CONTEXT   pIsrDpcWiData = MadGetIsrDpcWiContext(hDmaWI);
NTSTATUS              NtStatus       = pIsrDpcWiData->NtStatus;
WDFREQUEST            hRequest       = pIsrDpcWiData->hRequest;
PFDO_DATA             pFdoData       = (PFDO_DATA)pIsrDpcWiData->pFdoData;  
WDFDMATRANSACTION     hDmaXaxn       = pFdoData->hDmaXaxn;
NTSTATUS              NtStatX        = STATUS_SUCCESS;
BOOLEAN               bRC            = MadDmaTransactionDmaCompleted(
                                                                     #ifdef MAD_KERNEL_WONT_CREATE_DMA_ADAPTER
	                                                                 pFdoData->hDevice, //We violate the Wdf function signature here
                                                                     #endif                 
	                                                                 hDmaXaxn, &NtStatX);
size_t                XferCountXaxn  = MadDmaTransactionGetBytesTransferred(
                                                                            #ifdef MAD_KERNEL_WONT_CREATE_DMA_ADAPTER
	                                                                        pFdoData->hDevice, //We violate the Wdf function signature here
                                                                            #endif              
	                                                                        hDmaXaxn);
size_t                XferCountHdw   = pIsrDpcWiData->MadRegs.DmaChainItem0.DTBC; //Where our hardware saves the final xfer count
size_t                XferCount      = 0;

    ASSERT(pIsrDpcWiData->IsrDpcWiTag == eDmaWI);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtDmaWorkItem...SerialNo=%d Status=x%X DmaXaxnCmpltd=%d DmaXaxnGetBytesXferd=%d HdwXferCount=%d\n",
				pFdoData->SerialNo, NtStatus, (ULONG)bRC, (ULONG)XferCountXaxn, (ULONG)XferCountHdw);

	//Prototype caveat: assume the DMA transaction is always completed because our test app knows the Device extents;
	//else we will need to set up another DMA transfer ...just keeping the prototype simple.
	//
    SIMULATION_ASSERT(bRC); 
	UNREFERENCED_PARAMETER(bRC);

	if (NT_SUCCESS(NtStatus))
		XferCount = (XferCountXaxn > 0) ? XferCountXaxn : XferCountHdw;  

	WdfObjectDelete(hDmaXaxn);
	pFdoData->hDmaXaxn = NULL;

	WdfRequestCompleteWithInformation(hRequest, NtStatus, XferCount);
	//
	ULONG XferCountKb = (ULONG)(XferCount / MAD_XFER_UNIT_SIZE);
	pFdoData->MadDevWmiData.InfoClassData.IoCountKb += XferCountKb;
	pFdoData->MadDevWmiData.InfoClassData.PowerUsed_mW += MAD_DMA_MILLIWATTS_PER_XFER_UNIT; 

    pFdoData->hPendingReqs[pIsrDpcWiData->DevReqType] = NULL;

    return;
}


/************************************************************************//**
 * MadEvtErrorWorkItem
 *
 * DESCRIPTION:
 *    This function is the device error work-item enqueued in the DPC
 *    function. We clean up objects and inform the KMDF that we have a 
 *    serious hardware error.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevErrorWI  handle to the Device error work item.
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
VOID MadEvtErrorWorkItem(IN WDFWORKITEM  hDevErrorWI)

{
PISR_DPC_WI_CONTEXT   pIsrDpcWiData = MadGetIsrDpcWiContext(hDevErrorWI); 
PFDO_DATA             pFdoData    = (PFDO_DATA)pIsrDpcWiData->pFdoData;  

WDFREQUEST            hRequest    = pIsrDpcWiData->hRequest;
BOOLEAN               bDMA        = (pIsrDpcWiData->DevReqType >= eDmaRead);
    
    ASSERT(pIsrDpcWiData->IsrDpcWiTag == eDevErrWI);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtErrorWorkItem...SerialNo=%d, status=x%X, DMA=%d\n",
				pFdoData->SerialNo, pIsrDpcWiData->NtStatus, bDMA);

	if (hRequest == NULL) //No identified pending request
	    MadCompleteAnyPendingIOs(pFdoData, pIsrDpcWiData->NtStatus);
	else
	    {
		if (bDMA)
		    {
	        ASSERT(pFdoData->hDmaXaxn != NULL);
	        WdfObjectDelete(pFdoData->hDmaXaxn);
	        pFdoData->hDmaXaxn = NULL;
		    }

		WdfRequestComplete(hRequest, pIsrDpcWiData->NtStatus);
	    }

	// Prototype caveat: all device failures cause setting FailedAction to ...FailedNoRestart
	// Should we distinguish harware-error from device power-failure here?
	// (Set FailedAction to ...FailedAttemptRestart for a hardware error ?)
	//
	WdfDeviceSetFailed(pFdoData->hDevice, WdfDeviceFailedNoRestart);
	MadWriteEventLogMesg(pFdoData->pDriverObj, MADDEVICE_HARDWARE_FAULT, 0, 0, NULL); //Hard-coded message text. No payload

    return;
}

/************************************************************************//**
* MadCompleteXferReqWithInfo
*
* DESCRIPTION:
*    This function completes a bufr'd request & does the WMI accounting.
*
* PARAMETERS:
*     @param[in]  hRequest      handle to request to complete
*     @param[in]  NtStatus      the status to complete the request with
*     @param[in]  IoCount       the size of the I-O
*
* RETURNS:
*    @return      void        nothing returned
*
***************************************************************************/
inline VOID MadCompleteXferReqWithInfo(WDFREQUEST hRequest, NTSTATUS NtStatus, size_t IoCount)

{
static size_t RunningCount = 0;
WDFQUEUE      hIoQueue = WdfRequestGetIoQueue(hRequest);
WDFDEVICE     hDevice = WdfIoQueueGetDevice(hIoQueue);
PFDO_DATA     pFdoData = MadDeviceFdoGetData(hDevice);

	if (NtStatus == STATUS_SUCCESS)
		RunningCount += IoCount;

	if (RunningCount >= MAD_XFER_UNIT_SIZE)
	    {
		//We can increment safely because bufr'd transfers are small
		//so we should be advancing one KB at a time
		//
		pFdoData->MadDevWmiData.InfoClassData.IoCountKb++;
		pFdoData->MadDevWmiData.InfoClassData.PowerUsed_mW += MAD_MILLIWATTS_PER_XFER_UNIT;
		RunningCount -= MAD_XFER_UNIT_SIZE;
	    }

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadCompleteXferReqWithInfo...SerialNo=%d, hRequest=%p, Status=x%X, IoCount=%d, TotlXferCount(KB)=%d\n",
		        pFdoData->SerialNo, hRequest, NtStatus, (ULONG)IoCount, pFdoData->MadDevWmiData.InfoClassData.IoCountKb);

    WdfRequestCompleteWithInformation(hRequest, NtStatus, (ULONG_PTR)IoCount);

	return;
}


/************************************************************************//**
 * MadCompleteAnyPendingIOs
 *
 * DESCRIPTION:
 *    This function completes any pending I/Os for the device.
 *    
 * PARAMETERS: 
 *     @param[in]  pFdoData    pointer to the framework device extension
 *     @param[in]  NtStatus    indictes the completion status of the I/Os
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
VOID  MadCompleteAnyPendingIOs(PFDO_DATA pFdoData, NTSTATUS NtStatus)

{
register ULONG j;

    for (j = eBufrdRead; j < eMltplIO; j++)
        if (pFdoData->hPendingReqs[j] != NULL)
            {
			if (j >= eDmaRead)
				if (pFdoData->hDmaXaxn != NULL)
				    {
	                WdfObjectDelete(pFdoData->hDmaXaxn);
					pFdoData->hDmaXaxn = NULL;
				    }

            WdfRequestComplete(pFdoData->hPendingReqs[j], NtStatus);
            pFdoData->hPendingReqs[j] = NULL;
            }

    return;
}


/************************************************************************//**
 * MadEvtSelfManagedIoInit
 *
 * DESCRIPTION:
 *    This function is called once for each device, after the framework
 *    has called the driver's EvtDeviceD0Entry callback function for the 
 *    first time. The framework does not call the EvtDeviceSelfManagedIoInit
 *    callback function again for that device, unless the device is removed
 *    and reconnected, or the drivers are reloaded.
 *
 *   The EvtDeviceSelfManagedIoInit callback function must initialize the
 *   self-managed I/O operations that the driver will handle for the device.
 *
 *   This function is not marked pageable because this function is in the
 *   device power up path. When a function is marked pagable and the code
 *   section is paged out, it will generate a page fault which could impact
 *   the fast resume behavior because the client driver will have to wait
 *   until the system drivers can service this page fault.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice     handle to our device 
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadEvtSelfManagedIoInit(IN  WDFDEVICE hDevice)
{
NTSTATUS            NtStatus;
PFDO_DATA           pFdoData = MadDeviceFdoGetData(hDevice);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtSelfManagedIoInit enter, SerialNo=%d\n", pFdoData->SerialNo);

    pFdoData = MadDeviceFdoGetData(hDevice);

    // We will provide an example on how to get a bus-specific direct
    // call interface from a bus driver.
    //
    NtStatus = WdfFdoQueryForInterface(hDevice,
                                       &GUID_MADDEVICE_INTERFACE_STANDARD,
                                       (PINTERFACE)&pFdoData->BusInterface,
                                       sizeof(MADDEVICE_INTERFACE_STANDARD),
                                       1, NULL);// InterfaceSpecific Data
    if(NT_SUCCESS(NtStatus))
        {
        UCHAR powerlevel;

        // Call the direct callback functions to get the property or
        // configuration information of the device.
        //
        (*pFdoData->BusInterface.GetPowerLevel)(pFdoData->BusInterface.InterfaceHeader.Context, &powerlevel);
        (*pFdoData->BusInterface.SetPowerLevel)(pFdoData->BusInterface.InterfaceHeader.Context, 8);
        (*pFdoData->BusInterface.IsSafetyLockEnabled)(pFdoData->BusInterface.InterfaceHeader.Context);

        // Provider of this interface may have taken a reference on it.
        // So we must release the interface as soon as we are done using it.
        //
        (*pFdoData->BusInterface.InterfaceHeader.InterfaceDereference)
                            ((PVOID)pFdoData->BusInterface.InterfaceHeader.Context);
        } 
    else 
        {
        // In this sample, we don't want to fail start just because we weren't able to get the direct-call interface.
        // If this driver is loaded on top of a bus other than Model-Abstract, MadDeviceGetStandardInterface will return an error.
        //
        NtStatus = STATUS_SUCCESS;
        }

    return NtStatus;
}


/************************************************************************//**
 * MadEvtIoBufrdRead 
 *
 * DESCRIPTION:
 *    This function initiates a read to the mad device. It performs a read
 *    to the Mad device. This event is called when the framework receives
 *    IRP_MJ_READ requests.
 *    This version manages buffered i/o, byte-aligned, sequential access
 *    
 * PARAMETERS: 
 *     @param[in]  hQueue    handle to our I/O queue for this device.
 *     @param[in]  hRequest  handle to this I/O request
 *     @param[in]  Length    Length of the buffer associated with the request 
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
VOID MadEvtIoBufrdRead(WDFQUEUE hQueue, WDFREQUEST  hRequest, size_t IoLen)

{
static ULONG IntEnableReg = (MAD_INT_STATUS_ALERT_BIT | MAD_INT_BUFRD_INPUT_BIT); 
WDFDEVICE        hDevice   = WdfIoQueueGetDevice(hQueue);
PFDO_DATA        pFdoData  = MadDeviceFdoGetData(hDevice);
PHYSICAL_ADDRESS liReadAddr;
NTSTATUS         NtStatus;
WDFMEMORY        hMemory;
PVOID            pReadBufr;
size_t           NumReadBytes;
ULONG            ControlReg;

    PAGED_CODE();
	UNREFERENCED_PARAMETER(IoLen);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtIoBufrdRead...hRequest=%p, hQueue=%p, IoLen=%d, SerialNo=%d\n",
		        hRequest, hQueue, (ULONG)IoLen, pFdoData->SerialNo);

    if (!MadDevVerifyIoReady(pFdoData, hRequest))
        return;
 
    // Get the request memory and perform read operation here
    //
    NtStatus = WdfRequestRetrieveOutputMemory(hRequest, &hMemory);
    if(!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadEvtIoBufrdRead...can't get a handle to the user i-o buffer\n");
        WdfRequestComplete(hRequest, STATUS_INVALID_PARAMETER);
        return;
        }

    pReadBufr = WdfMemoryGetBuffer(hMemory, &NumReadBytes);
    if (pReadBufr == NULL) 
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadEvtIoBufrdRead...can't retrieve the read buffer\n");
        WdfRequestComplete(hRequest, STATUS_INVALID_PARAMETER);
        return;
        }

    ASSERT(pFdoData->hPendingReqs[eBufrdRead] == NULL);
    pFdoData->hPendingReqs[eBufrdRead] = hRequest;

    //Program the device for the i/o
    //
    //Fit the read count into the proper subset of bits in the control reg
    NumReadBytes = (NumReadBytes / MAD_BYTE_COUNT_MULT);  
    ControlReg   = (ULONG)((NumReadBytes - 1) << MAD_CONTROL_IO_COUNT_SHIFT);
    ControlReg  &=  MAD_CONTROL_IO_COUNT_MASK;
    ControlReg  |= MAD_CONTROL_IOSIZE_BYTES_BIT; //Indicate byte count vs sectors
	//
    liReadAddr = MmGetPhysicalAddress(pReadBufr);
    MadProgramBufrdIo(pFdoData, ControlReg, IntEnableReg, liReadAddr.QuadPart);

    return;
}


/************************************************************************//**
 * MadEvtIoBufrdWrite
 *
 * DESCRIPTION:
 *    This function performs write to the Mad device. This event is called 
 *    when the framework receives IRP_MJ_WRITE requests.
 *    This version manages buffered i/o, byte-aligned, sequential access
 *    
 * PARAMETERS: 
 *     @param[in]  hQueue    handle to our I/O queue for this device.
 *     @param[in]  hRequest  handle to this I/O request
 *                 Length    Length of the buffer associated w/ the request.
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
VOID MadEvtIoBufrdWrite(WDFQUEUE  hQueue, WDFREQUEST hRequest,  size_t IoLen)

{
static ULONG IntEnableReg = (MAD_INT_STATUS_ALERT_BIT | MAD_INT_BUFRD_OUTPUT_BIT); 
WDFDEVICE        hDevice   = WdfIoQueueGetDevice(hQueue);
PFDO_DATA        pFdoData  = MadDeviceFdoGetData(hDevice);
PHYSICAL_ADDRESS liWriteAddr;
NTSTATUS         NtStatus;
WDFMEMORY        hMemory;
PVOID            pWriteBufr;
size_t           NumWriteBytes;
ULONG            ControlReg;

    PAGED_CODE();
	UNREFERENCED_PARAMETER(IoLen);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtIoBufrdWrite...hRequest=%p, hQueue=%p, IoLen=%d, SerialNo=%d\n",
		        hRequest, hQueue, (ULONG)IoLen, pFdoData->SerialNo);

    if (!MadDevVerifyIoReady(pFdoData, hRequest))
        return;

    // Get the request buffer and perform write operation here
    //
    NtStatus = WdfRequestRetrieveInputMemory(hRequest, &hMemory);
    if(!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadEvtIoBufrdWrite...can't get a handle to the user i-o buffer\n");
        WdfRequestComplete(hRequest, STATUS_INVALID_PARAMETER);
        return;
        }

    pWriteBufr = WdfMemoryGetBuffer(hMemory, &NumWriteBytes);
    if (pWriteBufr == NULL) 
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadEvtIoBufrdWrite...can't retrieve the write buffer\n");
        WdfRequestComplete(hRequest, STATUS_INVALID_PARAMETER);
        return;
        }

    ASSERT(pFdoData->hPendingReqs[eBufrdWrite] == NULL);
    pFdoData->hPendingReqs[eBufrdWrite] = hRequest;

    //Program the device for the i/o
    //
    //Fit the read count into the proper subset of bits in the control reg
    NumWriteBytes = (NumWriteBytes / MAD_UNITIO_SIZE_BYTES);  
    ControlReg    = (ULONG)((NumWriteBytes - 1) << MAD_CONTROL_IO_COUNT_SHIFT);
    ControlReg   &= MAD_CONTROL_IO_COUNT_MASK;
    ControlReg   |= MAD_CONTROL_IOSIZE_BYTES_BIT; //Indicate byte count vs sectors
	//
	liWriteAddr   = MmGetPhysicalAddress(pWriteBufr);
    MadProgramBufrdIo(pFdoData, ControlReg, IntEnableReg, liWriteAddr.QuadPart);

    return;
}


/************************************************************************//**
 * MadEvtIoDmaRead
 *
 * DESCRIPTION:
 *    This function initiates a read to the mad device. It performs a read
 *    to the Mad device. This event is called when the framework receives
 *    IRP_MJ_READ requests. This is the direct-io (dma version) which uses
 *    MDLs as input.
 *    This version manages direct i/o, sector-aligned, random access
 *    
 * PARAMETERS: 
 *     @param[in]  hQueue    handle to our I/O queue for this device.
 *     @param[in]  hRequest  handle to this I/O request
 *     @param[in]  Length    Length of the buffer associated with the request 
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
 
VOID MadEvtIoDmaRead(WDFQUEUE hQueue, WDFREQUEST  hRequest, size_t IoLen)

{
WDFDEVICE         hDevice   = WdfIoQueueGetDevice(hQueue);
PFDO_DATA         pFdoData  = MadDeviceFdoGetData(hDevice);
ULONG             SerialNo = pFdoData->SerialNo;
NTSTATUS          NtStatus;
PVOID             pReadBufr;
size_t            ReadLen;
WDFDMATRANSACTION hDmaXaxn;

    PAGED_CODE();
	UNREFERENCED_PARAMETER(SerialNo);
	UNREFERENCED_PARAMETER(IoLen);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtIoDmaRead...hRequest=%p, hQueue=%p, IoLen=%d, SerialNo=%d\n",
		        hRequest, hQueue, (ULONG)IoLen, SerialNo);
	
	if (!MadDevVerifyIoReady(pFdoData, hRequest))
        return;

     // Get the request memory and perform read operation here
    //
    NtStatus = WdfRequestRetrieveOutputBuffer(hRequest, 1, &pReadBufr, &ReadLen);
    if(!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadEvtIoDmaRead:WdfRequestRetrieveOutputBuffer failed!...status=x%X\n", NtStatus);
        WdfRequestComplete(hRequest, NtStatus);
        return;
        }

//Set up and execute a DMA transaction
//
	NtStatus = MadDmaTransactionCreate(
                                       #ifdef MAD_KERNEL_WONT_CREATE_DMA_ADAPTER //We are violating the WDF function signature...I hate this !
		                               hDevice,
                                       #endif
		                               pFdoData->hDmaEnabler, WDF_NO_OBJECT_ATTRIBUTES, &hDmaXaxn);
    if (NtStatus != STATUS_SUCCESS)
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadEvtIoDmaRead:WdfDmaTransactionCreate failed!...status=x%X\n", NtStatus);
        WdfRequestComplete(hRequest, NtStatus);
        return;
        }

    NtStatus = MadDmaTransactionInitializeUsingRequest(hDmaXaxn, hRequest, 
                                                       &MadEvtProgramDma,
													   (WDF_DMA_DIRECTION)FALSE);
    if (!NT_SUCCESS(NtStatus))
       {
       TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		           "MadEvtIoDmaRead:WdfDmaTransactionInitialize failed!...status=x%X\n", NtStatus);
       WdfRequestComplete(hRequest, NtStatus);
       WdfObjectDelete(hDmaXaxn);
       return;
       }

	ASSERT(ReadLen == IoLen);
    //Program the device for the i/o
    //
    NtStatus = MadDmaTransactionExecute(hDmaXaxn, pFdoData);
    if (!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadEvtIoDmaRead:WdfDmaTransactionExecute failed! status=x%X\n", NtStatus);
        WdfRequestComplete(hRequest, NtStatus);
        WdfObjectDelete(hDmaXaxn);
		//Fall through
        }

     return;
}


 /************************************************************************//**
 * MadEvtIoDmaWrite
 *
 * DESCRIPTION:
 *    This function performs write to the Mad device. This event is called 
 *    when the framework receives IRP_MJ_WRITE requests. This is the direct-io 
 *    (dma version) which uses MDLs as input.
 *    This version manages direct i/o, sector-aligned, random access
 *    
 * PARAMETERS: 
 *     @param[in]  hQueue    handle to our I/O queue for this device.
 *     @param[in]  hRequest  handle to this I/O request
 *                 Length    Length of the buffer associated w/ the request.
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
 
VOID MadEvtIoDmaWrite(WDFQUEUE hQueue, WDFREQUEST hRequest,  size_t IoLen)

{
WDFDEVICE         hDevice   = WdfIoQueueGetDevice(hQueue);
PFDO_DATA         pFdoData  = MadDeviceFdoGetData(hDevice);
ULONG             SerialNo = pFdoData->SerialNo;
NTSTATUS          NtStatus;
PVOID             pWriteBufr;
size_t            WriteLen;
WDFDMATRANSACTION hDmaXaxn;

    PAGED_CODE();
	UNREFERENCED_PARAMETER(SerialNo);
	UNREFERENCED_PARAMETER(IoLen);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtIoDmaWrite...hRequest=%p, hQueue=%p, IoLen=%d, SerialNo=%d\n",
		        hRequest, hQueue, (ULONG)IoLen, SerialNo);

    if (!MadDevVerifyIoReady(pFdoData, hRequest))
        return;

    // Get the request memory and perform read operation here
    //
    NtStatus = WdfRequestRetrieveInputBuffer(hRequest, 1, &pWriteBufr, &WriteLen);
    if(!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadEvtIoDmaWrite:WdfRequestRetrieveInputBuffer failed!...status=x%X, SerialNo=%d\n",
					NtStatus, SerialNo);
        WdfRequestComplete(hRequest, NtStatus);
        return;
        }

//Set up and execute a DMA transaction
//
    NtStatus = MadDmaTransactionCreate(
                                       #ifdef MAD_KERNEL_WONT_CREATE_DMA_ADAPTER //We are violating the WDF function signature...I hate this !
	                                   hDevice,
                                       #endif
	                                   pFdoData->hDmaEnabler, WDF_NO_OBJECT_ATTRIBUTES, &hDmaXaxn);
    if (NtStatus != STATUS_SUCCESS)
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadEvtIoDmaWrite:WdfDmaTransactionCreate failed...status=x%X, SerialNo=%d\n", 
					NtStatus, SerialNo);
        WdfRequestComplete(hRequest, NtStatus);
        return;
        }

    NtStatus = MadDmaTransactionInitializeUsingRequest(hDmaXaxn, hRequest, 
                                                       &MadEvtProgramDma,
													   (WDF_DMA_DIRECTION)TRUE);
    if (!NT_SUCCESS(NtStatus))
       {
       TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		           "MadEvtIoDmaWrite:WdfDmaTransactionInitialize failed...status=x%X, SerialNo=%d\n",
				   NtStatus, SerialNo);
       WdfRequestComplete(hRequest, NtStatus);
       WdfObjectDelete(hDmaXaxn);
       return;
       }

	ASSERT(WriteLen == IoLen);
    //Program the device for the i/o
    //
    NtStatus = MadDmaTransactionExecute(hDmaXaxn, pFdoData);
    if (!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadEvtIoDmaWrite:WdfDmaTransactionExecute failed! status=x%X, SerialNo=%d\n",
					NtStatus, SerialNo);
        WdfRequestComplete(hRequest, NtStatus);
        WdfObjectDelete(hDmaXaxn);
		//Fall through
        }

    return;
}


/************************************************************************//**
 * MadDevVerifyIoReady
 *
 * DESCRIPTION:
 *    This function determines if the device is ready for the next I/O.
 *    
 * PARAMETERS: 
 *     @param[in]  pFdoData    pointer to the framework device extension
 *     @param[in]  hRequest    handle to this I/O request
 *     
 * RETURNS:
 *    @return      BOOLEAN     indicates that the device is ready for I/O
 * 
 ***************************************************************************/
BOOLEAN MadDevVerifyIoReady(PFDO_DATA pFdoData, WDFREQUEST hRequest)
{
    if (pFdoData->DevStatus != 0)
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadEvtIoXXX: returning known device error...DevStatus=x%X, NtStatus=x%X, SerialNo=%d\n",
                    pFdoData->DevStatus, pFdoData->DevNtStatus, pFdoData->SerialNo);
        WdfRequestComplete(hRequest, pFdoData->DevNtStatus);
        pFdoData->DevStatus   = 0;
        pFdoData->DevNtStatus = STATUS_SUCCESS;
        return FALSE;
        }

    if (pFdoData->CurrDevPowerState > PowerDeviceD0)
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadEvtIoXXX: returning device powered down...SerialNo=%d\n", pFdoData->SerialNo);
        WdfRequestComplete(hRequest, STATUS_POWER_STATE_INVALID);
        return FALSE;
        }

    return TRUE;
}


/**************************************************************************//**
 * MadEvtProgramDma
 *
 * DESCRIPTION:
 *    This function programs the device for a DMA transfer. It is the callback
 *    indicated in the framework WdfDmaTransactionInitializeUsingRequest
 *    
 * PARAMETERS: 
 *     @param[in]  hDmaXaxn    handle to this DMA transaction.
 *     @param[in]  hDevice     handle to our device 
 *     @param[in]  Context     pointer to the framework device extension
 *     @param[in]  Direxn      direction of DMA transfer
 *     @param[in]  pSgList     pointer to the scatter-gather list derived
 *                             from the MDL
 *     
 * RETURNS:
 *    @return      BOOLEAN     indicates success or failure
 * 
 *****************************************************************************/
BOOLEAN MadEvtProgramDma (IN WDFDMATRANSACTION hDmaXaxn, WDFDEVICE hDevice,
                          IN PVOID Context, IN WDF_DMA_DIRECTION Direxn,
						  IN PSCATTER_GATHER_LIST pSgList)

{
static ULONG64 Zero64       = 0;
WDFREQUEST     hRequest     = MadDmaTransactionGetRequest(
                                                          #ifdef MAD_KERNEL_WONT_CREATE_DMA_ADAPTER
	                                                      hDevice, //We are violating the WDF function signature...I hate this
                                                          #endif       
	                                                      hDmaXaxn);
PFDO_DATA      pFdoData     = MadDeviceFdoGetData(hDevice);
ULONG          SerialNo     = pFdoData->SerialNo;
PMADREGS       pMadRegs     = pFdoData->pMadRegs;
ULONG          IntEnableReg = MAD_INT_STATUS_ALERT_BIT; 
ULONG          ControlReg   = 0;
ULONG64        DevLoclAddr  = 0;
ULONG          DTBC         = 0;   
ULONG          DmaCntl      = MAD_DMA_CNTL_INIT;
MAD_DMA_CHAIN_ELEMENT    MadOneBlockDma = {Zero64/*Host addr*/, Zero64/*DevLoclAddr - device offset)*/, 
                                           0/*DmaCntl*/, 0/*DTBC*/, 0/*CDPP*/}; 
WDF_REQUEST_PARAMETERS DmaReqParms; 

    ASSERT(Context == pFdoData);
    UNREFERENCED_PARAMETER(Context);

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtProgramDma: SerialNo=%d, pSgList=%p\n", SerialNo, pSgList);

	if (pSgList == NULL)
	    {
		TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		            "MadEvtProgramDma: returning invalid_parm...SerialNo=%d, pSgList=%p\n",
					pFdoData->SerialNo, pSgList);
		return FALSE;
	    }

	BOOLEAN bChained = (pSgList->NumberOfElements > 1);

	WDF_REQUEST_PARAMETERS_INIT(&DmaReqParms);
	WdfRequestGetParameters(hRequest, &DmaReqParms);
 
    if ((BOOLEAN)Direxn == FALSE) // It's a read
		{
        IntEnableReg |= MAD_INT_DMA_INPUT_BIT;

		DevLoclAddr = (ULONG64)DmaReqParms.Parameters.Read.DeviceOffset;
		DTBC        = (ULONG)DmaReqParms.Parameters.Read.Length;
	    }
    else // It's a write 
        {
        //DMA write is indicated by two different bits...our device is a little screwy
        IntEnableReg |= MAD_INT_DMA_OUTPUT_BIT;
        DmaCntl      |= MAD_DMA_CNTL_H2D; //Host-->Device

		DevLoclAddr = (ULONG64)DmaReqParms.Parameters.Write.DeviceOffset;
		DTBC        = (ULONG)DmaReqParms.Parameters.Write.Length;
        }
	ASSERT((DevLoclAddr % MAD_SECTOR_SIZE) == 0);

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtProgramDma...SerialNo=%d, IoLen=%d, Chained=%d, #SglElems=%d, Direxn=%d, DevAddr=x%X\n",
				pFdoData->SerialNo, (ULONG)DTBC, (ULONG)bChained, pSgList->NumberOfElements, (ULONG)Direxn, (ULONG)DevLoclAddr);

    if (!bChained) //One block is sufficient
	    {
        pFdoData->hPendingReqs[(ULONG)(eDmaRead+Direxn)] = hRequest; // R/W
		//
		MadOneBlockDma.HostAddr    = pSgList->Elements[0].Address.QuadPart;
		MadOneBlockDma.DevLoclAddr = DevLoclAddr;
		MadOneBlockDma.DmaCntl     = DmaCntl; 
		MadOneBlockDma.DTBC        = DTBC;
		MadOneBlockDma.CDPP        = MAD_DMA_CDPP_END;
	    }
 	else
        {//Set up chained (SG)DMA
        pFdoData->hPendingReqs[(ULONG)(eSgDmaRead+Direxn)] = hRequest; // R/W

		//Assign the 1st chain element device addr to the initial device offset determined 
		// from the request parms above 
		pFdoData->HdwSgDmaList[0].DevLoclAddr = DevLoclAddr; 

		//Set the chained-DMA pointer to the start of the chain & build the hardware's chained DMA list in memory
		//
        MadOneBlockDma.CDPP = pFdoData->liCDPP.QuadPart; //Our static pre-allocated chain
        Mad_BuildDeviceChainedDmaListFromWinSgList(pFdoData, pSgList, (BOOLEAN)Direxn);
		//
        ControlReg |= MAD_CONTROL_CHAINED_DMA_BIT;
        }

	MadInterruptAcquireLock(pFdoData->hInterrupt);
	//
	// Here we leverage the fact that our DMA registers are aggregated & we do one
	// bulk write accross the bus - even though the MadBus is really, really fast  :)
	// This would not work if our DMA registers were scattered about our device
	// See below
	//
	//WRITE_REGISTER_BUFFER_UCHAR((PUCHAR)&pMadRegs->OneBlockDmaRegs, 
	//	                        (PUCHAR)&MadOneBlockDma, sizeof(MAD_DMA_CHAIN_ELEMENT));
	//
	//Program the DMA registers individually if necessary 
	//
    WRITE_REGISTER_BUFFER_UCHAR((PUCHAR)&pMadRegs->DmaChainItem0.HostAddr,
                                (PUCHAR)&MadOneBlockDma.HostAddr, sizeof(ULONG64));
    WRITE_REGISTER_BUFFER_UCHAR((PUCHAR)&pMadRegs->DmaChainItem0.DevLoclAddr,
                                (PUCHAR)&MadOneBlockDma.DevLoclAddr, sizeof(ULONG64));
    WRITE_REGISTER_ULONG(&pMadRegs->DmaChainItem0.DmaCntl, MadOneBlockDma.DmaCntl);
    WRITE_REGISTER_ULONG(&pMadRegs->DmaChainItem0.DTBC, MadOneBlockDma.DTBC);
	WRITE_REGISTER_BUFFER_UCHAR((PUCHAR)&pMadRegs->DmaChainItem0.CDPP,
                                (PUCHAR)&MadOneBlockDma.CDPP, sizeof(ULONG64));
	//
    WRITE_REGISTER_ULONG(&pMadRegs->IntEnable, IntEnableReg);
    WRITE_REGISTER_ULONG(&pMadRegs->Control, ControlReg);
    //
    //Hit the Go bit as a separate register write
    WRITE_REGISTER_ULONG(&pMadRegs->Control, (ControlReg | MAD_CONTROL_DMA_GO_BIT));
	//
	MadInterruptReleaseLock(pFdoData->hInterrupt);

	ASSERT(pFdoData->hDmaXaxn == NULL);
	pFdoData->hDmaXaxn = hDmaXaxn;

    return TRUE;
}


/************************************************************************//**
 * Mad_BuildDeviceChainedDmaListFromWinSgList
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
VOID Mad_BuildDeviceChainedDmaListFromWinSgList(PFDO_DATA pFdoData, PSCATTER_GATHER_LIST pSgList, BOOLEAN bWrite)

{
register ULONG           j;
PHYSICAL_ADDRESS         liCDPP;
PMAD_DMA_CHAIN_ELEMENT   pHdwSgItem   = &(pFdoData->HdwSgDmaList[0]);
PMAD_DMA_CHAIN_ELEMENT   pHdwSgNext   = &(pFdoData->HdwSgDmaList[1]);
PSCATTER_GATHER_ELEMENT  pOsSgElement = &(pSgList->Elements[0]); 
//
ULONG64                  DevOffset     = pHdwSgItem->DevLoclAddr; //Device-relative start of this i/o
ULONG                    NumElements   = pSgList->NumberOfElements;
ULONG                    DmaCntlInit   = MAD_DMA_CNTL_INIT;
ULONG                    TotlXferLen   = 0;
ULONG                    BlockSize;

    ASSERT(NumElements > 1); //Else we shouldn't be here
    ASSERT(NumElements <= MAD_DMA_MAX_SECTORS); //No more than our Device extent  

    if (bWrite)
        DmaCntlInit |= MAD_DMA_CNTL_H2D; //Host-->Device

    for (j = 0; j < NumElements; j++)
	    {  
		pHdwSgItem->HostAddr    = pOsSgElement->Address.QuadPart;
		pHdwSgItem->DevLoclAddr = DevOffset; //Although it's been assigned above for the 1st pass
		pHdwSgItem->DmaCntl     = DmaCntlInit; 
		//
		BlockSize = pOsSgElement->Length;
        pHdwSgItem->DTBC        = BlockSize;
        //
        TotlXferLen += BlockSize;
        DevOffset   += BlockSize;
        //
		if (j == (NumElements-1)) //No need to set up the next pass so ...
			break;                //exit from loop
		
		//Set CDPP to next item: Item[X].CDPP --> Item[X+1]
		liCDPP = MmGetPhysicalAddress(pHdwSgNext);
        pHdwSgItem->CDPP = liCDPP.QuadPart;  //Can't cast: C2664
		
		//Advance the list pointers ...
		//We could work with array indeces but the compiler's address calculation(s) would be gnarly
		//
        pOsSgElement++;
		pHdwSgItem++;
        pHdwSgNext++;
 	    } //next j

	//End the list the way the hardware expects
	//No need to decrement / backup since we broke out of the loop above
    ////pHdwSgItem--; //Point to the last element
	//
    pHdwSgItem->CDPP = MAD_DMA_CDPP_END; //Indicate no forward link 

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "Mad_BuildDeviceChainedDmaListFromWinSgList...SerialNo=%d, Direxn=%d, #Elements=%d, TotlXferLen=%d, DevOfst(end)=%d\n",
				pFdoData->SerialNo, (ULONG)bWrite, NumElements, TotlXferLen, (ULONG)DevOffset);

    return;
}


/************************************************************************//**
 * MadEvtIoDeviceControl
 *
 * DESCRIPTION:
 *    This function s called when the framework receives IRP_MJ_DEVICE_CONTROL
    requests from the system.
 *    
 * PARAMETERS: 
 *     @param[in]  hQueue      handle to our I/O queue for this device.
 *     @param[in]  hRequest    handle to this I/O request
 *     @param[in]  OutBufrLen  length of the request's output buffer  
 *     @param[in]  InBufrLen   length of the request's input buffer   
 *     @param[in]  IoControlCode    the driver/system-defined I/O control code
                                    associated with the request.
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
VOID MadEvtIoDeviceControl(IN WDFQUEUE hQueue, IN WDFREQUEST hRequest,
                           IN size_t  OutBufrLen, IN size_t InBufrLen,
                           IN ULONG IoControlCode)

{
static PHYSICAL_ADDRESS liZERO = {0L, 0L};
static ULONG64          Zero64 = 0;
WDFDEVICE               hDevice = WdfIoQueueGetDevice(hQueue);
PFDO_DATA               pFdoData = MadDeviceFdoGetData(hDevice);
PMADREGS                pMadRegs = pFdoData->pMadRegs;
LONG                    SerialNo = pFdoData->SerialNo; 
WDF_DEVICE_STATE        deviceState;
PVOID                   pInBufr = NULL;
PVOID                   pOutBufr = NULL;
PMADDEV_IOCTL_STRUCT    pMadDevIoctl;
PMADDEV_MAP_VIEWS       pMadDevMapViews;
size_t                  CompLen;
size_t                  InfoLen = 0;
PHYSICAL_ADDRESS        liIoAddr;
ULONG                   ControlReg;  
ULONG                   IntEnableReg;
ULONG                   OffsetBits;
ULONG                   CacheIndx;
NTSTATUS                NtStatus = STATUS_SUCCESS;
PULONG                  pX;

    PAGED_CODE();
    UNREFERENCED_PARAMETER(OutBufrLen);
    UNREFERENCED_PARAMETER(InBufrLen);

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadEvtIoDeviceControl enter...IoControlCode=x%X, SerialNo=%d\n",
		        IoControlCode, SerialNo);

	//We always need the security key from the input buffer
    NtStatus = WdfRequestRetrieveInputBuffer(hRequest, 1, (PVOID *)&pInBufr, &InfoLen);
    if (!NT_SUCCESS(NtStatus))
       {
       TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		           "MadEvtIoDeviceControl:WdfRequestRetrieveInputBuffer returned...NtStatus=x%X, SerialNo=%d\n",
                   NtStatus, SerialNo);

	    WdfRequestComplete(hRequest, NtStatus);
        return;
        }

	//We may need the output (return) buffer
	NtStatus = WdfRequestRetrieveOutputBuffer(hRequest, 1, (PVOID *)&pOutBufr, &InfoLen);
	if (!NT_SUCCESS(NtStatus))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	                "MadEvtIoDeviceControl:WdfRequestRetrieveOutputBuffer returned...NtStatus=x%X, SerialNo=%d\n",
                     NtStatus, SerialNo);
		}
	
	InfoLen = 0;

	//Verify the security key
	//
    pMadDevIoctl = (PMADDEV_IOCTL_STRUCT)pInBufr;
    CompLen      = RtlCompareMemory(&pMadDevIoctl->SecurityKey, 
                                    &GUID_DEVINTERFACE_MADDEVICE, sizeof(GUID));
    if (CompLen != sizeof(GUID))
        {
        TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			        "MadEvtIoDeviceControl...incorrect security key - returning invalid security; SerialNo=%d\n",
			        SerialNo);
        NtStatus = STATUS_INVALID_ID_AUTHORITY;
        WdfRequestComplete(hRequest, NtStatus);
        return;
        }

	if (pOutBufr != NULL)
	    pMadDevIoctl = (PMADDEV_IOCTL_STRUCT)pOutBufr;

    switch (IoControlCode) 
        {
        case IOCTL_MADDEVICE_DONT_DISPLAY_IN_UI_DEVICE:
            // This is just an example on how to hide your device in the device manager. 
            // Please remove this code when you adapt this sample for your hardware.
            //
            WDF_DEVICE_STATE_INIT(&deviceState);
            deviceState.DontDisplayInUI = WdfTrue;
            WdfDeviceSetDeviceState(hDevice, &deviceState);
            break;

        case MADDEV_IOCTL_INITIALIZE:
			TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				        "MadEvtIoDeviceControl: (re)initialize the device...SerialNo=%d\n", SerialNo);

            //We can service this request right here
            {
			MADREGS MadRegs;
			RtlFillMemory(&MadRegs, sizeof(MADREGS), 0x00);

			MadInterruptAcquireLock(pFdoData->hInterrupt);
			//
            #ifdef XOR_REGBITS_SET_BY_DEVICE //Fetch the registers we will need to eXclusive-OR
			ULONG StatusReg = READ_REGISTER_ULONG(&pMadRegs->Status);
            ULONG IntIdReg  = READ_REGISTER_ULONG(&pMadRegs->IntID);
            #endif

            WRITE_REGISTER_BUFFER_UCHAR((PUCHAR)pMadRegs, (PUCHAR)&MadRegs, sizeof(MADREGS));

			#ifdef XOR_REGBITS_SET_BY_DEVICE //Store the registers with the proper reset values by eXclusive-ORing
            MadResetStatusReg(pMadRegs, StatusReg);
            MadResetIntIdReg(pMadRegs, IntIdReg);
            #endif

			WRITE_REGISTER_ULONG(&pMadRegs->IntEnable, MAD_ALL_INTS_ENABLED_BITS);
			//
			MadInterruptReleaseLock(pFdoData->hInterrupt);
			}

            //And we're done
            WdfRequestComplete(hRequest, STATUS_SUCCESS);
            return;

		case MADDEV_IOCTL_GET_INTEN_REG:
            //We can service this request right here
			MadInterruptAcquireLock(pFdoData->hInterrupt);
			IntEnableReg = READ_REGISTER_ULONG(&pMadRegs->IntEnable);
			MadInterruptReleaseLock(pFdoData->hInterrupt);

			*((PULONG)pMadDevIoctl->DataBufr) = IntEnableReg;

			TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				        "MadEvtIoDeviceControl: Get the IntEnable register from the device...SerialNo=%d, IntEnableReg=x%X\n",
						SerialNo, IntEnableReg);
			//And we're done
            WdfRequestCompleteWithInformation(hRequest, STATUS_SUCCESS, sizeof(MADDEV_IOCTL_STRUCT));
            return;

		case MADDEV_IOCTL_SET_INTEN_REG:
            //We can service this request right here
 			MadInterruptAcquireLock(pFdoData->hInterrupt);
			//
            IntEnableReg = *((PULONG)pMadDevIoctl->DataBufr);
			WRITE_REGISTER_ULONG(&pMadRegs->IntEnable, IntEnableReg);
			//
			MadInterruptReleaseLock(pFdoData->hInterrupt);

			TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				        "MadEvtIoDeviceControl: Set the IntEnable register to the device...SerialNo=%d, IntEnableReg=x%X\n",
						SerialNo, IntEnableReg);
	
            //And we're done
            WdfRequestComplete(hRequest, STATUS_SUCCESS);
            return;

		case MADDEV_IOCTL_GET_MAD_CONTROL_REG:
            //We can service this request right here
 			MadInterruptAcquireLock(pFdoData->hInterrupt);
            ControlReg = READ_REGISTER_ULONG(&pMadRegs->Control);
			MadInterruptReleaseLock(pFdoData->hInterrupt);
			
			*((PULONG)pMadDevIoctl->DataBufr) = ControlReg;

			TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				        "MadEvtIoDeviceControl: Get the Control register from the device...SerialNo=%d, ControlReg=x%X\n",
						SerialNo, ControlReg);
	
            //And we're done
            WdfRequestCompleteWithInformation(hRequest, STATUS_SUCCESS, sizeof(MADDEV_IOCTL_STRUCT));
            return;

		case MADDEV_IOCTL_SET_MAD_CONTROL_REG:
            //We can service this request right here
 			MadInterruptAcquireLock(pFdoData->hInterrupt);
			//
            ControlReg = *((PULONG)pMadDevIoctl->DataBufr);
			WRITE_REGISTER_ULONG(&pMadRegs->Control, ControlReg);
			//
			MadInterruptReleaseLock(pFdoData->hInterrupt);

			TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				        "MadEvtIoDeviceControl: Set the Control register to the device...SerialNo=%d, ControlReg=x%X\n",
						SerialNo, ControlReg);
	
            //And we're done
            WdfRequestComplete(hRequest, STATUS_SUCCESS);
            return;

        case MADDEV_IOCTL_RESET_INDECES:
			TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				        "MadEvtIoDeviceControl: reset device indeces...SerialNo=%d\n", SerialNo);

            //We can service this request right here
			//
			MadInterruptAcquireLock(pFdoData->hInterrupt);
			//
            WRITE_REGISTER_ULONG(&pMadRegs->ByteIndxRd, 0);
            WRITE_REGISTER_ULONG(&pMadRegs->ByteIndxWr, 0);
            WRITE_REGISTER_ULONG(&pMadRegs->CacheIndxRd, 0);
            WRITE_REGISTER_ULONG(&pMadRegs->CacheIndxWr, 0);
            WRITE_REGISTER_ULONG(&pMadRegs->PioCacheReadLen, 0);
            WRITE_REGISTER_ULONG(&pMadRegs->PioCacheWriteLen, 0);
			//
			MadInterruptReleaseLock(pFdoData->hInterrupt);

            //And we're done
            WdfRequestComplete(hRequest, STATUS_SUCCESS);
            return;

        case MADDEV_IOCTL_CACHE_READ:
            TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				        "MadEvtIoDeviceControl: read from cache...SerialNo=%d\n", SerialNo);
            ControlReg   = MAD_CONTROL_CACHE_XFER_BIT;
            IntEnableReg = (MAD_INT_STATUS_ALERT_BIT | MAD_INT_BUFRD_INPUT_BIT);
            liIoAddr     = MmGetPhysicalAddress(pMadDevIoctl->DataBufr);
            MadProgramBufrdIo(pFdoData, ControlReg, IntEnableReg, liIoAddr.QuadPart);
            ASSERT(pFdoData->hPendingReqs[eCachedRead] == NULL);
            pFdoData->hPendingReqs[eCachedRead] = hRequest;
            return; 

        case MADDEV_IOCTL_CACHE_WRITE:
            TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				        "MadEvtIoDeviceControl: write to cache...SerialNo=%d\n", SerialNo);
            ControlReg   = MAD_CONTROL_CACHE_XFER_BIT;
            IntEnableReg = (MAD_INT_STATUS_ALERT_BIT | MAD_INT_BUFRD_OUTPUT_BIT);
            liIoAddr     = MmGetPhysicalAddress(pMadDevIoctl->DataBufr);
            MadProgramBufrdIo(pFdoData, ControlReg, IntEnableReg, liIoAddr.QuadPart);
            ASSERT(pFdoData->hPendingReqs[eCachedWrite] == NULL);
            pFdoData->hPendingReqs[eCachedWrite] = hRequest;
            return; 

		case MADDEV_IOCTL_ALIGN_READ:
			pX = (PULONG)pMadDevIoctl->DataBufr;
			CacheIndx  = *pX;  
            CacheIndx  = (CacheIndx / MAD_CACHE_ALIGN_MULTIPLE);
			CacheIndx  = (CacheIndx * MAD_CACHE_ALIGN_MULTIPLE);
            TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				        "MadEvtIoDeviceControl...align read cache to indx %d; SerialNo=%d\n",
				        CacheIndx, SerialNo);

			OffsetBits   = (CacheIndx / MAD_CACHE_ALIGN_MULTIPLE); 
            OffsetBits   = (OffsetBits << MAD_CONTROL_IO_OFFSET_SHIFT);
            ControlReg   = (OffsetBits | MAD_CONTROL_CACHE_XFER_BIT);
            IntEnableReg = (MAD_INT_STATUS_ALERT_BIT | MAD_INT_ALIGN_INPUT_BIT);
            MadProgramBufrdIo(pFdoData, ControlReg, IntEnableReg, Zero64);
            ASSERT(pFdoData->hPendingReqs[eReadAlign] == NULL);
            pFdoData->hPendingReqs[eReadAlign] = hRequest;
            return; 

        case MADDEV_IOCTL_ALIGN_WRITE:
			pX = (PULONG)pMadDevIoctl->DataBufr;
			CacheIndx  = *pX;  
			CacheIndx  = (CacheIndx / MAD_CACHE_ALIGN_MULTIPLE);
			CacheIndx  = (CacheIndx * MAD_CACHE_ALIGN_MULTIPLE);
            TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				        "MadEvtIoDeviceControl...align write cache to indx %d; SerialNo=%d\n",
				        CacheIndx, SerialNo);

			OffsetBits   = (CacheIndx / MAD_CACHE_ALIGN_MULTIPLE); 
            OffsetBits   = (OffsetBits << MAD_CONTROL_IO_OFFSET_SHIFT);
            ControlReg   = (OffsetBits | MAD_CONTROL_CACHE_XFER_BIT);
            IntEnableReg = (MAD_INT_STATUS_ALERT_BIT | MAD_INT_ALIGN_OUTPUT_BIT);
            MadProgramBufrdIo(pFdoData, ControlReg, IntEnableReg, Zero64);
            ASSERT(pFdoData->hPendingReqs[eWriteAlign] == NULL);
            pFdoData->hPendingReqs[eWriteAlign] = hRequest;
            return; 

        case MADDEV_IOCTL_MAP_VIEWS:
            TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				        "MadEvtIoDeviceControl...mapping device# (%d) to the test app address space\n",
                        SerialNo);

            pMadDevMapViews = (PMADDEV_MAP_VIEWS)pMadDevIoctl->DataBufr;
            NtStatus        = MadDev_MapViews(pFdoData, pMadDevMapViews, SerialNo);
            //if (NT_SUCCESS(NtStatus))
            InfoLen = sizeof(MADDEV_IOCTL_STRUCT);
            break;

        case MADDEV_IOCTL_UNMAP_VIEWS:
            TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				        "MadEvtIoDeviceControl...unmapping device# (%d) from the test app address space\n",
                        SerialNo);

            pMadDevIoctl    = (PMADDEV_IOCTL_STRUCT)pInBufr;
			pMadDevMapViews = (PMADDEV_MAP_VIEWS)pMadDevIoctl->DataBufr;
            NtStatus        = Mad_UnmapUsrAddr(pMadDevMapViews->pDeviceRegs);
            break;

        default:
			TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				        "MadEvtIoDeviceControl...returning invalid device request; Ioctl=x%X\n, SerialNo=%d",
				        IoControlCode, SerialNo);
            NtStatus = STATUS_INVALID_DEVICE_REQUEST;
        } // end switch

    // Complete the Request.
    //
    MadCompleteXferReqWithInfo(hRequest, NtStatus, InfoLen);

    return;
}


/************************************************************************//**
 * MadProgramBufrdIo
 *
 * DESCRIPTION:
 *    This function programs the device for buffered i/o..
 *    
 * PARAMETERS: 
 *     @param[in]  pFdoData     pointer to the framework device extension
 *     @param[in]  IntEnableReg The bit-data to load to the IntEnable register
 *     @param[in]  IoAddr       physical address of the data buffer 
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
VOID  MadProgramBufrdIo(PFDO_DATA pFdoData, 
                        ULONG ControlReg, ULONG IntEnableReg, ULONG64 IoAddr)
   
{
PMADREGS pMadRegs = pFdoData->pMadRegs;

     MadInterruptAcquireLock(pFdoData->hInterrupt);
	 //
     WRITE_REGISTER_BUFFER_UCHAR((PUCHAR)&pMadRegs->DmaChainItem0.HostAddr,
                                 (PUCHAR)&IoAddr, sizeof(ULONG64));
     WRITE_REGISTER_ULONG(&pMadRegs->IntEnable, IntEnableReg);
     WRITE_REGISTER_ULONG(&pMadRegs->Control, ControlReg);
     //
     //Hit the Go bit as a separate register write
     WRITE_REGISTER_ULONG(&pMadRegs->Control, (ControlReg | MAD_CONTROL_BUFRD_GO_BIT));
	 //
     MadInterruptReleaseLock(pFdoData->hInterrupt);

     return;
}


/************************************************************************//**
 * MadDev_MapViews
 *
 * DESCRIPTION:
 *    This function maps device components into the calling program's
 *    virtual address space. One internal mapping is sufficient because
 *    all components are contiguous
 *    
 * PARAMETERS: 
 *     @param[in]  pFdoData        pointer to the framework device extension
 *     @param[in]  pMadDevMapViews pointer to the structure containing 
 *                                 the pointers to the device components
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadDev_MapViews(PFDO_DATA pFdoData, PMADDEV_MAP_VIEWS pMadDevMapViews, ULONG SerialNo)

{
PVOID pDevBase  = (PVOID)pFdoData->pMadRegs; 
//PHYSICAL_ADDRESS liDeviceRegs = {0, 0};
NTSTATUS NtStatus;

    UNREFERENCED_PARAMETER(SerialNo);

	//One mapping for all components of device memory
	//
    pMadDevMapViews->pDeviceRegs = NULL;
    NtStatus = Mad_MapSysAddr2UsrAddr(pDevBase, MAD_DEVICE_MAP_MEM_SIZE,
                                      &pMadDevMapViews->liDeviceRegs, &pMadDevMapViews->pDeviceRegs);

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDev_MapViews...pFdoData=%p, SerialNo=%d, NtStatus=x%X, liDeviceRegs=x%X:%X\n",
                pFdoData, SerialNo, NtStatus,
				pMadDevMapViews->liDeviceRegs.HighPart, pMadDevMapViews->liDeviceRegs.LowPart);

    if (pMadDevMapViews->pDeviceRegs != NULL)
	    {
        //All components are contiguous so using offsets works fine
        pMadDevMapViews->pPioRead  = 
        (PVOID)((ULONG_PTR)pMadDevMapViews->pDeviceRegs + MAD_MAPD_READ_OFFSET); 
        pMadDevMapViews->pPioWrite = 
        (PVOID)((ULONG_PTR)pMadDevMapViews->pDeviceRegs + MAD_MAPD_WRITE_OFFSET); 
	    }

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDev_MapWholeDevice...SerialNo=%d, pDeviceRegs=%p, pPioRead=%p, pPioWrite=%p\n",
                SerialNo, pMadDevMapViews->pDeviceRegs, 
				pMadDevMapViews->pPioRead, pMadDevMapViews->pPioWrite);

    return NtStatus;
}


#ifdef _MAD_SIMULATION_MODE_ //===================================================================================
// Define replacement functions for WdfInterruptAcquireLock, WdfInterruptReleaseLock
//
static KIRQL  BaseIRQL; //This is only to be used by the two functions immediately below
//
/************************************************************************//**
 * MadInterruptAcquireLock
 *
 * DESCRIPTION:
 *    This is the simulation mode replacement for WdfInterruptAcquireLock
 *    
 * PARAMETERS: 
 *     @param[in]  hInterrupt  handle to the device interrupt object
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
#pragma warning(suppress: 28167) //Because we acquire a spinlock in a function w/out releasing it
VOID MadInterruptAcquireLock(IN WDFINTERRUPT hInterrupt)

{
WDFDEVICE hDevice  = WdfInterruptGetDevice(hInterrupt);
PFDO_DATA pFdoData = MadDeviceFdoGetData(hDevice);

    MAD_ACQUIRE_LOCK_RAISE_IRQL(pFdoData->hIntSpinlock, pFdoData->Irql, &BaseIRQL);

    return;
}
//
/************************************************************************//**
 * MadInterruptReleaseLock
 *
 * DESCRIPTION:
 *    This is the simulation mode replacement for  WdfInterruptReleaseLock.
 *    
 * PARAMETERS: 
 *     @param[in]  hInterrupt  handle to the device interrupt object
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
#pragma warning(suppress: 28167) //Because we release a spinlock w/out acquiring it
VOID MadInterruptReleaseLock(IN WDFINTERRUPT hInterrupt)
{
WDFDEVICE hDevice  = WdfInterruptGetDevice(hInterrupt);
PFDO_DATA pFdoData = MadDeviceFdoGetData(hDevice);

    #pragma warning(suppress: 26110) //Because we release a spinlock w/out acquiring it
    MAD_LOWER_IRQL_RELEASE_LOCK(BaseIRQL, pFdoData->hIntSpinlock);

    return;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
#ifdef MAD_KERNEL_WONT_CREATE_DMA_ADAPTER 
//Here we replace all the DMA processing functions which WDF won't fulfill because the kernel won't provide 
//a DMA adapter for devices not on (DMA-capable) PCI. Therefore WDF can't provide a DMA Enabler ... etc.
//The functions contained within this #ifdef are shell function which call the simulator implementations by pointer
//
/************************************************************************//**
 * MadDmaEnablerCreate
 *
 * DESCRIPTION:
 *    This is the shell function to call the simulation mode replacement for 
 *    WdfDmaEnablerCreate. See the called function in the simulator.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice      handle to the parent device 
 *     @param[out] phDmaEnabler pointer to the dma enabler handle
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadDmaEnablerCreate(IN WDFDEVICE hDevice, IN PWDF_DMA_ENABLER_CONFIG pDmaConfig, 
							 IN PWDF_OBJECT_ATTRIBUTES pObjAttr, OUT WDFDMAENABLER* phDmaEnabler)

{
PFDO_DATA               pFdoData = MadDeviceFdoGetData(hDevice);
PFN_DMA_ENABLER_CREATE  pDmaEnablerCreate = pFdoData->pMadSimDmaFunxns->pDmaEnablerCreate;

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDmaEnablerCreate...SerialNo=%d pDmaEnablerCreate=%p\n",
				pFdoData->SerialNo, pDmaEnablerCreate);

	return pDmaEnablerCreate(hDevice, pDmaConfig, pObjAttr, phDmaEnabler);
}

/************************************************************************//**
 * MadDmaTransactionCreate
 *
 * DESCRIPTION:
 *    This is the shell function to call the simulation mode replacement for
 *    WdfDmaTransactionCreate.  See the called function in the simulator.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice     handle to the device ... needed to get the
 *                             function pointer.  Drat!
 *     @param[in]  hDmaEnabler handle to the parent dma enabler  
 *     @param[out] pDmaXaxn    pointer to the dma transaction handle
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadDmaTransactionCreate(WDFDEVICE hDevice, //We are violating the WDF function signature...I hate this!
	                             IN WDFDMAENABLER hDmaEnabler,
								 IN OPTIONAL WDF_OBJECT_ATTRIBUTES *pObjAttrs,
								 OUT WDFDMATRANSACTION *phDmaXaxn)

{
PFDO_DATA                  pFdoData = MadDeviceFdoGetData(hDevice);
PFN_DMA_TRANSACTION_CREATE pDmaXaxnCreate = pFdoData->pMadSimDmaFunxns->pDmaXaxnCreate;

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDmaTransactionCreate...SerialNo=%d, hDmaEnabler=%p, pDmaXaxnCreate=%p\n",
				pFdoData->SerialNo, hDmaEnabler, pDmaXaxnCreate);

    return pDmaXaxnCreate(hDmaEnabler, pObjAttrs, phDmaXaxn);
}

VOID MadDmaEnablerSetMaximumScatterGatherElements(IN WDFDMAENABLER hDmaEnabler,
                                                  IN size_t MaximumFragments)

{
	UNREFERENCED_PARAMETER(hDmaEnabler);
	UNREFERENCED_PARAMETER(MaximumFragments);

	return; //Nothing to do
}

/************************************************************************//**
 * MadDmaTransactionInitializeUsingRequest
 *
 * DESCRIPTION:
 *    This is the the shell function to call simulation mode replacement for 
 *    WdfDmaTransactionInitializeUsingRequest.
 *    See the called function in the simulator.
 *    
 * PARAMETERS: 
 *     @param[in]  hDmaXaxn            handle to the Dma transaction  
 *     @param[in]  hRequest            handle to the parent request   
 *     @param[in]  pEvtProgramDmaFunxn pointer to our dma processing function
 *                                     to be executed in XxxDmaXaxnExecute
 *     @param[in]  Direxn              indicates read or write
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS 
MadDmaTransactionInitializeUsingRequest(IN WDFDMATRANSACTION hDmaXaxn, IN WDFREQUEST hRequest,
										IN PFN_WDF_PROGRAM_DMA pEvtProgramDmaFunxn, IN WDF_DMA_DIRECTION Direxn)

{
WDFQUEUE                          hIoQueue = WdfRequestGetIoQueue(hRequest);
WDFDEVICE                         hDevice = WdfIoQueueGetDevice(hIoQueue);
PFDO_DATA                         pFdoData = MadDeviceFdoGetData(hDevice);
PFN_DMA_TRANSACTION_INIT_FROM_REQ pDmaXaxnInitFromReq = pFdoData->pMadSimDmaFunxns->pDmaXaxnInitFromReq;

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDmaTransactionInitializeUsingRequest...SerialNo=%d, hDmaXaxn=%p, hRequest=%p, Direxn=%d, pDmaXaxnInitFromReq=%p\n",
				pFdoData->SerialNo, hDmaXaxn, hRequest, (ULONG)Direxn, pDmaXaxnInitFromReq);
 
	ASSERT(pEvtProgramDmaFunxn == &MadEvtProgramDma); //Because we know what EVT_WDF_PROGRAM_DMA function in this driver we want called by 'KMDF'

    return pDmaXaxnInitFromReq(hDmaXaxn, hRequest, pEvtProgramDmaFunxn, Direxn);
}
//
/************************************************************************//**
 * MadDmaTransactionExecute
 *
 * DESCRIPTION:
 *    This is the shell function to call the simulation mode replacement for
 *     WdfDmaTransactionExecute.  See the called function in the simulator.
 *    
 * PARAMETERS: 
 *     @param[in]  hDmaEnabler handle to the parent dma enabler  
 *     @param[out] pDmaXaxn    pointer to the dma transaction handle
 *     
 * RETURNS:
 *    @return      NtStatus    indicates success or reason for the failure
 * 
 ***************************************************************************/
NTSTATUS MadDmaTransactionExecute(IN WDFDMATRANSACTION hDmaXaxn, /*IN OPTIONAL*/ PVOID Context)
{
PFDO_DATA                   pFdoData =  (PFDO_DATA)Context;
PFN_DMA_TRANSACTION_EXECUTE pDmaXaxnExecute = pFdoData->pMadSimDmaFunxns->pDmaXaxnExecute;

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadDmaTransactionExecute...SerialNo=%d, hDmaXaxn=%p, pDmaXaxnExecute=%p\n",
				pFdoData->SerialNo, hDmaXaxn, pDmaXaxnExecute);

	return pDmaXaxnExecute(hDmaXaxn, (PVOID)pFdoData);
}
//
/************************************************************************//**
 * MadDmaTransactionGetRequest
 *
 * DESCRIPTION:
 *    This is the shell function to call the simulation mode replacement for
 *    WdfDmaTransactionGetRequest.  See the called function in the simulator.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice     handle to the device ... needed to get the
 *                             function pointer.  Drat!
 *     @param[out] hDmaXaxn    handle to the dma transaction
 *     
 * RETURNS:
 *    @return      hRequest    handle to the parent/associated request
 * 
 ***************************************************************************/
WDFREQUEST MadDmaTransactionGetRequest(IN WDFDEVICE hDevice, //We are violating the WDF function signature...I hate this
	                                   IN WDFDMATRANSACTION hDmaXaxn)

{
PFDO_DATA                       pFdoData = MadDeviceFdoGetData(hDevice);
PFN_DMA_TRANSACTION_GET_REQUEST pDmaXaxnGetRequest = pFdoData->pMadSimDmaFunxns->pDmaXaxnGetRequest;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	            "MadDmaTransactionGetRequest...SerialNo=%d, hDmaXaxn=%p, pDmaXaxnGetRequest=%p\n",
				pFdoData->SerialNo, hDmaXaxn, pDmaXaxnGetRequest);

	return pDmaXaxnGetRequest(hDmaXaxn);
}
//
/************************************************************************//**
 * MadDmaTransactionGetBytesTransferred
 *
 * DESCRIPTION:
 *    This is the the shell function to call simulation mode replacement for 
 *     WdfDmaTransactionGetBytesTransferred.  
 *    See the called function in the simulator.
 *    
 * PARAMETERS: 
 *     @param[in]  hDevice     handle to the device ... needed to get the
 *                             function pointer.  Drat!
 *     @param[in]  hDmaXaxn    handle to the dma transaction  
 *     @param[out] pDmaXaxn    pointer to the dma transaction handle
 *     
 * RETURNS:
 *    @return      BytesXferd  nuber of bytes dma-transferred
 * 
 ***************************************************************************/

size_t  MadDmaTransactionGetBytesTransferred(WDFDEVICE hDevice, //We are violating the WDF function signature...I hate this
	                                         IN WDFDMATRANSACTION hDmaXaxn)

{
PFDO_DATA                           pFdoData   = MadDeviceFdoGetData(hDevice);
PFN_DMA_TRANSACTION_GET_BYTES_XFERD pDmaXaxnGetBytesXferd = pFdoData->pMadSimDmaFunxns->pDmaXaxnGetBytesXferd;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	            "MadDmaTransactionGetBytesTransferred...SerialNo=%d, hDmaXaxn=%p, pDmaXaxnGetBytesXferd=%p\n",
				pFdoData->SerialNo, hDmaXaxn, pDmaXaxnGetBytesXferd);

	return pDmaXaxnGetBytesXferd(hDmaXaxn);
}
//
/************************************************************************//**
* MadDmaTransactionDmaCompleted
*
* DESCRIPTION:
*    This is the the shell function to call simulation mode replacement for
*     WdfDmaTransactionGetBytesTransferred.
*    See the called function in the simulator.
*
* PARAMETERS:
*     @param[in]  hDevice     handle to the device ... needed to get the
*                             function pointer.  Drat!
*     @param[in]  hDmaXaxn    handle to the dma transaction
*     @param[out] pStatus     Status value through a pointer
*
* RETURNS:
*    @return      BOOLEAN     True/False
*
***************************************************************************/
BOOLEAN MadDmaTransactionDmaCompleted(WDFDEVICE hDevice, //We are violating the WDF function signature...I hate this
	                                  IN WDFDMATRANSACTION hDmaXaxn, OUT NTSTATUS *pStatus)

{
PFDO_DATA                         pFdoData = MadDeviceFdoGetData(hDevice);
PFN_DMA_TRANSACTION_DMA_COMPLETED pDmaXaxnCompleted = pFdoData->pMadSimDmaFunxns->pDmaXaxnCompleted;

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	            "MadDmaTransactionCompleted...SerialNo=%d, hDmaXaxn=%p, pDmaXaxnCompleted=%p\n",
				pFdoData->SerialNo, hDmaXaxn, pDmaXaxnCompleted);

	BOOLEAN bRC = pDmaXaxnCompleted(hDmaXaxn, pStatus);
	ASSERT(bRC); //Should always be true in the prototype

	return bRC;
}
#endif //MAD_KERNEL_WONT_CREATE_DMA_ADAPTER +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
#endif //_MAD_SIMULATION_MODE_  ...............................................................................

