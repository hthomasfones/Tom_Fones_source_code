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
__iomem MADREGS   *pmadregs   = pmadbusobj->pmadregs;
//
int cur_cpu = -1;
IoType iotype = eNOP;
int rc = -EADDRNOTAVAIL;

    ASSERT((int)(pmadregs != NULL));
    //cur_cpu = get_cpu(); put_cpu();
    cur_cpu = smp_processor_id(); //No reference count

    PINFO("madbus_dev_thread... dev#=%d cpu=%d pThread=%px madbusobj=%px maddev=%px\n",
    	  (int)pmadbusobj->devnum, cur_cpu, 
          (void *)current, (void *)pmadbusobj, (void *)pmadregs);
 
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
    pmadregs->Status |= MAD_STATUS_CACHE_INIT_MASK;

    //Look for the device mesg-id register to become non-zero
    //set by the simulator-ui through direct wiring by mmap
    //
    while (1)
        {
        iotype =  mbdt_get_programmed_iotype(pmadregs, 
                                             (uint32_t *)&pmadbusobj->dev_iotag);
     	if (iotype != eNOP) //We are ready to complete an i/o
    	    {
            //pmadregs->MesgID = mesgid; ////////////////////////////////////////
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
       	cur_cpu = smp_processor_id();
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

static inline IoType mbdt_get_programmed_iotype_worker(__iomem MADREGS *pMadDevRegs)
{
u32 IntEnable = ioread32(&pMadDevRegs->IntEnable);

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
        if ((ioread32(&pMadDevRegs->Control) & MAD_CONTROL_CACHE_XFER_BIT) != 0)
            {return eLoadRdCache;} 
        else
            {return eBufrdRd;}    
        }     

    return eNOP;
}

//Detemine what Io has been programmed and  needs to be completed in this device thread
static IoType mbdt_get_programmed_iotype(__iomem MADREGS *pMadDevRegs, u32* pIotag)
{
static U32 MAD_GO_BITS = (MAD_CONTROL_DMA_GO_BIT | MAD_CONTROL_BUFRD_GO_BIT);
//
IoType iotype = eNOP;
u32 IntEnable = ioread32(&pMadDevRegs->IntEnable);
u32 IntID     = ioread32(&pMadDevRegs->IntID);
u32 IoTag     = ioread32(&pMadDevRegs->IoTag);
u32 devnum    = ioread32(&pMadDevRegs->Devnum);

    if (IntEnable == 0) //No i/o is programmed
        {return iotype;}

    if ((ioread32(&pMadDevRegs->Control) & MAD_GO_BITS) == 0) //No i/o is programmed
        {return iotype;}

    if (IntID != 0) //an i/o is in progress
        {return iotype;}
        
    if (*pIotag != IoTag)
        {
        PERR("mbdt_get_progammed_iotype... dev#=%ld sequence error; expected=x%lX iotag=x%lX\n",
             (long int)devnum, (long int)*pIotag, (long int)IoTag); 
        BUG_ON(*pIotag != IoTag);
        } /* */

    iotype = mbdt_get_programmed_iotype_worker(pMadDevRegs);
    if (iotype != eNOP) //We have a valid programmed-io to complete
        {(*pIotag)++;}

    return iotype;
}            

static void mbdt_process_io(PMADBUSOBJ pmadbusobj, IoType iotype)
{
	__iomem MADREGS *pmadregs = pmadbusobj->pmadregs;
    u32 msidx = 0; //default to legacy-int
    U8 bMSI = pmadbusobj->pcidev.msi_cap;
    u32 IntEnable = ioread32(&pmadregs->IntEnable);

    if (bMSI)
        {  
        // Our dumb device assigns msidx's in round-robin fashion
        msidx = (ioread32(&pmadregs->IoTag) % 8) + 1; //0..7 --> 1..8  
        }

	PINFO("mbdthread:mbdt_process_io... dev#=%d iotype=%d IntEnable=x%lX msidx=%d\n",
	   	  (int)pmadbusobj->devnum, iotype,
          (long unsigned int)IntEnable, (int)msidx);

    ASSERT((int)(IntEnable != 0));

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

	PINFO("mbdthread:mbdt_process_io... dev#=%d iotype=%d fini\n", 
          (int)pmadbusobj->devnum, iotype);

    return;
}

//This function transfers data between the host and the 'device'
//and updates the appropriate index register
//Then it invokes the target device driver's Interrupt Service Routine
//
void mbdt_process_bufrd_io(PMADBUSOBJ pmadbusobj, bool bH2D, u32 msidx)
{
   	//PMAD_SIMULATOR_PARMS pSimParms = &pmadbusobj->SimParms;
	__iomem MADREGS *pmadregs = pmadbusobj->pmadregs;
    //U8       bMSI = pmadbusobj->pcidev.msi_cap;
    u32 IntID =   ioread32(&pmadregs->IntID);
    u32 Status =   ioread32(&pmadregs->Status);
    u32 ReadIndx = ioread32(&pmadregs->ByteIndxRd);
    u32 WriteIndx = ioread32(&pmadregs->ByteIndxWr);
    U32 Control  =  ioread32(&pmadregs->Control);
    U32 CountBits = 
        ((Control & MAD_CONTROL_IO_COUNT_MASK) >> MAD_CONTROL_IO_COUNT_SHIFT);
    U8       UnitSzB = ((Control & MAD_CONTROL_IOSIZE_BYTES_BIT) != 0);      
    U32      UnitSize = (UnitSzB) ? MAD_UNITIO_SIZE_BYTES : MAD_SECTOR_SIZE;
    U32      IoCount = (CountBits + 1) * UnitSize;
    //U32      ByteIndx = 0;
	irqreturn_t  isr_rc;

    PINFO("mbdt_process_bufrd_io... \n    \
          dev#=%d CountBits=x%x IoSize=%d UnitSz=%u IoCount=%u RdIndx=%x WrIndx=%x\n",
          (int)pmadbusobj->devnum, (uint32_t)CountBits, (uint32_t)UnitSzB, 
          (uint32_t)UnitSize, (uint32_t)IoCount, ReadIndx, WriteIndx);

    ASSERT((int)((Control & MAD_CONTROL_CACHE_XFER_BIT) == 0));
    //if (bMSI)
    //    {msidx = pmadregs->MesgID - 1;}
    iowrite32(msidx, &pmadregs->MesgID);

	if (bH2D)
	    {
		Status |= (CountBits << MAD_STATUS_WRITE_COUNT_SHIFT);
        IntID |= MAD_INT_BUFRD_OUTPUT_BIT;
 	    }
	else
	    {
		Status |= (CountBits << MAD_STATUS_READ_COUNT_SHIFT);
		IntID |= MAD_INT_BUFRD_INPUT_BIT;
	    }

    iowrite32(IntID, &pmadregs->IntID);
    iowrite32(Status, &pmadregs->Status);
    madsim_complete_simulated_io(pmadbusobj, pmadregs);

//Invoke_ISR:;
    //PINFO("madbus_dev_thread:mbdt_process_bufrd_io... dev#=%d pmadregs=%px msidx=%d isrfn=%px iocount=%ld\n",
    //       (int)pmadbusobj->devnum, pmadregs, msidx, pmadbusobj->isrfn[msidx], IoCount);

    //Call the device driver's ISR
    isr_rc = pmadbusobj->isrfn[msidx](pmadbusobj->irq[msidx], 
                                      pmadbusobj->isr_devobj);
	if (isr_rc != IRQ_HANDLED)
        {PERR("madbus_dev_thread:mbdt_process_bufrd_io... devnum=%d isr_rc=(%d)\n",
              (int)pmadbusobj->devnum, (int)isr_rc);}

    return;
}

//This function transfers data between the host and the device
//and updates the appropriate index register
void mbdt_process_cache_io(PMADBUSOBJ pmadbusobj, bool write, u32 msidx)
{
    static u8 bouncebufr[MAD_CACHE_SIZE_BYTES];
//
	__iomem MADREGS *pmadregs = pmadbusobj->pmadregs;
	__iomem U8*      pDevData = (__iomem u8*)((U64)pmadregs + MAD_DEVICE_DATA_OFFSET);
	__iomem U8*      pCacheData = (__iomem u8*)pmadregs;
    u32 IntID =      ioread32(&pmadregs->IntID);
    u32 Status =     ioread32(&pmadregs->Status);
    u32 CacheIndx = 0;

 	if (write)
	    {
        CacheIndx = ioread32(&pmadregs->CacheIndxWr);
        iowrite32((IntID | MAD_INT_BUFRD_OUTPUT_BIT), &pmadregs->IntID);
		pDevData += ((u64)(CacheIndx * MAD_SECTOR_SIZE));
		pCacheData += MAD_CACHE_WRITE_OFFSET;

		// Return the error or push the write cache to the device,
        if (Status & MAD_STATUS_WRITE_CACHE_EMPTY_BIT)
		    {
            Status = (Status & ~MAD_STATUS_WRITE_CACHE_EMPTY_BIT);
            Status |= MAD_STATUS_OVER_UNDER_ERR_BIT;
            iowrite32(Status, &pmadregs->Status);
            iowrite32((IntID | MAD_INT_STATUS_ALERT_BIT), &pmadregs->IntID);
		    }
		else //only push and advance the cache index if the cache was not empty madsim_complete_simulated_io(pmadbusobj, pmadregs);
		    {
            memcpy_fromio(bouncebufr, pCacheData, MAD_CACHE_SIZE_BYTES);
	        memcpy_toio(pDevData, bouncebufr, MAD_CACHE_SIZE_BYTES);
 			iowrite32((CacheIndx + MAD_CACHE_NUM_SECTORS), &pmadregs->CacheIndxWr);
		    }
	    }
	else //cache read
	    {
        CacheIndx = ioread32(&pmadregs->CacheIndxRd);
        iowrite32((IntID | MAD_INT_BUFRD_INPUT_BIT), &pmadregs->IntID);
        //Pull the device data into the cache,... 
	    pDevData += ((u64)(CacheIndx * MAD_SECTOR_SIZE));
		pCacheData += MAD_CACHE_READ_OFFSET;
        memcpy_fromio(bouncebufr, pDevData, MAD_CACHE_SIZE_BYTES);
        memcpy_toio(pCacheData, bouncebufr, MAD_CACHE_SIZE_BYTES);
        iowrite32((CacheIndx + MAD_CACHE_NUM_SECTORS), &pmadregs->CacheIndxRd);
		
    	if (Status & MAD_STATUS_READ_CACHE_EMPTY_BIT)
		    {
            Status &= ~MAD_STATUS_READ_CACHE_EMPTY_BIT;
            Status |= MAD_STATUS_OVER_UNDER_ERR_BIT;
			iowrite32(Status, &pmadregs->Status); 
			iowrite32((IntID | MAD_INT_STATUS_ALERT_BIT), &pmadregs->IntID);
		    }
	    }

    //Release the target device driver's device spinlock 
	//spin_unlock(pSimParms->pdevlock);

    //Let the cache i/o complete synchronously
  
    return;
}

//This function (re)aligns a specific cache read/write by updating 
//the appropriate index register
void mbdt_process_align_cache(PMADBUSOBJ pmadbusobj, bool bH2D, u32 msidx)
{
	__iomem MADREGS *pmadregs = pmadbusobj->pmadregs;
	__iomem U8*      pDevData = ((__iomem u8*)(U64)pmadregs + MAD_DEVICE_DATA_OFFSET);
     u32 IntID =     ioread32(&pmadregs->IntID);
    u32 Status =     ioread32(&pmadregs->Status);
    U32 Control    = ioread32(&pmadregs->Control);
	u32      OffsetBits = (Control & MAD_CONTROL_IO_OFFSET_MASK);
    u32      AlignVal = 0;

 	if (bH2D)
        {pDevData += (ioread32(&pmadregs->CacheIndxWr) * MAD_CACHE_SIZE_BYTES);}
 	else
        {pDevData += (ioread32(&pmadregs->CacheIndxRd) * MAD_CACHE_SIZE_BYTES);}

	OffsetBits = (OffsetBits >> MAD_CONTROL_IO_OFFSET_SHIFT);
	AlignVal   = OffsetBits * MAD_CACHE_GRANULARITY;

	if (bH2D)
	    {
		iowrite32(AlignVal, &pmadregs->CacheIndxWr);
		//Invalidate the write cache
		iowrite32((Status | MAD_STATUS_WRITE_CACHE_EMPTY_BIT), &pmadregs->Status);
		iowrite32((IntID | MAD_INT_ALIGN_OUTPUT_BIT), &pmadregs->IntID);
	    }
	else
	    {
		iowrite32(AlignVal, &pmadregs->CacheIndxRd);
		//Invalidate the read cache
		iowrite32((Status | MAD_STATUS_READ_CACHE_EMPTY_BIT), &pmadregs->Status);
		iowrite32((IntID | MAD_INT_ALIGN_INPUT_BIT), &pmadregs->IntID);
	    }

    //Let the cache i/o complete synchronously

    return;
}

//This function implements one DMA-Io
void mbdt_process_dma_io(PMADBUSOBJ pmadbusobj, bool bH2D, u32 msidx)
{
    //PMAD_SIMULATOR_PARMS pSimParms = &pmadbusobj->SimParms;
	__iomem MADREGS *pmadregs = pmadbusobj->pmadregs;
    u32 IntID =      ioread32(&pmadregs->IntID);
    u32 Status =     ioread32(&pmadregs->Status);
    U32 Control  =   ioread32(&pmadregs->Control);
    U8  bSGDMA     = ((Control & MAD_CONTROL_CHAINED_DMA_BIT) != 0);
    U32  CountBits =
         ((Control & MAD_CONTROL_IO_COUNT_MASK) >> MAD_CONTROL_IO_COUNT_SHIFT);
	irqreturn_t  isr_rc;

    //We know that the block-io driver has six MSI-ISRs
    //MesgID 3..8 --> ISR[0..5] ... we just know
    //
    //if (pmadbusobj->pcidev.msi_cap != 0)
    //    {msidx = pmadregs->MesgID - 3;}
 
    if (bSGDMA)
        {
        if (bH2D)
            {IntID |= MAD_INT_DMA_OUTPUT_BIT;}
        else
            {IntID |= MAD_INT_DMA_INPUT_BIT;}
        }
    else //one block dma
        {
        if (bH2D)
            {
            IntID |= MAD_INT_DMA_OUTPUT_BIT;
            Status |= (CountBits << MAD_STATUS_WRITE_COUNT_SHIFT);
            }
        else
            {
            IntID |= MAD_INT_DMA_INPUT_BIT;
            Status |= (CountBits << MAD_STATUS_READ_COUNT_SHIFT);
            }
        }

    iowrite32(IntID, &pmadregs->IntID);
    iowrite32(Status, &pmadregs->Status);
    madsim_complete_simulated_io(pmadbusobj, pmadregs);

//Invoke_ISR:; ASSERT((int)(PHYS_ADDR_VALID(HostAddr)));
    //PINFO("madbus_dev_thread:mbdt_processs_dma_io... dev#=%d IntID=x%X\n",
    //     (int)pmadbusobj->devnum, pmadregs->IntID);

    //Call the device driver's ISR
	isr_rc = pmadbusobj->isrfn[msidx](pmadbusobj->irq[msidx], 
                                      pmadbusobj->isr_devobj);
    if (isr_rc != IRQ_HANDLED)
        {PERR("madbus_dev_thread:mbdt_processs_dma_io... dev#=%d isr_rc=(%d)\n",
              (int)pmadbusobj->devnum, (int)isr_rc);}

    return;
}

//This function completes one i/o indicated by the non-zero value
//in the mesg id register (implemented as part of the device)
void madsim_complete_simulated_io(void* vpmadbusobj, __iomem MADREGS *pmadregs)
{
    PMADBUSOBJ pmadbusobj = (PMADBUSOBJ)vpmadbusobj;
    u32 devnum =      ioread32(&pmadregs->Devnum);
    u32 IntID =      ioread32(&pmadregs->IntID);
    u32 IntEnable =  ioread32(&pmadregs->IntEnable);
    //u32 Status =     ioread32(&pmadregs->Status);
    U32 DTBC         = ioread32(&pmadregs->DTBC);
    U32        DevDataOfst = ioread32(&pmadregs->DevDataOfst);
    U32        Control  = ioread32(&pmadregs->Control);
    U32        CountBits = ((Control & MAD_CONTROL_IO_COUNT_MASK) >> 
                            MAD_CONTROL_IO_COUNT_SHIFT);
    U8         UnitSzB = ((Control & MAD_CONTROL_IOSIZE_BYTES_BIT) != 0);      
    U32        UnitSize = (UnitSzB) ? MAD_UNITIO_SIZE_BYTES : MAD_SECTOR_SIZE;
    bool       bSGDMA = ((Control & MAD_CONTROL_CHAINED_DMA_BIT) != 0);

    __iomem u8* pDevData  = (__iomem u8*)((U64)pmadregs + MAD_DEVICE_DATA_OFFSET);
    __iomem U8* pSectorData = (__iomem U8*)((U64)pDevData + DevDataOfst);
    __iomem U8* pDevAddr = NULL;
  
	IoType     iotype = mbdt_get_programmed_iotype_worker(pmadregs);
    U32        IoCount = 
               (iotype < eLoadRdCache) ? ((CountBits + 1) * UnitSize) : DTBC;
    //U32        minsize = min_t(U32, IoCount, MAD_SECTOR_SIZE);      
    phys_addr_t HostPA = (phys_addr_t)ioread64(&pmadregs->HostPA);

    __iomem MAD_DMA_CHAIN_ELEMENT *pSgle = 
                                  (PMAD_DMA_CHAIN_ELEMENT)&pmadregs->HostPA;
    U8*        pHpg = kmap_local_page((void*)ioread64(&pSgle->hpage));
    U8*        pHostData = (U8*)((U64)pHpg + ioread32(&pSgle->hoffset));
    U32 ByteIndx = 0;

    ASSERT((int)(IntID != 0));
    ASSERT((int)(IntEnable != 0));

    if (bSGDMA)
        {
        madsim_complete_simulated_sgdma(pmadbusobj, pmadregs);
        return;
        }

    PINFO("madsim_complete_simulated_io...\n    \
          dev#=%d iotype=%d IntID=x%x Control=x%x HostPA=x%llx DevDataOfst=x%x iocount=%u\n",
          (int)pmadbusobj->devnum, iotype, (uint32_t)IntID, (uint32_t)Control,
          HostPA, (uint32_t)DevDataOfst, (uint32_t)IoCount);
 
	switch (iotype)
	    {
        case eBufrdRd: 
            ByteIndx = ioread32(&pmadregs->ByteIndxRd);
            pDevAddr = (__iomem U8*)((U64)pDevData + ByteIndx); 
            pHostData = phys_to_virt(HostPA);
            memcpy_fromio(pHostData, pDevAddr, IoCount);
            iowrite32((ByteIndx+IoCount), &pmadregs->ByteIndxRd);
		    break;

        case eBufrdWr: 
            //back up the index register because it has been advanced
            ByteIndx = ioread32(&pmadregs->ByteIndxWr);
            pDevAddr = (__iomem U8*)((U64)pDevData + ByteIndx); 
            pHostData = phys_to_virt(HostPA);
            memcpy_toio(pDevAddr, pHostData, IoCount);
            iowrite32((ByteIndx+IoCount), &pmadregs->ByteIndxWr);
		    break;

	    case eLoadRdCache: 
	    case eFlushWrCache:
	    case eAlignRdCache: 
	    case eAlignWrCache: 
            PINFO("madsim_complete_simulated_io... dev#=%d cache op: iotype=%d\n",
                  (int)devnum, iotype);
            break;

        case eDmaRead: 
            IoCount = DTBC;
            ASSERT((int)(IoCount != 0));
            ASSERT((int)((DevDataOfst+IoCount) <= MAD_DEVICE_DATA_SIZE));
            memcpy_fromio(pHostData, pSectorData, IoCount);
            kunmap_local(pHpg);   
		    break;

        case eDmaWrite: 
            IoCount = DTBC;
            ASSERT((int)(IoCount != 0));
            ASSERT((int)((DevDataOfst+IoCount) <= MAD_DEVICE_DATA_SIZE));
            memcpy_toio(pSectorData, pHostData, IoCount);
            kunmap_local(pHpg);   
		    break;

        default:
	        PERR("madddev_complete_simulated_io... dev#=%d invalid iotype=%d\n",
    		     (int)devnum, iotype);
	    }

    PINFO("madsim_complete_simulated_io... dev#=%d iotype=%d\n",
          (int)pmadbusobj->devnum, iotype);
  
    return;
}

void madsim_complete_simulated_sgdma(PMADBUSOBJ pmadbusobj, 
                                     __iomem MADREGS *pmadregs)
{
    __iomem MAD_DMA_CHAIN_ELEMENT *pSgle = pmadregs->SgDmaUnits;
    bool       bH2D = ((ioread32(&pSgle->DmaCntl) & MAD_DMA_CNTL_H2D) != 0);
    U32        IoCount = 0;
    U32        num_elems = 1;

    madsim_complete_xfer_one_dma_element(pmadbusobj, pSgle);
    IoCount += ioread32(&pSgle->DXBC);

    while ((ioread64(&pSgle->CDPP) & MAD_DMA_CDPP_END) == 0)
        {
        num_elems++;
        pSgle++; // = phys_to_virt(pSgle->CDPP);
        madsim_complete_xfer_one_dma_element(pmadbusobj, pSgle);
        IoCount += ioread32(&pSgle->DXBC);
        }

    iowrite32(IoCount, &pmadregs->DTBC);

    PINFO("madsim_complete_simulated_sgdma... dev#=%d num_elems=%ld iocount=%ld Wr=%d\n",
          (int)pmadbusobj->devnum, (long int)num_elems, (long int)IoCount, bH2D);
    
    return;
}

//We must never call phys_to_virt() on a dma address
inline void madsim_complete_xfer_one_dma_element(PMADBUSOBJ pmadbusobj, 
                                                 __iomem MAD_DMA_CHAIN_ELEMENT *pSgle)
{
    __iomem u8* pDevData = 
                (u8*)((U64)pmadbusobj->pmadregs + MAD_DEVICE_DATA_OFFSET);
    __iomem U8* pSectorData = (U8*)((U64)pDevData + ioread32(&pSgle->DevDataOfst));
    //
    U8*        pHpg = kmap_local_page(pSgle->hpage);
    U8*        pHostData = (U8*)((U64)pHpg + ioread32(&pSgle->hoffset));
    //
    bool       bH2D = ((ioread32(&pSgle->DmaCntl) & MAD_DMA_CNTL_H2D) != 0);
    U32        xferlen = ioread32(&pSgle->DXBC);

    #if 0
    PINFO("madsim_complete_xfer_1_dma_elem \n \
          dev#=%d HostAddr=%llx pDevLoc=%px pHostData=%px xferlen=%lu Wr=%d\n",
          (int)pmadbusobj->devnum, pSgle->HostAddr,  
          pSectorData, pHostData, xferlen, bH2D);

    ASSERT((int)(xferlen != 0));
    ASSERT((int)((xferlen % MAD_SECTOR_SIZE) == 0));
    ASSERT((int)(VIRT_ADDR_VALID(pSectorData, xferlen)));
    ASSERT((int)(VIRT_ADDR_VALID(pHostData, xferlen)));
    ASSERT((int)((pSgle->DevDataOfst % MAD_SECTOR_SIZE) == 0));
    #endif

    if (bH2D)
        {memcpy_toio(pSectorData, pHostData, xferlen);}
    else
        {memcpy_fromio(pHostData, pSectorData, xferlen);}

    kunmap_local(pHpg);   
}
