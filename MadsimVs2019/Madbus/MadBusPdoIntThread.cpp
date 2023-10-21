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
/*  Module  NAME : MadBusPdoIntThread.cpp                                      */
/*                                                                             */
/*  DESCRIPTION  : Definition of the per-device thread to simulate interrupts  */
/*                 to the device driver; helper functions for buffer copying   */
/*                                                                             */
/*******************************************************************************/

#include "MadBus.h"
#ifdef WPP_TRACING
    #include "trace.h"
	#include "MadPdoIntThread.tmh"
#endif

//extern WDFDEVICE         ghDevFDO[];
//extern WDFSPINLOCK       ghPdoDevOidsLock;
extern ULONG             gNumAllocDevices;
extern BOOLEAN           gbAffinityOn;
extern DRIVER_DATA       gDriverData;

#ifdef ALLOC_PRAGMA
//
#endif

/**************************************************************************//**
 * MadPdoIntThread
 *
 * DESCRIPTION:
 *    This function is the one-per-PDO interrupt thread. It is on the lookout
 *    for a non-zero value in the MesgID to indicate that an interrupt should
 *    be issued.
 *    
 * PARAMETERS: 
 *     @param[in]  Context pointer to the PDO device extension for this device
 *     
 * RETURNS:
 *    @return      void     nothing returned
 * 
 *****************************************************************************/

VOID MadPdoIntThread(PVOID Context) 
{
static LARGE_INTEGER liZERO = {0, 0};
PPDO_DEVICE_DATA pPdoData = (PPDO_DEVICE_DATA)Context;
PMADREGS         pMadRegs = pPdoData->pMadRegs;
PMAD_SIMULATION_INT_PARMS  pMadSimIntParms = &pPdoData->MadSimIntParms;
KAFFINITY        CurrAffinity;
NTSTATUS         NtStatus;
KIRQL            PrevIRQL;    
KIRQL            BaseIRQL;  
BOOLEAN          bNewINT = FALSE;     
MADDEV_IO_TYPE   DevReqType;

    UNREFERENCED_PARAMETER(BaseIRQL);

    //Fetch & save our thread event object for the parent thread
    pPdoData->pIntThreadObj = PsGetCurrentThread();

    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	"MadBusPdoIntThread enter... pPdoData=%p SerialNo=%d pIntThreadObj=%p pMadRegs=%p\n",
		        pPdoData, pPdoData->SerialNo, 
		        pPdoData->pIntThreadObj, pMadRegs);
 
WaitOnPowerUp:;
    NtStatus = KeWaitForSingleObject(&pMadSimIntParms->evDevPowerUp,
                                     Executive, KernelMode, FALSE, NULL); //Wait until signalled
    KeClearEvent(&pMadSimIntParms->evDevPowerUp);

	if (gbAffinityOn) //Set the processor affinity to what is assigned ... (serialno)
        CurrAffinity = KeSetSystemAffinityThreadEx(pMadSimIntParms->u.WdfIntParms.IntAffinity);
    
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	"MadBusPdoIntThread activated... pPdoData=%p SerialNo=%d Affinity=x%X Irql=%d\n",
                pPdoData, pPdoData->SerialNo, 
		        (ULONG)pMadSimIntParms->u.WdfIntParms.IntAffinity, 
		        pMadSimIntParms->u.WdfIntParms.Irql);

	//Sanity check that parameters are passed correctly to & from the device driver
	ASSERT(pMadSimIntParms->u.WdfIntParms.Irql == pPdoData->Irql);
	ASSERT(pMadSimIntParms->u.WdfIntParms.IntAffinity == pPdoData->IntAffinity);

// Loop until the device power-down event.
    NtStatus = STATUS_TIMEOUT;
    while (NtStatus == STATUS_TIMEOUT)
        {
		bNewINT = (pMadRegs->MesgID != 0); 
		if (bNewINT)
            {
			//Let's not get preempted during the buffer copying 
			KeRaiseIrql((KIRQL)DISPATCH_LEVEL, &PrevIRQL); 

            DevReqType = MadDevAnalyzeReqType(pMadRegs);
            if (DevReqType > eNoIO)
				if ((pMadRegs->Status & MAD_STATUS_DEVICE_FAILURE_BIT) == 0)
					//Normal circumstances - do whatever data transfer is needed
                    MadBusPdoProcessDevIo(pPdoData, DevReqType);

			//Simulate interrupt conditions
            MAD_ACQUIRE_LOCK_RAISE_IRQL(pMadSimIntParms->u.WdfIntParms.hSpinlock, 
				                        pMadSimIntParms->u.WdfIntParms.Irql, &BaseIRQL);
            ASSERT(KeGetCurrentIrql() == pMadSimIntParms->u.WdfIntParms.Irql);
 
            //Call the function (device) driver's KMDF-ISR
            pMadSimIntParms->u.WdfIntParms.pMadEvtIsrFunxn(pMadSimIntParms->u.WdfIntParms.hInterrupt,
				                                           pMadRegs->MesgID);

    		MAD_LOWER_IRQL_RELEASE_LOCK(BaseIRQL, pMadSimIntParms->u.WdfIntParms.hSpinlock);
            SIMULATION_ASSERT(KeGetCurrentIrql() == BaseIRQL);

			KeLowerIrql(PrevIRQL);
    		} //end if New INT (non-zero MesgID)

        // Release our quantum but don't delay. Just get ready-listed right away
        KeDelayExecutionThread(KernelMode, FALSE, &liZERO);

        //Verify that our processor affinity persists
        #pragma warning(suppress: 4334)
        ASSERT(((KAFFINITY)(0x01 << (ULONG_PTR)KeGetCurrentProcessorNumber()) & pMadSimIntParms->u.WdfIntParms.IntAffinity) != 0);

        //Device powered down *?*
        NtStatus = KeWaitForSingleObject(&pMadSimIntParms->evDevPowerDown,
                                         Executive, KernelMode, FALSE, 
										 &liZERO);//Don't wait - just check
        if (NtStatus == STATUS_SUCCESS) 
            {
            TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				        "MadBusPdoIntThread de-activated... SerialNo=%d\n",
				        pPdoData->SerialNo);
            KeClearEvent(&pMadSimIntParms->evDevPowerDown);
            break; //Out of the while loop
			}

        ASSERT(NtStatus == STATUS_TIMEOUT);
        } //end while

	//Power down may also indicate device unplugged/ejected *?*
	NtStatus = KeWaitForSingleObject(&pPdoData->evIntThreadExit,
		                             Executive, KernelMode, FALSE, &liZERO);//Don't wait - just check
    if (NtStatus != STATUS_SUCCESS) //If the event is not signalled 
        {
        ASSERT(NtStatus == STATUS_TIMEOUT);
        goto WaitOnPowerUp; //Up at the beginning
        }

	//Unplugged! - we're leaving town
	//As we exit ... our thread event object identified above is signalled auto-magically
    KeClearEvent(&pPdoData->evIntThreadExit);
	
    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBusPdoIntThread exit...SerialNo=%d\n", pPdoData->SerialNo);

    return;
}

/**************************************************************************//**
 * MadPdoScsiIntThread
 *
 * DESCRIPTION:
 *    This function is the one-per-PDO interrupt thread. It is on the lookout
 *    for a non-zero value in the MesgID to indicate that an interrupt should
 *    be issued.
 *
 * PARAMETERS:
 *     @param[in]  Context pointer to the PDO device extension for this device
 *
 * RETURNS:
 *    @return      void     nothing returned
 *
*****************************************************************************/

#define IS_GO_BIT_SET(Control)  (ULONG)((Control & GO_BITS_MASK) != 0)

VOID MadPdoScsiIntThread(PVOID Context)
{
	static LARGE_INTEGER liZERO = { 0, 0 };
	//
	PPDO_DEVICE_DATA pPdoData = (PPDO_DEVICE_DATA)Context;
	PDRIVER_DATA     pDriverData = pPdoData->pFdoData->pDriverData;
	PMADREGS         pMadRegs = pPdoData->pMadRegs;
	//ULONG            BusSerNum  = pPdoData->pFdoData->SerialNo;
	ULONG            SerialNo = pPdoData->SerialNo;
	PMAD_SIMULATION_INT_PARMS  pMadSimIntParms = &pPdoData->MadSimIntParms;
	//
	KAFFINITY        CurrAffinity;
	NTSTATUS         NtStatus;
	KIRQL            PrevIRQL;
	//KIRQL            BaseIRQL;
	BOOLEAN          bNewINT = FALSE;
	BOOLEAN          bRC;
	MADDEV_IO_TYPE   DevReqType;
	ULONG            IntIdReg;
	ULONG            ControlReg;

	//Retrieve and save our thread event object for the parent thread
	//This event object will be signalled when we exit below
	pPdoData->pIntThreadObj = PsGetCurrentThread();

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	"MadBusPdoScsiIntThread enter... Bus#:Ser#=%d:%d pIntThreadObj=%p pMadRegs=%p\n",
		        pPdoData->pFdoData->SerialNo, SerialNo, 
		        pPdoData->pIntThreadObj, pMadRegs);

WaitOnPowerUp:;
	NtStatus = KeWaitForSingleObject(&pMadSimIntParms->evDevPowerUp,
		                             Executive, KernelMode, FALSE,
		                             NULL); //No time limit ...Wait until signalled
	KeClearEvent(&pMadSimIntParms->evDevPowerUp);

	if (pDriverData->bAffinityOn) //Set the processor affinity to what is assigned ... (serialno)
		CurrAffinity = KeSetSystemAffinityThreadEx(pPdoData->IntAffinity);

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBusPdoScsiIntThread activated... Bus#:Ser#=%d:%d Affinity=x%X Irql=%d\n",
		        pPdoData->pFdoData->SerialNo, pPdoData->SerialNo, 
		        (ULONG)pPdoData->IntAffinity, pPdoData->Irql);

	// Loop until the device power-down event.
	NtStatus = STATUS_TIMEOUT;
	while (NtStatus == STATUS_TIMEOUT) //Power-down event not signalled
	    {
		//Force this thread to service i/os without a trigger from the user layer
		pMadRegs->MesgID = IS_GO_BIT_SET(pMadRegs->Control);

		bNewINT = (pMadRegs->MesgID != 0);
		if (bNewINT) //We have a new interrupt
		    {
			TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			"MadBusPdoScsiIntThread... MesgID=x%X IntEnable=x%X Control=x%X HostAddr=x%llX DevAddr=x%llX DTBC=%ld BCDPP=x%llX\n",
				        pMadRegs->MesgID, pMadRegs->IntEnable, pMadRegs->Control,
				        pMadRegs->DmaChainItem0.HostAddr, 
				        pMadRegs->DmaChainItem0.DevLoclAddr, 
				        pMadRegs->DmaChainItem0.DTBC, 
				        pMadRegs->DmaChainItem0.CDPP);

			//Let's not get preempted during the buffer copying 
			KeRaiseIrql((KIRQL)DISPATCH_LEVEL, &PrevIRQL); 

			//Indicate the iotype without input from the user layer
			IntIdReg = pMadRegs->IntEnable;
			IntIdReg &= ~MAD_INT_STATUS_ALERT_BIT;
			pMadRegs->IntID = IntIdReg;

			ControlReg = pMadRegs->Control;
			ControlReg &= ~MAD_CONTROL_DMA_GO_BIT;
			pMadRegs->Control = ControlReg;

			DevReqType = MadDevAnalyzeReqType(pMadRegs);
			if (DevReqType > eNoIO)
				if ((pMadRegs->Status & MAD_STATUS_DEVICE_FAILURE_BIT) == 0)
					//Normal circumstances - do whatever data transfer is needed
					MadBusPdoProcessDevIo(pPdoData, DevReqType);

			//Simulate interrupt conditions
			pMadSimIntParms->u.StorIntParms.pAcquireLockFunxn(SerialNo, StartIoLock);

			// Call the Miniport's ISR
			bRC = 
			pMadSimIntParms->u.StorIntParms.pMadDiskISR(pPdoData->MadSimIntParms.u.StorIntParms.pDevXtensn);
			if (!bRC)
			    {
				TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				"MadBusPdoScsiIntThread: our device driver ISR did not recognize the interrupt...SerialNo=%d\n",
					        pPdoData->SerialNo);
			    }

			pMadSimIntParms->u.StorIntParms.pReleaseLockFunxn(SerialNo);
			KeLowerIrql(PrevIRQL);
		    } //end if New INT (non-zero MesgID)

	    // Release our quantum but don't delay. Just get ready-listed right away
		KeDelayExecutionThread(KernelMode, FALSE, &liZERO);

		//Device powered down *?*
		NtStatus = KeWaitForSingleObject(&pMadSimIntParms->evDevPowerDown,
			                             Executive, KernelMode, FALSE, 
										 &liZERO);//Don't wait - just check
		if (NtStatus == STATUS_SUCCESS)
		    {
			TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				        "MadBusPdoScsiIntThread... SerialNo=%d de-activated\n",
				        pPdoData->SerialNo);
			KeClearEvent(&pMadSimIntParms->evDevPowerDown);
			break; //Out of the while loop
		    }

		ASSERT(NtStatus == STATUS_TIMEOUT);
	    } //end while

//Power down may also indicate device unplugged/ejected *?*
	NtStatus = KeWaitForSingleObject(&pPdoData->evIntThreadExit,
		                             Executive, KernelMode, FALSE,
		                             &liZERO);//Don't wait - just check
	if (NtStatus != STATUS_SUCCESS) //The unplug event is not signalled 
	    {
		ASSERT(NtStatus == STATUS_TIMEOUT); //Else some error
		goto WaitOnPowerUp; //Up at the top 
	    }

	//=== Unplugged! - we're leaving town ===================================================
	//1st clear the event for next time
	KeClearEvent(&pPdoData->evIntThreadExit);

	//As we exit...our thread event object is signalled auto-magically as indicated above at the top
	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
		        "MadBusPdoScsiIntThread exit...BusNum=%d SerialNo=%d\n",
		        pPdoData->pFdoData->SerialNo, pPdoData->SerialNo);

	return;
}

 /************************************************************************//**
 * MadBusPdoProcessDevIo
 *
 * DESCRIPTION:
 *    This function does the buffer copying for the device simulation in 
 *    response to an interrupt indication from the simulator-ui.
 *    
 * PARAMETERS: 
 *     @param[in]  pFdoData     pointer to the framework device extension
 *     @param[in]  DevReqType the indicated type of i/o
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/

#define MBPIT_SET_REGS_NO_RESOURCE \
	    pMadRegs->IntID  |= MAD_INT_STATUS_ALERT_BIT;        \
		pMadRegs->Status |= MAD_STATUS_RESOURCE_ERROR_BIT;   \
        (*pWmiErrCount)++; /*Let's treat this as a bus data error to give the WMI domain something to chew on*/ \
		MadWriteEventLogMesg(pDriverObj, MADBUS_HARDWARE_FAULT, 0, 0, NULL); //Hard-coded message text. No payload
//
#define MBPIT_SET_REGS_EXTENT_EXCEEDED                   \
	    pMadRegs->IntID  |= MAD_INT_STATUS_ALERT_BIT;        \
		pMadRegs->Status |= MAD_STATUS_OVER_UNDER_ERR_BIT;

VOID  MadBusPdoProcessDevIo(PPDO_DEVICE_DATA pPdoData, MADDEV_IO_TYPE DevReqType)
{
PDRIVER_OBJECT pDriverObj = pPdoData->pFdoData->pDriverData->pDriverObj;
ULONG     SerialNo       = pPdoData->SerialNo;
PMADREGS  pMadRegs       = pPdoData->pMadRegs;
ULONG     IntIdReg       = pMadRegs->IntID;
ULONG     ControlReg     = pMadRegs->Control;
ULONG     DTBC           = pMadRegs->DmaChainItem0.DTBC;
PULONG    pWmiErrCount   = 
          &pPdoData->pFdoData->MadBusWmiData.InfoClassData.ErrorCount;
//
ULONG     ControlWork = ControlReg;
ULONG     CacheIndx;
ULONG     IoCount;  
ULONG     NextIndx = 0;
PVOID     pHostBufr;
PVOID     pXferLoc; 
PHYSICAL_ADDRESS        liHostAddr;

    UNREFERENCED_PARAMETER(SerialNo);
	UNREFERENCED_PARAMETER(IntIdReg);

    // If we are going to do buffer copying - the host-addr better be valid.
    // We are covering the case of having an interrupt not expected by a pending i/o,
    // or a real software error.
    // The device driver should zero-out the host-addr in the device registers before & between i/os
    if (DevReqType < eSgDmaRead)
		if ((DevReqType != eReadAlign) && (DevReqType != eWriteAlign))
             if (pMadRegs->DmaChainItem0.HostAddr == 0)
			     {//Report the error, don't transfer the data
				 pMadRegs->IntID  |= MAD_INT_STATUS_ALERT_BIT;
				 pMadRegs->Status |= MAD_STATUS_INVALID_IO_BIT;

				 TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				 "MadBusPdoIntThread:MadBusPdoProcessDevIo... SerialNo=%d invalid HostAddr=x%llX DevReqType=%d \n",
					 SerialNo, (ULONG)pMadRegs->DmaChainItem0.HostAddr, DevReqType);
		         return; 
			     }

    liHostAddr.QuadPart = pMadRegs->DmaChainItem0.HostAddr; //Can't cast: C2664
	ASSERT(liHostAddr.QuadPart > 0);

    switch (DevReqType)
        {
        case eBufrdRead:
            ASSERT(IntIdReg == MAD_INT_BUFRD_INPUT_BIT);

			//Determine the byte count
            IoCount = (pMadRegs->Status & MAD_STATUS_READ_COUNT_MASK); //Isolate N bits as a number
            IoCount = (IoCount >> MAD_STATUS_READ_COUNT_SHIFT);        //Shift to low-order bits
            IoCount++; //0..N-1 --> 1..N
            IoCount = (IoCount * MAD_BYTE_COUNT_MULT); //Apply the multiplier
			
			NextIndx = pMadRegs->ByteIndxRd + IoCount;
			if (NextIndx >= pPdoData->pFdoData->pDriverData->MadDataXtent) //Report the error, don't transfer the data
			    {
				MBPIT_SET_REGS_EXTENT_EXCEEDED;
				TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				"MadBusPdoIntThread:MadBusPdoProcessDevIo i/o request exceeds device extent... SerialNo=%d DevReqType=%d\n",
					        SerialNo, DevReqType);
			    }
			else
			    {
                pHostBufr = MmMapIoSpaceEx(liHostAddr, IoCount, MAD_MEM_MAPIO_FLAGS);
				if (pHostBufr == NULL) //Driver-verifier causes this by intent
				    {
					MBPIT_SET_REGS_NO_RESOURCE;
					TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, 
		            "MadBusPdoIntThread:MadBusPdoProcessDevIo insufficient system resources...  SerialNo=%d DevReqType=%d\n",
						        SerialNo, DevReqType);
				    }
				else
				    {
                    pXferLoc = (PVOID)((ULONG_PTR)pPdoData->pDeviceData + pMadRegs->ByteIndxRd);
                    RtlCopyMemory(pHostBufr, pXferLoc, IoCount);
			        MmUnmapIoSpace(pHostBufr, IoCount);
				    
                    pMadRegs->ByteIndxRd += IoCount;
                    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				    "MadBusPdoIntThread:MadBusPdoProcessDevIo... SerialNo=%d %d bytes transferred from the device ByteDX=%d\n",
						        SerialNo, IoCount, (NextIndx-IoCount));
			        }
			    }
            break;

        case eBufrdWrite:
            ASSERT(IntIdReg == MAD_INT_BUFRD_OUTPUT_BIT);

			//Determine the byte count
            IoCount = (pMadRegs->Status & MAD_STATUS_WRITE_MASK); //Isolate N bits as a number
            IoCount = (IoCount >> MAD_STATUS_WRITE_SHIFT);        //Shift to low-order bits
            IoCount++; //0..N-1 --> 1..N
            IoCount = (IoCount * MAD_BYTE_COUNT_MULT); //Apply the multiplier
			
			NextIndx = pMadRegs->ByteIndxWr + IoCount;
			if (NextIndx >= pPdoData->pFdoData->pDriverData->MadDataXtent) //Report the error, don't transfer the data
				{MBPIT_SET_REGS_EXTENT_EXCEEDED; TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, 
		         "MadBusPdoIntThread:MadBusPdoProcessDevIo i/o request exceeds device extent... SerialNo=%d DevReqType=%d \n",
					                                         SerialNo, DevReqType);}
			else
			    {
                pHostBufr = MmMapIoSpaceEx(liHostAddr, IoCount, MAD_MEM_MAPIO_FLAGS);
				if (pHostBufr == NULL) //Driver-verifier causes this by intent
				    {
					MBPIT_SET_REGS_NO_RESOURCE;
					TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, 
		            "MadBusPdoIntThread:MadBusPdoProcessDevIo insufficient system resources... SerialNo=%d DevReqType=%d\n",
						        SerialNo, DevReqType);
				    }
				else
				    {
			        pXferLoc = (PVOID)((ULONG_PTR)pPdoData->pDeviceData + pMadRegs->ByteIndxWr);
                    RtlCopyMemory(pXferLoc, pHostBufr, IoCount);
			        MmUnmapIoSpace(pHostBufr, IoCount);
			        
                    pMadRegs->ByteIndxWr += IoCount;
                    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				    "MadBusPdoIntThread:MadBusPdoProcessDevIo... SerialNo=%d %d bytes transferred to the device, ByteDX=%d\n",
						        SerialNo, IoCount, (NextIndx-IoCount));
			        }
			    }
            break;

        case eCachedRead:
            ASSERT(IntIdReg == MAD_INT_BUFRD_INPUT_BIT);
            ASSERT((ControlReg & MAD_CONTROL_CACHE_XFER_BIT) == MAD_CONTROL_CACHE_XFER_BIT);
            ASSERT(pMadRegs->PioCacheReadLen == MAD_CACHE_SIZE_BYTES);
            
            pHostBufr = MmMapIoSpaceEx(liHostAddr, MAD_CACHE_SIZE_BYTES, MAD_MEM_MAPIO_FLAGS);
			if (pHostBufr == NULL) //Driver-verifier causes this 
			    {
				MBPIT_SET_REGS_NO_RESOURCE;
				TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, 
		        "MadBusPdoIntThread:MadBusPdoProcessDevIo insufficient system resources... SerialNo=%d DevReqType=%d\n",
					        SerialNo, DevReqType);
			    }
			else
			    {
                RtlCopyMemory(pHostBufr, pPdoData->pReadCache, MAD_CACHE_SIZE_BYTES);
   		        MmUnmapIoSpace(pHostBufr, MAD_CACHE_SIZE_BYTES);

                //Reposition and reload the read cache
                pMadRegs->CacheIndxRd += MAD_CACHE_NUM_SECTORS;
                pXferLoc = (PVOID)((ULONG_PTR)pPdoData->pDeviceData + 
                                   (pMadRegs->CacheIndxRd * MAD_SECTOR_SIZE));
                RtlCopyMemory(pPdoData->pReadCache, pXferLoc, MAD_CACHE_SIZE_BYTES);
                TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				"MadBusPdoIntThread:MadBusPdoProcessDevIo... SerialNo=%d %d bytes xferred from the read cache to the user buffer \n",
					        SerialNo, MAD_CACHE_SIZE_BYTES);
			    }
            break;

        case eCachedWrite:
            ASSERT(IntIdReg == MAD_INT_BUFRD_OUTPUT_BIT);
            ASSERT((ControlReg & MAD_CONTROL_CACHE_XFER_BIT) == MAD_CONTROL_CACHE_XFER_BIT);
            ASSERT(pMadRegs->PioCacheWriteLen == 0);
            
            pHostBufr = MmMapIoSpaceEx(liHostAddr, MAD_CACHE_SIZE_BYTES, MAD_MEM_MAPIO_FLAGS);
			if (pHostBufr == NULL) //Driver-verifier causes this 
			    {
				MBPIT_SET_REGS_NO_RESOURCE;
				TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, 
		        "MadBusPdoIntThread:MadBusPdoProcessDevIo insufficient system resources... SerialNo=%d DevReqType=%d\n",
					        SerialNo, DevReqType);
			}
			else
			    {
                RtlCopyMemory(pPdoData->pWriteCache, pHostBufr, MAD_CACHE_SIZE_BYTES);
			    MmUnmapIoSpace(pHostBufr, MAD_CACHE_SIZE_BYTES);

                //Flush / write-through
                pXferLoc = (PVOID)((ULONG_PTR)pPdoData->pDeviceData + 
                                   (pMadRegs->CacheIndxWr * MAD_SECTOR_SIZE));
                RtlCopyMemory(pXferLoc, pPdoData->pWriteCache, MAD_CACHE_SIZE_BYTES);
                 
                pMadRegs->CacheIndxWr += MAD_CACHE_NUM_SECTORS;
                ASSERT(pMadRegs->CacheIndxWr < MAD_DEVICE_MAX_SECTORS);
                pMadRegs->PioCacheWriteLen = 0; //After a flush of the write cache
                TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				"MadBusPdoIntThread:MadBusPdoProcessDevIo... SerialNo=%d %d bytes transferred to the write cache from user buffer \n",
					        SerialNo, MAD_CACHE_SIZE_BYTES);
			    }
            break;

        case eReadAlign:
            ASSERT(IntIdReg == MAD_INT_ALIGN_INPUT_BIT);
            ControlWork &= MAD_CONTROL_IO_OFFSET_MASK;
            CacheIndx    = (ControlWork >> MAD_CONTROL_IO_OFFSET_SHIFT);
            CacheIndx    = (CacheIndx * MAD_CACHE_ALIGN_MULTIPLE);
			if (CacheIndx >= MAD_DEVICE_MAX_SECTORS) //Report the error, don't transfer the data
				{MBPIT_SET_REGS_EXTENT_EXCEEDED; TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, 
		         "MadBusPdoIntThread:MadBusPdoProcessDevIo i/o request exceeds device extent... SerialNo=%d DevReqType=%d\n",
					                                         SerialNo, DevReqType);}
			else
			    {
                pMadRegs->CacheIndxRd = CacheIndx; 
				pXferLoc = (PVOID)((ULONG_PTR)pPdoData->pDeviceData + 
                                   (pMadRegs->CacheIndxRd * MAD_SECTOR_SIZE));

                //Read device data into the cache
                RtlCopyMemory(pPdoData->pReadCache, pXferLoc, MAD_CACHE_SIZE_BYTES);
                pMadRegs->PioCacheReadLen  = MAD_CACHE_SIZE_BYTES;
                TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				"MadBusPdoIntThread:MadBusPdoProcessDevIo... SerialNo=%d read cache aligned to sector %d; %d bytes loaded into the read cache - \n",
					        SerialNo, CacheIndx, MAD_CACHE_SIZE_BYTES);
			    }
            break;

        case eWriteAlign:
            ASSERT(IntIdReg == MAD_INT_ALIGN_OUTPUT_BIT);
			if (pMadRegs->PioCacheWriteLen > 0)
			    { //Brackets around null stmt in the case of Free_build & No WPP_TRACING(KdPrint)
				TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				"MadBusPdoIntThread:MadBusPdoProcessDevIo...write cache (re)alignment before a cache flush - SerialNo=%d\n",
                            SerialNo);
			    }

			ControlWork &= MAD_CONTROL_IO_OFFSET_MASK;
            CacheIndx    = (ControlWork >> MAD_CONTROL_IO_OFFSET_SHIFT);
            CacheIndx    = (CacheIndx * MAD_CACHE_ALIGN_MULTIPLE);
			if (CacheIndx >= MAD_DEVICE_MAX_SECTORS) //Report the error, don't transfer the data
				{MBPIT_SET_REGS_EXTENT_EXCEEDED; TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, 
		         "MadBusPdoIntThread:MadBusPdoProcessDevIo i/o request exceeds device extent... SerialNo=%d DevReqType=%d\n",
					                                          SerialNo, DevReqType);}
			else
			    {
                pMadRegs->CacheIndxWr = CacheIndx; 
 			    TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
					        "MadBusPdoIntThread:MadBusPdoProcessDevIo... SerialNo=%d write cache aligned to sector %d \n",
					        SerialNo, CacheIndx);
				}
            break;

        case eDmaRead:
            ASSERT(IntIdReg == MAD_INT_DMA_INPUT_BIT);
            ASSERT(pMadRegs->DmaChainItem0.CDPP == MAD_DMA_CDPP_END);
            //
            pHostBufr = MmMapIoSpaceEx(liHostAddr, DTBC, MAD_MEM_MAPIO_FLAGS);
			if (pHostBufr == NULL) //Driver-verifier causes this 
			    {
				MBPIT_SET_REGS_NO_RESOURCE; 
				TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, 
		        "MadBusPdoIntThread:MadBusPdoProcessDevIo insufficient system resources... SerialNo=%d DevReqType=%d\n",
					        SerialNo, DevReqType);
    			}
			else
			    {
				pXferLoc = (PVOID)((ULONG_PTR)pPdoData->pDeviceData + (ULONG)pMadRegs->DmaChainItem0.DevLoclAddr);
                RtlCopyMemory(pHostBufr, pXferLoc, DTBC); //Host-->Device
   		        MmUnmapIoSpace(pHostBufr, DTBC);

                TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				"MadBusPdoIntThread:MadBusPdoProcessDevIo... SerialNo=%d dma read %d bytes transferred\n",
					        SerialNo, DTBC);
                }
            break;

	   case eDmaWrite:
            ASSERT(IntIdReg == MAD_INT_DMA_OUTPUT_BIT);
            ASSERT(pMadRegs->DmaChainItem0.CDPP == MAD_DMA_CDPP_END);
            //
            pHostBufr = MmMapIoSpaceEx(liHostAddr, DTBC, MAD_MEM_MAPIO_FLAGS);
			if (pHostBufr == NULL) //Driver-verifier causes this 
			    {
				MBPIT_SET_REGS_NO_RESOURCE;
				TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, 
		        "MadBusPdoIntThread:MadBusPdoProcessDevIo insufficient system resources... SerialNo=%d DevReqType=%d\n",
					        SerialNo, DevReqType);
   			    }
			else
			    {
		        pXferLoc = (PVOID)((ULONG_PTR)pPdoData->pDeviceData + 
					               (ULONG)pMadRegs->DmaChainItem0.DevLoclAddr);
                RtlCopyMemory(pXferLoc, pHostBufr, DTBC); //Device-->Host
                MmUnmapIoSpace(pHostBufr, DTBC);

                TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				"MadBusPdoIntThread:MadBusPdoProcessDevIo... SerialNo=%d dma write %d bytes transferred\n",
					        SerialNo, DTBC);
                }
            break;

        case eSgDmaRead:
            ASSERT(IntIdReg == MAD_INT_DMA_INPUT_BIT);
            ASSERT(pMadRegs->Control & MAD_CONTROL_CHAINED_DMA_BIT);
            ASSERT((pMadRegs->DmaChainItem0.CDPP & ~MAD_DMA_CDPP_END) != 0);
            
            MadBusPdoProcessDevSgDma(pPdoData, eSgDmaRead);
            break;

        case eSgDmaWrite:
            ASSERT(IntIdReg == MAD_INT_DMA_OUTPUT_BIT);
            ASSERT(pMadRegs->Control & MAD_CONTROL_CHAINED_DMA_BIT);
            ASSERT((pMadRegs->DmaChainItem0.CDPP  & ~MAD_DMA_CDPP_END) != 0);
            
            MadBusPdoProcessDevSgDma(pPdoData, eSgDmaWrite);
		    break;

        default:
			TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
			"MadBusPdoIntThread:MadBusPdoProcessDevIo... SerialNo=%d unrecognized IoType=%d \n",
				        SerialNo, DevReqType);
        } //end switch

    return;
}

/************************************************************************//**
 * MadBusPdoProcessDevSgDma
 *
 * DESCRIPTION:
 *    This function does the buffer copying for the device simulation 
 *    specific to Scatter-Gather DMA. It is a helper for the function above.  
 *    This is where the dma-controller processes the hardware sg-list
 *    
 * PARAMETERS: 
 *     @param[in]  pPdoData    pointer to PDO device context
 *     @param[in]  DevReqType  the indicated type of i/o
 *     
 * RETURNS:
 *    @return      void        nothing returned
 * 
 ***************************************************************************/
VOID MadBusPdoProcessDevSgDma(PPDO_DEVICE_DATA pPdoData, MADDEV_IO_TYPE DevReqType)
{
PDRIVER_OBJECT pDriverObj = pPdoData->pFdoData->pDriverData->pDriverObj;
PMADREGS       pMadRegs  = pPdoData->pMadRegs;
ULONG          SerialNo  = pPdoData->SerialNo;
ULONG64        CDPP      = pMadRegs->DmaChainItem0.CDPP;
PULONG         pWmiErrCount = 
               &pPdoData->pFdoData->MadBusWmiData.InfoClassData.ErrorCount;
//
PHYSICAL_ADDRESS        liHostAddr;
PHYSICAL_ADDRESS        liCDPP;
PVOID                   pHostSgBufr;
PVOID                   pXferLoc;
ULONG                   DTBC;
ULONG                   TotlXferLen = 0;
ULONG                   ChainLen = 0;
PMAD_DMA_CHAIN_ELEMENT  pDmaPktPntr;

    UNREFERENCED_PARAMETER(DevReqType);
	UNREFERENCED_PARAMETER(SerialNo);

    while (CDPP != MAD_DMA_CDPP_END)
        {
        liCDPP.QuadPart = CDPP; //Can't cast: C2664
        pDmaPktPntr = (PMAD_DMA_CHAIN_ELEMENT)MmMapIoSpaceEx(liCDPP,
			                                               sizeof(MAD_DMA_CHAIN_ELEMENT),
														   MAD_MEM_MAPIO_FLAGS);
		if (pDmaPktPntr == NULL) //Driver-verifier causes this by intent
		    {
			MBPIT_SET_REGS_NO_RESOURCE;
			TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, 
		    "MadBusPdoIntThread:MadBusPdoProcessDevIo... insufficient system resources DevReqType=%d SerialNo=%d\n",
		    			DevReqType, SerialNo); 
			break; // out of while loop
		    }

        DTBC = pDmaPktPntr->DTBC;
        liHostAddr.QuadPart = pDmaPktPntr->HostAddr; //Can't cast: C2664
        pHostSgBufr = MmMapIoSpaceEx(liHostAddr, DTBC, MAD_MEM_MAPIO_FLAGS);
 		if (pHostSgBufr == NULL) //Driver-verifier causes this 
		    {
			MBPIT_SET_REGS_NO_RESOURCE;
			MmUnmapIoSpace(pDmaPktPntr, sizeof(MAD_DMA_CHAIN_ELEMENT));
			TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, 
		    "MadBusPdoIntThread:MadBusPdoProcessDevIo... insufficient system resources DevReqType=%d SerialNo=%d\n",
		    			DevReqType, SerialNo); 
			break; // out of while loop
			}
		else
			{
            pXferLoc = 
		    (PVOID)((ULONG_PTR)pPdoData->pDeviceData + (ULONG)pDmaPktPntr->DevLoclAddr);

            if (pDmaPktPntr->DmaCntl & MAD_DMA_CNTL_H2D) //Host-->Device
                { //Write
                ASSERT(DevReqType == eSgDmaWrite);
                RtlCopyMemory(pXferLoc, pHostSgBufr, DTBC); 
                }
            else 
                { //Read
                ASSERT(DevReqType == eSgDmaRead);
                RtlCopyMemory(pHostSgBufr, pXferLoc, DTBC); 
                }
			
            TotlXferLen += DTBC;
		    ChainLen++;
            MmUnmapIoSpace(pHostSgBufr, DTBC);
            CDPP = pDmaPktPntr->CDPP;        //Get next chain element 
            MmUnmapIoSpace(pDmaPktPntr, sizeof(MAD_DMA_CHAIN_ELEMENT));
		    }
        } //end while

    pMadRegs->DmaChainItem0.DTBC = TotlXferLen; //Store the final total in the base DTBC
	
	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
				"MadBusPdoIntThread:MadBusPdoProcessDevIo(SgDMA)...ChainLen=%d TotlXferLen=%d IoType=%d\n",
				ChainLen, TotlXferLen, DevReqType);

    return;
}

inline PPDO_DEVICE_DATA MadBusGetPdoContextFromFdoAndSernum(ULONG BusNum, ULONG Sernum)
{
	WDFDEVICE         hDevFDO = gDriverData.hDevFDO[BusNum];
	PFDO_DEVICE_DATA  pFdoData = MadBusFdoGetData(hDevFDO);
	WDFDEVICE         hDevPDO = pFdoData->PdoDevOidsList[Sernum].hPhysDevice;
	PPDO_DEVICE_DATA  pPdoData = MadBusPdoGetData(hDevPDO);

	return pPdoData;
}

//Set the power down event for this device - called by the miniport
VOID MadBusPdoSetPowerDown(ULONG SerialNo)
{
PPDO_DEVICE_DATA  pPdoData = 
                 MadBusGetPdoContextFromFdoAndSernum(eScsiDiskBus, //We know it's the Scsi disk bus
	                                                 SerialNo);

	ULONG Signald = 
	KeSetEvent(&pPdoData->MadSimIntParms.evDevPowerDown,  (KPRIORITY)0, FALSE);
	UNREFERENCED_PARAMETER(Signald);

	return;
}

//Set the power up event for this device - called by the miniport
VOID MadBusPdoSetPowerUp(ULONG SerialNo)
{
PPDO_DEVICE_DATA  pPdoData = 
                  MadBusGetPdoContextFromFdoAndSernum(eScsiDiskBus, //We know it's the Scsi disk bus
					                                  SerialNo);
    ASSERT(pPdoData != NULL);
	ASSERT(pPdoData->SerialNo == SerialNo);
	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	         	"MadBusPdoSetPowerUp... SerialNo=%d\n", pPdoData->SerialNo);

    ULONG Signald = 
	KeSetEvent(&pPdoData->MadSimIntParms.evDevPowerUp, (KPRIORITY)0, FALSE);
	UNREFERENCED_PARAMETER(Signald);

	return;
}

/************************************************************************//**
 * MadBusPdoGetLocString
 *
 * DESCRIPTION:
 *    This function returns a PnP device location string.
 *    It is a DDI Query interface function
 *
 * PARAMETERS:
 *     @param[in]  Context    the PDO Context
 *     @param[in]  Level      pointer to the power level
 *
 * RETURNS:
 *    @return      NSTATUS    indicates success or failure
 *
 ***************************************************************************/
NTSTATUS MadBusPdoGetLocString(_Inout_  PVOID Context, 
	                           _Out_ PWCHAR* ppPnpLocStrings)

{
	static WCHAR Digits[] = L"0123456789";
	static WCHAR PnpLocString[] = L"PCI(0d00)\0\0"; //This is the format by convention (XXYY) = device_num:function_num
	//
	PPDO_DEVICE_DATA pPdoData = (PPDO_DEVICE_DATA)Context;

	//PnpMngr wants this data in a pool & will delete the pool for us
	PVOID pPnpLocStr = 
	ExAllocatePoolWithTag(PagedPool, sizeof(WCHAR) * 20, MADBUS_POOL_TAG);
	ASSERT(pPnpLocStr != NULL);

	PnpLocString[5] = Digits[pPdoData->SerialNo];
	RtlFillMemory(pPnpLocStr, sizeof(WCHAR) * 20, 0x00);
	RtlCopyMemory(pPnpLocStr, PnpLocString, sizeof(WCHAR) * 11);
	*ppPnpLocStrings = (PWCHAR)pPnpLocStr;

	TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO,
	"MadBusPdoGetLocString... SerialNo=%d pLocStr=%p assigned pnp_loc_str=%S\n",
		        pPdoData->SerialNo, *ppPnpLocStrings, *ppPnpLocStrings);

	return STATUS_SUCCESS;
}
