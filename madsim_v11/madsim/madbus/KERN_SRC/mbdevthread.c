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

static IoMesgId mbdt_get_programmed_iotype(PMADREGS pMadDevRegs);
static void mbdt_process_io(PMADBUSOBJ pmadbusobj);

//This function creates a one-per-device thread and binds it to a processor
//
int madbus_create_thread(PMADBUSOBJ pmadbusobj)
{
//static ULONG Flags = (CLONE_FS | CLONE_FILES | CLONE_SIGHAND | SIGCHLD);
static char ThreadName[] = "mbdthreadX";
static ULONG TNDX = 9;    //.........^
static char DevNumStr[] = DEVNUMSTR;
//
int rc = 0;

    ASSERT((int)(pmadbusobj != NULL));

    ThreadName[TNDX] = DevNumStr[pmadbusobj->devnum];
 
    pmadbusobj->pThread = 
    kthread_create(madbus_dev_thread, (void *)pmadbusobj, "%s", ThreadName);
    //
    kthread_bind(pmadbusobj->pThread, pmadbusobj->devnum); //assigning cpu=dev#
    
    //kthread_bind succeeds even when specifying a nox-existent cpu - go figure
    if (!IS_ERR(pmadbusobj->pThread))
        {wake_up_process(pmadbusobj->pThread);}

    PINFO("madbus_create_thread... dev#=%d pmadbusobj=%px pThread=%px\n",
    	  (int)pmadbusobj->devnum, pmadbusobj, pmadbusobj->pThread);

    //Release quantum for sequencing... so that threads initialize in start order
    schedule(); 

    if ((pmadbusobj->pThread == NULL) || (IS_ERR(pmadbusobj->pThread)))
        {
    	rc = -ESRCH; //No such process *!?!*
    	PERR("madbus:madbus_create_thread failure... error=%ld, rc=-ESRCH\n",
    		 PTR_ERR(pmadbusobj->pThread));
        }

    return rc;
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
int bindcpu = get_cpu();
int cur_cpu = -1;
IoMesgId mesgid = eNOP;
//int rc = 0;

    ASSERT((int)(pmaddevice != NULL));
     
    PINFO("madbus_dev_thread... dev#=%d cpu=%d pmadbusobj=%px pmaddevice=%px\n",
    	  (int)pmadbusobj->devnum, bindcpu, pmadbusobj, pmaddevice);
 
    //We want to run specifically on cpu# = dev# and leave cpu 0 alone
    //This test only works because kthread_bind(s) to proc_0 when we ask for cpu > (numcpus - 1)
    // (devnum == numcpus)
    if (bindcpu != pmadbusobj->devnum)
        {
        pmadbusobj->pThread = NULL;
        PERR("MBDT... bindcpu(%d) != devnum(%d); returning -EINVAL\n",
    		 bindcpu, (int)pmadbusobj->devnum);

        return -EBADSLT; //Invalid bus slot - whatever
        }

    //Initialize the device (any non-zero values) before processing begins
    pmaddevice->Status |= MAD_STATUS_CACHE_INIT_MASK;

    //Look for the device mesg-id register to become non-zero
    //set by the simulator-ui through direct wiring by mmap
    //
    while (1)
        {
        mesgid =  mbdt_get_programmed_iotype(pmaddevice);
     	if (mesgid != eNOP) //We are ready to complete an i/o
    	    {
            pmaddevice->MesgID = mesgid;
    	    mbdt_process_io(pmadbusobj);

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
       	if (cur_cpu != bindcpu) //Should not happen after initial check above
            {PWARN("MBDT... cur_cpu(%d) != bindcpu-devnum(%d)\n",
                   cur_cpu, (int)pmadbusobj->devnum);}
        }

    PINFO("madbus_dev_thread exiting... pmadbusobj=%px dev#=%d\n",
    	  pmadbusobj, (int)pmadbusobj->devnum);

    //Falling through and returning 0 is sufficient
    //exit_kthread(pmadbusobj->pThread);

    return 0;
}

//Detemine what Io has been programmed and  needs to be completed in this device thread
static IoMesgId mbdt_get_programmed_iotype(PMADREGS pMadDevRegs)
{
static u32 dev_iotag = 0;
static U32 MAD_GO_BITS = (MAD_CONTROL_DMA_GO_BIT | MAD_CONTROL_BUFRD_GO_BIT);

IoMesgId mesgid = eNOP;
u32 IntEnable = pMadDevRegs->IntEnable;

    if (pMadDevRegs->IntID != 0) //an i/o is in progress
        {return mesgid;}
        
    if (IntEnable == 0) //No i/o is programmed
        {return mesgid;}

    if ((pMadDevRegs->Control & MAD_GO_BITS) == 0) //No i/o is programmed
        {return mesgid;}
        
    if (dev_iotag != pMadDevRegs->IoTag)
        {
        PERR("mbdt_get_progammed_iotype... sequence error; expected=%ld device=%ld\n",
             dev_iotag, pMadDevRegs->IoTag); 
        BUG_ON(dev_iotag != pMadDevRegs->IoTag);
        }

    if ((IntEnable & MAD_INT_DMA_OUTPUT_BIT) != 0)
        {mesgid = eDmaWrite;}    

    if ((IntEnable & MAD_INT_DMA_INPUT_BIT) != 0)
        {mesgid = eDmaRead;}    

    if ((IntEnable & MAD_INT_ALIGN_OUTPUT_BIT) != 0)
        {mesgid = eAlignWrCache;}    

    if ((IntEnable & MAD_INT_ALIGN_INPUT_BIT) != 0)
        {mesgid = eAlignRdCache;}    

    if ((IntEnable & MAD_INT_BUFRD_OUTPUT_BIT) != 0)
        {
        if ((pMadDevRegs->Control & MAD_CONTROL_CACHE_XFER_BIT) != 0)
            {mesgid = eFlushWrCache;} 
        else
            {mesgid = eBufrdWr;}    
        }     

    if ((IntEnable & MAD_INT_BUFRD_INPUT_BIT) != 0)
        {
        if ((pMadDevRegs->Control & MAD_CONTROL_CACHE_XFER_BIT) != 0)
            {mesgid = eLoadRdCache;} 
        else
            {mesgid = eBufrdRd;}    
        }     

    if (mesgid != eNOP)
        {dev_iotag++;}

    return mesgid;
}            

static void mbdt_process_io(PMADBUSOBJ pmadbusobj)
{
	PMADREGS pmaddev = pmadbusobj->pmaddevice;
	u32      IoType  = pmaddev->MesgID;

	PINFO("mbdthread%d:mbdt_process_io... IoType=%d IntEnable=x%X IntID=x%X\n",
	   	  (int)pmadbusobj->devnum, (int)IoType, pmaddev->IntEnable, pmaddev->IntID);

    ASSERT((int)(pmaddev->IntEnable != 0));

    switch (IoType)
        {
        case eBufrdRd: 
            mbdt_process_bufrd_io(pmadbusobj, false);
            break;

        case eBufrdWr: 
            mbdt_process_bufrd_io(pmadbusobj, true);
            break;

        case eLoadRdCache: 
            mbdt_process_cache_io(pmadbusobj, false);
            break;

        case eFlushWrCache:
            mbdt_process_cache_io(pmadbusobj, true);
            break;

        case eAlignRdCache: 
            mbdt_process_align_cache(pmadbusobj, false);
            break;

        case eAlignWrCache: 
            mbdt_process_align_cache(pmadbusobj, true);
            break;

        case eDmaRead: 
            mbdt_process_dma_io(pmadbusobj, false);
            break;

        case eDmaWrite: 
            mbdt_process_dma_io(pmadbusobj, true);
            break;

        default:
            PWARN("madbus_dev_thread:mbdt_complete_simulated_io... dev#=%d invalid iotype=%d\n",
                  (int)pmadbusobj->devnum), IoType;
        }

	//pmadbusobj->pmaddevice->MesgID = 0; //So as not to repeat the i/o
	return;
}

//This function transfers data between the host and the 'device'
//and updates the appropriate index register
//Then it invokes the target device driver's Interrupt Service Routine
//
static void mbdt_process_bufrd_io(PMADBUSOBJ pmadbusobj, bool bWrite)
{
   	PMAD_SIMULATOR_PARMS pSimParms = &pmadbusobj->SimParms;
	PMADREGS pmaddevice = pmadbusobj->pmaddevice;
    U8       bMSI = pmadbusobj->pcidev.msi_cap;
    U32      CountBits =
             ((pmaddevice->Control & MAD_CONTROL_IO_COUNT_MASK) >> MAD_CONTROL_IO_COUNT_SHIFT);
    U8       UnitSzB = ((pmaddevice->Control & MAD_CONTROL_IOSIZE_BYTES_BIT) != 0);      
    U32      UnitSize = (UnitSzB) ? MAD_UNITIO_SIZE_BYTES : MAD_SECTOR_SIZE;
    U32      IoCount = (CountBits + 1) * UnitSize;
    //
	irqreturn_t  isr_rc;
    U32      msidx = 0;

    PINFO("mbdt_process_bufrd_io... \ndev#=%d CountBits=x%X IoSzf=x%X UnitSz=%ld IoCount=%ld RdIndx=%ld WrIndx=%ld\n",
          pmadbusobj->devnum, CountBits, UnitSzB, UnitSize, IoCount,
          pmaddevice->ByteIndxRd, pmaddevice->ByteIndxWr);

    ASSERT((int)((pmaddevice->Control & MAD_CONTROL_CACHE_XFER_BIT) == 0));
    if (bMSI)
        {msidx = pmaddevice->MesgID - 1;}

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

Invoke_ISR:;
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
static void mbdt_process_cache_io(PMADBUSOBJ pmadbusobj, bool write)
{
	PMADREGS pmaddevice = pmadbusobj->pmaddevice;
	U8*      pDevData   = ((u8*)pmaddevice + MAD_DEVICE_DATA_OFFSET);
	U8*      pCacheData = (u8*)pmaddevice;
	PMAD_SIMULATOR_PARMS pSimParms = &pmadbusobj->SimParms;
    U8       bMSI = pmadbusobj->pcidev.msi_cap;
    //
    irqreturn_t  isr_rc;
    U32      msidx = 0;

    //BUG_ON(!(virt_addr_valid(pHostData)));

    //We know that the block-io driver has six MSI-ISRs
    //MesgID 3..8 --> ISR[0..5] ... we just know
    if (bMSI)
        {msidx = pmaddevice->MesgID - 3;}

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

Invoke_ISR:; 
    //Call the device driver's ISR
    isr_rc = pmadbusobj->isrfn[msidx](pmadbusobj->irq[msidx], pSimParms->pmaddevobj);
	if (isr_rc != IRQ_HANDLED)
        {PERR("madbus_dev_thread:mbdt_process_cache_io... devnum=%d isr_rc=(%d)\n",
              (int)pmadbusobj->devnum, (int)isr_rc);}

    return;
}

//This function (re)aligns a specific cache read/write by updating 
//the appropriate index register
//Then it invokes the target device driver's Interrupt Service Routine
//
static void mbdt_process_align_cache(PMADBUSOBJ pmadbusobj, bool bWrite)
{
	PMADREGS pmaddevice = pmadbusobj->pmaddevice;
	U8*      pDevData = ((u8*)pmaddevice + MAD_DEVICE_DATA_OFFSET);
	u32      OffsetBits = 
             (pmaddevice->Control & MAD_CONTROL_IO_OFFSET_MASK);
	PMAD_SIMULATOR_PARMS pSimParms = &pmadbusobj->SimParms;
    U8       bMSI = pmadbusobj->pcidev.msi_cap;
	//
    U32      msidx = 0;
	u32          AlignVal;
	irqreturn_t  isr_rc;

    //We know that the block-io driver has six MSI-ISRs
    //MesgID 3..8 --> ISR[0..5] ... we just know
    if (bMSI)
        {msidx = pmaddevice->MesgID - 3;}

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

    //Call the device driver's ISR
	isr_rc = pmadbusobj->isrfn[msidx](pmadbusobj->irq[msidx], pSimParms->pmaddevobj);
    if (isr_rc != IRQ_HANDLED)
        {PERR("madbus_dev_thread:mbdt_processs_Align_Cache... devnum=%d, isr_rc=(%d)\n",
              (int)pmadbusobj->devnum, (int)isr_rc);}

    return;
}

//This function implements one DMA-Io
//
static void mbdt_process_dma_io(PMADBUSOBJ pmadbusobj, bool bWrite)
{
    PMAD_SIMULATOR_PARMS pSimParms = &pmadbusobj->SimParms;
	PMADREGS pmaddevice = pmadbusobj->pmaddevice;
    //
    U32      ControlReg = pmaddevice->Control;
    U8       bSGDMA = ((ControlReg & MAD_CONTROL_CHAINED_DMA_BIT) != 0);
    U32      CountBits =
             ((pmaddevice->Control & MAD_CONTROL_IO_COUNT_MASK) >> MAD_CONTROL_IO_COUNT_SHIFT);
    U32      UnitSize = MAD_SECTOR_SIZE;
	//
    U32      msidx = 0;
	irqreturn_t  isr_rc;

    //We know that the block-io driver has six MSI-ISRs
    //MesgID 3..8 --> ISR[0..5] ... we just know
    //
    if (pmadbusobj->pcidev.msi_cap != 0)
        {msidx = pmaddevice->MesgID - 3;}
 
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

Invoke_ISR:; 
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

#if 0
// This function implements one Scatter-Gather DMA-Io
// This 'DMA controller' is modelled on the Tundra Universe-II DMA controller
// It processes a simple linked-list of physical addresses
// where the root address comes from a register in the device
//
void mbdt_process_sgdma(PMADREGS pmaddevice, bool bWrite)
{
    u8*      pDevData = ((u8*)pmaddevice + MAD_DEVICE_DATA_OFFSET);
    U32        CDPP  = pmaddevice->BCDPP; //Chained-DMA Base Pkt Pntr
    //
    struct page* pPgStrDma;
    struct page* pPgStrHost;
    PMAD_DMA_CHAIN_ELEMENT  pDmaRootPktPntr = Get_KVA(CDPP, &pPgStrDma);
    PMAD_DMA_CHAIN_ELEMENT  pDmaPktPntr = pDmaRootPktPntr;
    U8*        pHostSgBufr;
    U8*        pDeviceLoc;
    U32        DXBC; //Data Xfer Byte Count
    U32        TotlXferLen = 0;
    U32        ChainLen = 0;

    ASSERT((int)(pDmaRootPktPntr != NULL));

    while (CDPP != MAD_DMA_CDPP_END)
        {
		if (pDmaPktPntr == NULL) 
		    {break;} 

        DXBC = pDmaPktPntr->DXBC;
        pHostSgBufr = Get_KVA(pDmaPktPntr->HostAddr, &pPgStrHost);
        BUG_ON(!(virt_addr_valid(pHostSgBufr)));
        pDeviceLoc = (pDevData + pDmaPktPntr->DevLoclAddr);
        //
        if (pDmaPktPntr->DmaCntl & MAD_DMA_CNTL_H2D) //Host-->Device
            { //Write
            ASSERT((int)(bWrite == 1));
            memcpy(pDeviceLoc, pHostSgBufr, DXBC); 
            }
        else 
            { //Read
            ASSERT((int)(bWrite == 0));
            memcpy(pHostSgBufr, pDeviceLoc, DXBC); 
            }
		//
        if (pPgStrHost != NULL)
            kunmap(pPgStrHost);

        TotlXferLen += DXBC;
		ChainLen++;

        //Get next hardware scatter-gather element physical address
        CDPP = pDmaPktPntr->CDPP; 
        
        //We know that the linked-list of dma pkt pntrs is a packed array...we just know
        //We need to accomodate the fact that phys-addr CDPPs won't be page aligned
        // after the 1st one - so we just jump to the next array element
        //    
        pDmaPktPntr++;
        } //end while

    if (pPgStrDma != NULL)
        kunmap(pPgStrDma);

    pmaddevice->DTBC = TotlXferLen; //Store the final total in the base DTBC

    return;
}
#endif

//This function completes one i/o for the implements one device i/o indicated by the non-zero value
//in the mesg id register (implemented as part of the device)
void madsim_complete_simulated_io(void* vpmadbusobj, PMADREGS pmadregs)
{
    PMADBUSOBJ pmadbusobj = (PMADBUSOBJ)vpmadbusobj;
	u32        IoType    = pmadregs->MesgID;
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

	//PINFO("madsim_complete_simulated_io... dev#=%d IoType=%d IntID=x%X Control=x%X\n",
	//   	  (int)pmadbusobj->devnum, (int)IoType, pmadregs->IntID, pmadregs->Control); 
    ASSERT((int)(pmadregs->IntID != 0));
    ASSERT((int)(pmadregs->IntEnable != 0));

    if (bSGDMA)
        {
        madsim_complete_simulated_sgdma(pmadbusobj, pmadregs);
        return;
        }

    //PINFO("madsim_complete_simulated_io... IoCount=%ld RdIndx=%ld WrIndx=%ld\n",
    //      IoCount, pIsrState->ByteIndxRd, pIsrState->ByteIndxWr);

    BUG_ON(!(virt_addr_valid(pHostData)));

	switch (IoType)
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
                  (int)pmadregs->Devnum, IoType);
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
    		     (int)pmadbusobj->devnum, IoType);
	    }

    PINFO("madsim_complete_simulated_io...\n          dev#=%d IoType=%d IntID=x%X Control=x%X HostPA=x%llX DevLoclAddr=x%lX iocount=%ld\n",
          (int)pmadbusobj->devnum, (int)IoType, pmadregs->IntID, pmadregs->Control,
          pmadregs->HostPA, pmadregs->DevLoclAddr, IoCount);

    return;
}

static void madsim_complete_simulated_sgdma(PMADBUSOBJ pmadbusobj, PMADREGS pmadregs)
{
    U64        BCDPP     = pmadregs->BCDPP;
    PMAD_DMA_CHAIN_ELEMENT pSgDmaElement = 
                           (PMAD_DMA_CHAIN_ELEMENT)phys_to_virt(BCDPP);
    //
    U64        CDPP;
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

    PDEBUG("madsim_complete_simulated_sgdma... dev#=%d num_elems=%d iocount=%ld\n",
           (int)pmadbusobj->devnum, num_elems, IoCount);
    return;
}

static void madsim_complete_xfer_one_dma_element(PMADBUSOBJ pmadbusobj, 
                                                 PMAD_DMA_CHAIN_ELEMENT pSgDmaElement)
{
    u8*        pDevData  = 
               ((u8*)pmadbusobj->pmaddevice + MAD_DEVICE_DATA_OFFSET);
    U8*        pDeviceLoc = (pDevData + pSgDmaElement->DevLoclAddr);
    U8*        pHostData = phys_to_virt(pSgDmaElement->HostAddr);
    bool       bWrite = ((pSgDmaElement->DmaCntl & MAD_DMA_CNTL_H2D) != 0);

    BUG_ON(!(virt_addr_valid(pHostData)));
    BUG_ON(!(virt_addr_valid(pDeviceLoc)));
    BUG_ON(!((pSgDmaElement->DevLoclAddr % MAD_SECTOR_SIZE) == 0));
    ASSERT((int)(pSgDmaElement->DXBC != 0));
    ASSERT((int)((pSgDmaElement->DXBC % MAD_SECTOR_SIZE) == 0));

    if (bWrite)
        {memcpy(pDeviceLoc, pHostData, pSgDmaElement->DXBC);}
    else
        {memcpy(pHostData, pDeviceLoc, pSgDmaElement->DXBC);}
}

