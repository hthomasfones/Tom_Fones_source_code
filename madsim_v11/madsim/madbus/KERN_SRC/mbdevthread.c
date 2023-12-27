/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : MAD Device Simulation Framework                             */
/*  COPYRIGHT    : (c) 2021 HTF Consulting                                     */
/*                                                                             */
/* This source code is provided by Dual/GPL license to the Linux open source   */ 
/* community                                                                   */
/*                                                                             */ 
/*******************************************************************************/
/*                                                                             */
/*  Exe files   : madbus.ko                                                    */ 
/*                                                                             */
/*  Module NAME : mbdevthread.c                                                */
/*                                                                             */
/*  DESCRIPTION : Function definitions for device threads of the MAD bus driver*/
/*                                                                             */
/*  MODULE_AUTHOR("HTF Consulting");                                           */
/*  MODULE_LICENSE("Dual/GPL");                                                */
/*                                                                             */
/* The source code in this file can be freely used, adapted, and redistributed */
/* in source or binary form, so long as an acknowledgment appears in derived   */
/* source files.  The citation should state that the source code comes from a  */
/* a set of source files developed by HTF Consulting                           */
/* http://www.htfconsulting.com                                                */
/*                                                                             */
/* No warranty is attached.                                                    */
/* HTF Consulting assumes no responsibility for errors or fitness of use       */
/*                                                                             */
/*                                                                             */
/* $Id: mbdevthread.c, v 1.0 2021/01/01 00:00:00 htf $                         */
/*                                                                             */
/*******************************************************************************/

//
#define _SIM_DRIVER_
#include "madbus.h"

static IoType mbdt_get_programmed_iotype(PMADREGS pMadDevRegs, u32* iotag);
static void mbdt_process_io(PMADBUSOBJ pmadbusobj, IoType iotype);

//This function creates a one-per-device thread and binds it to a processor
//
int madbus_create_thread(PMADBUSOBJ pmadbusobj)
{
static char ThreadName[] = "mbdthreadX";
static ULONG TNDX = 9;    //.........^
static char DevNumStr[] = DEVNUMSTR;
//
int rc = 0;
int pstate = 0;
struct task_struct *pThread = NULL;
int devnum;
int num_cpus = 0;

    ASSERT((int)(pmadbusobj != NULL));
    devnum = pmadbusobj->devnum;

    num_cpus = num_online_cpus();
    if ((devnum < 1) || (devnum >= num_cpus)) 
        {
    	rc = -EBADSLT; //No such cpu#-bus_slot *!?!*
    	PERR("madbus:madbus_create_thread error... invalid cpu#(%d) rc=%d\n",
    		 devnum, rc);
        return rc;
        }

    pmadbusobj->pThread = NULL;
    ThreadName[TNDX] = DevNumStr[pmadbusobj->devnum];
    pThread = kthread_create(madbus_dev_thread, (void *)pmadbusobj, ThreadName);
    if ((pThread == NULL) || (IS_ERR(pThread)))
        {
    	rc = -ESRCH; //No such process *!?!*
    	PERR("madbus:madbus_create_thread failure... error=%ld rc=%d\n",
    		 PTR_ERR(pThread), rc);
        return rc;
        }

    kthread_bind(pThread, devnum);
    pstate = wake_up_process(pThread);

    //Release quantum so that threads initialize in start order
    schedule();

    pmadbusobj->pThread = pThread;
    PINFO("madbus_create_thread... dev#=%d pmadbusobj=%px pThread=%px name:%s\n",
    	  (int)pmadbusobj->devnum, pmadbusobj, pThread, ThreadName);

    return 0;
}

//The device-implementing thread itself
//This thread will look for a change in the state of the 'hardware'
//and do whatever processing is called for to complete a device i/o until shutdown
//
int madbus_dev_thread(void* pvoid)
{
PMADBUSOBJ pmadbusobj = (PMADBUSOBJ)pvoid;
PMADREGS   pmaddevice = pmadbusobj->pmaddevice;
//
int cur_cpu = -1;
IoType iotype = eNOP;
int rc = -EADDRNOTAVAIL;

    ASSERT((int)(pmaddevice != NULL));
    cur_cpu = get_cpu();

    PINFO("madbus_dev_thread... dev#=%d cpu=%d pThread=%px madbusobj=%px maddev=%px\n",
    	  (int)pmadbusobj->devnum, cur_cpu, 
          (void *)current, (void *)pmadbusobj, (void *)pmaddevice);
 
    //We want to run specifically on cpu# = dev# and leave cpu 0 alone
    //This test only works because kthread_bind(s) to proc_0 when we ask for cpu > (numcpus - 1)
    if (cur_cpu != pmadbusobj->devnum)
        {
        //pmadbusobj->pThread = NULL;
        PWARN("MBDT... cur_cpu(%d) != dev#(%d) rc=%d\n", 
              cur_cpu, (int)pmadbusobj->devnum, rc);
        return rc; 
        }

    //pmadbusobj->pThread = current;
    //Initialize the device (any non-zero values) before processing begins
    pmaddevice->Status |= MAD_STATUS_CACHE_INIT_MASK;

    //Look for the device mesg-id register to become non-zero
    //set by the simulator-ui through direct wiring by mmap
    //
    while (1)
        {
        iotype =  mbdt_get_programmed_iotype(pmaddevice, &pmadbusobj->dev_iotag);
     	if (iotype != eNOP) //We are ready to complete an i/o
    	    {
            //pmaddevice->MesgID = mesgid; ////////////////////////////////////////
    	    mbdt_process_io(pmadbusobj, iotype);

            //Check for kthread_stop after some work - before releasing our quantum
    		if (kthread_should_stop()) 
                {break;}
    	    }

        //Release our quantum 
    	schedule(); 

        //Check for kthread_stop after reactivating
       	if (kthread_should_stop()) 
            {break;}

        //Let's confirm that we maintain processor affinity after releasing our quantum
       	cur_cpu = get_cpu();
       	if (cur_cpu != pmadbusobj->devnum) //Should not happen after initial check above
            {PWARN("MBDT... cur_cpu(%d) != dev#=%d\n",
                   cur_cpu, (int)pmadbusobj->devnum);}
        }

    PINFO("madbus_dev_thread exiting... dev#=%d pmadbusobj=%px\n",
    	 (int)pmadbusobj->devnum, pmadbusobj);

    //Falling through and returning 0 is sufficient
    //exit_kthread(pmadbusobj->pThread);

    return 0;
}

static inline IoType mbdt_get_programmed_iotype_worker(PMADREGS pMadDevRegs)
{
u32 IntEnable = pMadDevRegs->IntEnable;

    if ((IntEnable & MAD_INT_DMA_OUTPUT_BIT) != 0)
        {return eDmaWrite;}    

    if ((IntEnable & MAD_INT_DMA_INPUT_BIT) != 0)
        {return eDmaRead;}    

    if ((IntEnable & MAD_INT_ALIGN_OUTPUT_BIT) != 0)
        {return eAlignWrCache;}    

    if ((IntEnable & MAD_INT_ALIGN_INPUT_BIT) != 0)
        {return eAlignRdCache;}    

    if ((IntEnable & MAD_INT_BUFRD_OUTPUT_BIT) != 0)
        {
        if ((pMadDevRegs->Control & MAD_CONTROL_CACHE_XFER_BIT) != 0)
            {return eFlushWrCache;} 
        else
            {return eBufrdWr;}    
        }     

    if ((IntEnable & MAD_INT_BUFRD_INPUT_BIT) != 0)
        {
        if ((pMadDevRegs->Control & MAD_CONTROL_CACHE_XFER_BIT) != 0)
            {return eLoadRdCache;} 
        else
            {return eBufrdRd;}    
        }     

    return eNOP;
}

//Detemine what Io has been programmed and  needs to be completed in this device thread
static IoType mbdt_get_programmed_iotype(PMADREGS pMadDevRegs, u32* pIotag)
{
static U32 MAD_GO_BITS = (MAD_CONTROL_DMA_GO_BIT | MAD_CONTROL_BUFRD_GO_BIT);
//
IoType iotype = eNOP;
u32 IntEnable = pMadDevRegs->IntEnable;

    if (IntEnable == 0) //No i/o is programmed
        {return iotype;}

    if ((pMadDevRegs->Control & MAD_GO_BITS) == 0) //No i/o is programmed
        {return iotype;}

    if (pMadDevRegs->IntID != 0) //an i/o is in progress
        {return iotype;}
        
    if (*pIotag != pMadDevRegs->IoTag)
        {
        PERR("mbdt_get_progammed_iotype... dev#=%ld sequence error; expected=x%lX iotag=x%lX\n",
             (long int)pMadDevRegs->Devnum, (long int)*pIotag, (long int)pMadDevRegs->IoTag); 
        BUG_ON(*pIotag != pMadDevRegs->IoTag);
        } /* */

    iotype = mbdt_get_programmed_iotype_worker(pMadDevRegs);
    if (iotype != eNOP) //We have a valid programmed-io to complete
        {(*pIotag)++;}

    return iotype;
}            

static void mbdt_process_io(PMADBUSOBJ pmadbusobj, IoType iotype)
{
	PMADREGS pmaddev = pmadbusobj->pmaddevice;
    u32 msidx = 0; //default to legacy-int
    U8 bMSI = pmadbusobj->pcidev.msi_cap;

    if (bMSI)
        {  
        // Our dumb device assigns msidx's in round-robin fashion
        msidx = (pmaddev->IoTag % 8) + 1; //0..7 --> 1..8  
        }

    //{msidx = pmaddevice->MesgID - 1;}
	PINFO("mbdthread:mbdt_process_io... dev#=%ld iotype=%d IntEnable=x%lX msidx=%d\n",
	   	  (long int)pmadbusobj->devnum, iotype,
          (long unsigned int)pmaddev->IntEnable, (int)msidx);

    ASSERT((int)(pmaddev->IntEnable != 0));

    switch (iotype)
        {
        case eBufrdRd: 
            mbdt_process_bufrd_io(pmadbusobj, false, msidx);
            break;

        case eBufrdWr: 
            mbdt_process_bufrd_io(pmadbusobj, true, msidx);
            break;

        case eLoadRdCache: 
            mbdt_process_cache_io(pmadbusobj, false, msidx);
            break;

        case eFlushWrCache:
            mbdt_process_cache_io(pmadbusobj, true, msidx);
            break;

        case eAlignRdCache: 
            mbdt_process_align_cache(pmadbusobj, false, msidx);
            break;

        case eAlignWrCache: 
            mbdt_process_align_cache(pmadbusobj, true, msidx);
            break;

        case eDmaRead: 
            mbdt_process_dma_io(pmadbusobj, false, msidx);
            break;

        case eDmaWrite: 
            mbdt_process_dma_io(pmadbusobj, true, msidx);
            break;

        default:
            PWARN("madbus_dev_thread:mbdt_complete_simulated_io... dev#=%d invalid iotype=%d\n",
                  (int)pmadbusobj->devnum, iotype);
        }

    return;
}

//This function transfers data between the host and the 'device'
//and updates the appropriate index register
//Then it invokes the target device driver's Interrupt Service Routine
//
void mbdt_process_bufrd_io(PMADBUSOBJ pmadbusobj, bool bWrite, u32 msidx)
{
   	PMAD_SIMULATOR_PARMS pSimParms = &pmadbusobj->SimParms;
	PMADREGS pmaddevice = pmadbusobj->pmaddevice;
    //U8       bMSI = pmadbusobj->pcidev.msi_cap;
    U32      CountBits =
             ((pmaddevice->Control & MAD_CONTROL_IO_COUNT_MASK) >> MAD_CONTROL_IO_COUNT_SHIFT);
    U8       UnitSzB = ((pmaddevice->Control & MAD_CONTROL_IOSIZE_BYTES_BIT) != 0);      
    U32      UnitSize = (UnitSzB) ? MAD_UNITIO_SIZE_BYTES : MAD_SECTOR_SIZE;
    U32      IoCount = (CountBits + 1) * UnitSize;
    //
	irqreturn_t  isr_rc;

    PINFO("mbdt_process_bufrd_io... \ndev#=%ld CountBits=x%lX IoSzize=%ld UnitSz=%ld IoCount=%ld RdIndx=%ld WrIndx=%ld\n",
          (long int)pmadbusobj->devnum, CountBits, (long int)UnitSzB, (long int)UnitSize, (long int)IoCount,
          (long int)pmaddevice->ByteIndxRd, (long int)pmaddevice->ByteIndxWr);

    ASSERT((int)((pmaddevice->Control & MAD_CONTROL_CACHE_XFER_BIT) == 0));
    //if (bMSI)
    //    {msidx = pmaddevice->MesgID - 1;}
    pmaddevice->MesgID = msidx;

    //Acquire the target device driver's device spinlock before changing the device state
	//spin_lock(pSimParms->pdevlock);

	if (bWrite)
	    {
		pmaddevice->Status |= (CountBits << MAD_STATUS_WRITE_COUNT_SHIFT);
        pmaddevice->ByteIndxWr += IoCount;
        pmaddevice->IntID |= MAD_INT_BUFRD_OUTPUT_BIT;
	    }
	else
	    {
		pmaddevice->Status |= (CountBits << MAD_STATUS_READ_COUNT_SHIFT);
        pmaddevice->ByteIndxRd += IoCount;
		pmaddevice->IntID |= MAD_INT_BUFRD_INPUT_BIT;
	    }

    //Release the target device driver's device spinlock 
	//spin_unlock(pSimParms->pdevlock);

//Invoke_ISR:;
    //PDEBUG("madbus_dev_thread:mbdt_process_bufrd_io... dev#=%d pmaddev=%px msidx=%d isrfn=%px iocount=%ld\n",
    //       (int)pmadbusobj->devnum, pmaddevice, msidx, pmadbusobj->isrfn[msidx], IoCount);

    //Call the device driver's ISR
    isr_rc = pmadbusobj->isrfn[msidx](pmadbusobj->irq[msidx], pSimParms->pmaddevobj);
	if (isr_rc != IRQ_HANDLED)
        {PERR("madbus_dev_thread:mbdt_process_bufrd_io... devnum=%d isr_rc=(%d)\n",
              (int)pmadbusobj->devnum, (int)isr_rc);}

    return;
}

//This function transfers data between the host and the 'device'
//and updates the appropriate index register
//Then it invokes the target device driver's Interrupt Service Routine
//
void mbdt_process_cache_io(PMADBUSOBJ pmadbusobj, bool write, u32 msidx)
{
	PMADREGS pmaddevice = pmadbusobj->pmaddevice;
	U8*      pDevData   = ((u8*)pmaddevice + MAD_DEVICE_DATA_OFFSET);
	U8*      pCacheData = (u8*)pmaddevice;
	PMAD_SIMULATOR_PARMS pSimParms = &pmadbusobj->SimParms;
    //
    irqreturn_t  isr_rc;

    //BUG_ON(!(virt_addr_valid(pHostData)));

    //We know that the block-io driver has six MSI-ISRs
    //MesgID 3..8 --> ISR[0..5] ... we just know
    //if (bMSI)
    //    {msidx = pmaddevice->MesgID - 3;}

    //Acquire the target device driver's device spinlock before changing
    //the device state
	//spin_lock(pSimParms->pdevlock);

	if (write)
	    {
		pDevData += (pmaddevice->CacheIndxWr * MAD_CACHE_SIZE_BYTES);
		pCacheData += MAD_CACHE_WRITE_OFFSET;

		//Push the write cache to the device if the cache is loaded,
        // load/reload the write cache
    	if (pmaddevice->Status & MAD_STATUS_WRITE_CACHE_MT_BIT)
		    {
			pmaddevice->Status &= ~MAD_STATUS_WRITE_CACHE_MT_BIT;
			pmaddevice->Status |= MAD_STATUS_OVER_UNDER_ERR_BIT;
			pmaddevice->IntID  |= MAD_INT_STATUS_ALERT_BIT;
		    }
		else //only push and advance the cache index if the cache was not empty
		    {
	        memcpy(pDevData, pCacheData, MAD_CACHE_SIZE_BYTES);
			pmaddevice->CacheIndxWr += MAD_CACHE_NUM_SECTORS;
		    }

		pmaddevice->IntID |= MAD_INT_BUFRD_OUTPUT_BIT;
	    }
	else //cache read
	    {
	    pDevData += (pmaddevice->CacheIndxRd * MAD_CACHE_SIZE_BYTES);
		pCacheData += MAD_CACHE_READ_OFFSET;

		//Pull the read cache into the host, reload the read cache
		if (pmaddevice->Status & MAD_STATUS_READ_CACHE_MT_BIT)
		    {
			pmaddevice->Status &= ~MAD_STATUS_READ_CACHE_MT_BIT;
			pmaddevice->Status |= MAD_STATUS_OVER_UNDER_ERR_BIT;
			pmaddevice->IntID  |= MAD_INT_STATUS_ALERT_BIT;
		    }
		else //only pull and advance the cache index if the cache was not empty
		    {
			pmaddevice->CacheIndxRd += MAD_CACHE_NUM_SECTORS;
		    }

        // load/reload the read cache
	    memcpy(pCacheData, pDevData, MAD_CACHE_SIZE_BYTES);
		pmaddevice->IntID |= MAD_INT_BUFRD_INPUT_BIT;
	    }

    //Release the target device driver's device spinlock 
	//spin_unlock(pSimParms->pdevlock);

//Invoke_ISR:; 
    //Let the cache i/o complete synchronously
    //isr_rc = pmadbusobj->isrfn[msidx](pmadbusobj->irq[msidx], pSimParms->pmaddevobj);
	//if (isr_rc != IRQ_HANDLED)
    //    {PERR("madbus_dev_thread:mbdt_process_cache_io... devnum=%d isr_rc=(%d)\n",
    //          (int)pmadbusobj->devnum, (int)isr_rc);}

    return;
}

//This function (re)aligns a specific cache read/write by updating 
//the appropriate index register
//Then it invokes the target device driver's Interrupt Service Routine
//
void mbdt_process_align_cache(PMADBUSOBJ pmadbusobj, bool bWrite, u32 msidx)
{
	PMADREGS pmaddevice = pmadbusobj->pmaddevice;
	U8*      pDevData = ((u8*)pmaddevice + MAD_DEVICE_DATA_OFFSET);
	u32      OffsetBits = 
             (pmaddevice->Control & MAD_CONTROL_IO_OFFSET_MASK);
	PMAD_SIMULATOR_PARMS pSimParms = &pmadbusobj->SimParms;
	//
	u32          AlignVal;
	irqreturn_t  isr_rc;

    //We know that the block-io driver has six MSI-ISRs
    //MesgID 3..8 --> ISR[0..5] ... we just know
    //if (bMSI)
    //    {msidx = pmaddevice->MesgID - 3;}

	if (bWrite)
        {pDevData += (pmaddevice->CacheIndxWr * MAD_CACHE_SIZE_BYTES);}
 	else
        {pDevData += (pmaddevice->CacheIndxRd * MAD_CACHE_SIZE_BYTES);}

	OffsetBits = (OffsetBits >> MAD_CONTROL_IO_OFFSET_SHIFT);
	AlignVal   = OffsetBits * MAD_CACHE_GRANULARITY;

    //Acquire the target device driver's device spinlock before changing
    //the device state
	//spin_lock(pSimParms->pdevlock);

	if (bWrite)
	    {
		pmaddevice->CacheIndxWr = AlignVal;
		//Invalidate the write cache
		pmaddevice->Status |= MAD_STATUS_WRITE_CACHE_MT_BIT;
		pmaddevice->IntID |= MAD_INT_ALIGN_OUTPUT_BIT;
	    }
	else
	    {
		pmaddevice->CacheIndxRd = AlignVal;
		//Invalidate the read cache
		pmaddevice->Status |= MAD_STATUS_READ_CACHE_MT_BIT;
		pmaddevice->IntID |= MAD_INT_ALIGN_INPUT_BIT;
	    }

    //Release the target device driver's device spinlock 
	//spin_unlock(pSimParms->pdevlock);

    //Let the cache i/o complete synchronously
	//isr_rc = pmadbusobj->isrfn[msidx](pmadbusobj->irq[msidx], pSimParms->pmaddevobj);
    //if (isr_rc != IRQ_HANDLED)
    //    {PERR("madbus_dev_thread:mbdt_processs_Align_Cache... devnum=%d, isr_rc=(%d)\n",
    //          (int)pmadbusobj->devnum, (int)isr_rc);}

    return;
}

//This function implements one DMA-Io
//
void mbdt_process_dma_io(PMADBUSOBJ pmadbusobj, bool bWrite, u32 msidx)
{
    PMAD_SIMULATOR_PARMS pSimParms = &pmadbusobj->SimParms;
	PMADREGS pmaddevice = pmadbusobj->pmaddevice;
    //
    U32      ControlReg = pmaddevice->Control;
    U8       bSGDMA = ((ControlReg & MAD_CONTROL_CHAINED_DMA_BIT) != 0);
    U32      CountBits =
             ((pmaddevice->Control & MAD_CONTROL_IO_COUNT_MASK) >> MAD_CONTROL_IO_COUNT_SHIFT);
    //U32      UnitSize = MAD_SECTOR_SIZE;
	//
    //U32      msidx = 0;
	irqreturn_t  isr_rc;

    //We know that the block-io driver has six MSI-ISRs
    //MesgID 3..8 --> ISR[0..5] ... we just know
    //
    //if (pmadbusobj->pcidev.msi_cap != 0)
    //    {msidx = pmaddevice->MesgID - 3;}
 
    //Acquire the target device driver's device spinlock before changing
    //the device state
	//spin_lock(pSimParms->pdevlock);

    if (bSGDMA)
        {
        if (bWrite)
            {pmaddevice->IntID |= MAD_INT_DMA_OUTPUT_BIT;}
        else
            {pmaddevice->IntID |= MAD_INT_DMA_INPUT_BIT;}
        }
    else //one block dma
        {
        if (bWrite)
            {
            pmaddevice->IntID |= MAD_INT_DMA_OUTPUT_BIT;
            pmaddevice->Status |= (CountBits << MAD_STATUS_WRITE_COUNT_SHIFT);
            }
        else
            {
            pmaddevice->IntID |= MAD_INT_DMA_INPUT_BIT;
            pmaddevice->Status |= (CountBits << MAD_STATUS_READ_COUNT_SHIFT);
            }
        }

//Invoke_ISR:; 
    //PDEBUG("madbus_dev_thread:mbdt_processs_dma_io... dev#=%d IntID=x%X\n",
    //     (int)pmadbusobj->devnum, pmaddevice->IntID);
    //Release the target device driver's device spinlock 
	//spin_unlock(pSimParms->pdevlock);

    //Call the device driver's ISR
	isr_rc = pmadbusobj->isrfn[msidx](pmadbusobj->irq[msidx], pSimParms->pmaddevobj);
    if (isr_rc != IRQ_HANDLED)
        {PERR("madbus_dev_thread:mbdt_processs_dma_io... dev#=%d isr_rc=(%d)\n",
              (int)pmadbusobj->devnum, (int)isr_rc);}

    return;
}

//This function completes one i/o for the implements one device i/o indicated by the non-zero value
//in the mesg id register (implemented as part of the device)
void madsim_complete_simulated_io(void* vpmadbusobj, PMADREGS pmadregs)
{
    PMADBUSOBJ pmadbusobj = (PMADBUSOBJ)vpmadbusobj;
    u8*        pDevData  = 
               ((u8*)pmadbusobj->pmaddevice + MAD_DEVICE_DATA_OFFSET);
    U8*        pDeviceLoc = (pDevData + pmadregs->DevLoclAddr);
    U8*        pHostData = phys_to_virt(pmadregs->HostPA);
    U32        CountBits =
               ((pmadregs->Control & MAD_CONTROL_IO_COUNT_MASK) >> MAD_CONTROL_IO_COUNT_SHIFT);
    U8         UnitSzB = ((pmadregs->Control & MAD_CONTROL_IOSIZE_BYTES_BIT) != 0);      
    U32        UnitSize = (UnitSzB) ? MAD_UNITIO_SIZE_BYTES : MAD_SECTOR_SIZE;
    U32        IoCount = (CountBits + 1) * UnitSize;
    bool       bSGDMA = ((pmadregs->Control & MAD_CONTROL_CHAINED_DMA_BIT) != 0);
	IoType     iotype = mbdt_get_programmed_iotype_worker(pmadregs);
 
    ASSERT((int)(pmadregs->IntID != 0));
    ASSERT((int)(pmadregs->IntEnable != 0));

    if (bSGDMA)
        {
        madsim_complete_simulated_sgdma(pmadbusobj, pmadregs);
        return;
        }

    PINFO("madsim_complete_simulated_io... dev#=%d iotype=%d\n",
          (int)pmadbusobj->devnum, iotype);

    BUG_ON(!(virt_addr_valid(pHostData)));

	switch (iotype)
	    {
        case eBufrdRd: 
            //back up the index register because it has been advanced
            //not a problem because we are working with the ISR-DPC copy of the registers
            pmadregs->ByteIndxRd -= IoCount;
            pDeviceLoc = (U8*)((U64)pDevData + pmadregs->ByteIndxRd); 
            //PINFO("Data=x%lX\n", *(U32*)pDeviceLoc);
            memcpy(pHostData, pDeviceLoc, IoCount);
		    break;

        case eBufrdWr: 
            //back up the index register because it has been advanced
            //not a problem because we are working with the ISR-DPC copy of the registers
            pmadregs->ByteIndxWr -= IoCount;
            pDeviceLoc = (U8*)((U64)pDevData + pmadregs->ByteIndxWr); 
            memcpy(pDeviceLoc, pHostData, IoCount);
		    break;

	    case eLoadRdCache: 
	    case eFlushWrCache:
	    case eAlignRdCache: 
	    case eAlignWrCache: 
            PINFO("madsim_complete_simulated_io... dev#=%d cache op: iotype=%d\n",
                  (int)pmadregs->Devnum, iotype);
            break;

        case eDmaRead: 
            ASSERT((int)(pmadregs->DTBC != 0));
            IoCount = pmadregs->DTBC;
            memcpy(pHostData, pDeviceLoc, IoCount);
		    break;

        case eDmaWrite: 
            ASSERT((int)(pmadregs->DTBC != 0));
            IoCount = pmadregs->DTBC;
            memcpy(pDeviceLoc, pHostData, IoCount);
		    break;

        default:
	        PERR("madddev_complete_simulated_io... dev#=%d invalid iotype=%d\n",
    		     (int)pmadbusobj->devnum, iotype);
	    }

    PINFO("madsim_complete_simulated_io...\n          dev#=%d IoType=%d IntID=x%lX Control=x%lX HostPA=x%llX DevLoclAddr=x%llX iocount=%ld\n",
          (int)pmadbusobj->devnum, iotype, pmadregs->IntID, pmadregs->Control,
          pmadregs->HostPA, pmadregs->DevLoclAddr, (long int)IoCount);

    return;
}

void madsim_complete_simulated_sgdma(PMADBUSOBJ pmadbusobj, PMADREGS pmadregs)
{
    U64        BCDPP     = pmadregs->BCDPP;
    PMAD_DMA_CHAIN_ELEMENT pSgDmaElement = 
                           (PMAD_DMA_CHAIN_ELEMENT)phys_to_virt(BCDPP);
    //
    //U64        CDPP;
    U32        IoCount = 0;
    U32        num_elems = 1;

    madsim_complete_xfer_one_dma_element(pmadbusobj, pSgDmaElement);
    IoCount += pSgDmaElement->DXBC;

    while ((pSgDmaElement->CDPP & MAD_DMA_CDPP_END) == 0)
        {
        num_elems++;
        pSgDmaElement = phys_to_virt(pSgDmaElement->CDPP);
        madsim_complete_xfer_one_dma_element(pmadbusobj, pSgDmaElement);
        IoCount += pSgDmaElement->DXBC;
        }

    pmadbusobj->pmaddevice->DTBC = IoCount;

    PDEBUG("madsim_complete_simulated_sgdma... dev#=%d num_elems=%ld iocount=%ld\n",
           (int)pmadbusobj->devnum, (long int)num_elems, (long int)IoCount);
    return;
}

void madsim_complete_xfer_one_dma_element(PMADBUSOBJ pmadbusobj, 
                                          PMAD_DMA_CHAIN_ELEMENT pSgDmaElement)
{
    u8*        pDevData = 
               ((u8*)pmadbusobj->pmaddevice + MAD_DEVICE_DATA_OFFSET);
    U8*        pDeviceLoc = (pDevData + pSgDmaElement->DevLoclAddr);
    U8*        pHostData = phys_to_virt(pSgDmaElement->HostAddr);
    bool       bWrite = ((pSgDmaElement->DmaCntl & MAD_DMA_CNTL_H2D) != 0);

    BUG_ON(!(virt_addr_valid(pHostData)));
    BUG_ON(!(virt_addr_valid(pDeviceLoc)));
    BUG_ON(!((pSgDmaElement->DevLoclAddr % MAD_SECTOR_SIZE) == 0));
    //
    ASSERT((int)(pSgDmaElement->DXBC != 0));
    ASSERT((int)((pSgDmaElement->DXBC % MAD_SECTOR_SIZE) == 0));

    if (bWrite)
        {memcpy(pDeviceLoc, pHostData, pSgDmaElement->DXBC);}
    else
        {memcpy(pHostData, pDeviceLoc, pSgDmaElement->DXBC);}
}


