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
/*  Exe files   : maddevb.ko                                                   */ 
/*                                                                             */
/*  Module NAME : maddevbio.c                                                  */
/*                                                                             */
/*  DESCRIPTION : Function definitions for the MAD character-mode driver       */
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
/* $Id: maddevbio.c, v 1.0 2021/01/01 00:00:00 htf $                           */
/*                                                                             */
/*******************************************************************************/ 

#include "maddevb.h"		/* local definitions */

extern struct mad_dev_obj *mad_dev_objects;

static u8   IoDataRd[MAD_UNITIO_SIZE_BYTES * (1 << MAD_BUFRD_IO_COUNT_BITS)];
static u8   IoDataWr[MAD_UNITIO_SIZE_BYTES * (1 << MAD_BUFRD_IO_COUNT_BITS)];

//This function implements a queued read through a kernel io control block
#if 0
ssize_t maddev_queued_read(struct kiocb *pkiocb, struct iov_iter *piov)
{
	static u32  IntEnable = (MAD_INT_BUFRD_INPUT_BIT | MAD_INT_STATUS_ALERT_BIT);
	//
	PMADDEVOBJ pmaddevobj = (PMADDEVOBJ)pkiocb->ki_filp->private_data;
	PMADREGS   pmadregs = pmaddevobj->pDevBase;
    size_t     count   = piov->count;
    u8* __user usrbufr = piov->iov->iov_base;
    void*      pBufr = IoDataRd;
    //
	U32      CntlReg = 0;
    U32      StatusReg = 0;
	U32      CountBits = 0;
	int      rc = 0;
    ssize_t  rdcount;
    size_t   remains = 0;

    PNOTICE( "maddev_queued_read... devnum=%d, kiocb=%px, piov=%px, count=%d\n",
           (int)pmaddevobj->devnum, pkiocb, piov, (int)count);

    //If the read queue is not free we return
    if (pmaddevobj->read_f != eIoReset)
        {return -EAGAIN;}

    //Acquire the semaphore for the device context
    down(&pmaddevobj->devsem);

    //Indicate the read queue is busy
    pmaddevobj->read_f = eIoPending;

    //Program the i/o
	CountBits = maddev_set_count_bits(count, MAD_CONTROL_IO_COUNT_MASK,
			                          MAD_CONTROL_IO_COUNT_SHIFT, MAD_UNITIO_SIZE_BYTES);
	CntlReg = (CountBits | MAD_CONTROL_IOSIZE_BYTES_BIT);

    #ifdef _MAD_SIMULATION_MODE_ 
    //Use the kmalloc'd buffer provided by the simulator
    pBufr = pmaddevobj->pSimParms->pInBufr;
    ASSERT((int)(pBufr != NULL));
    #endif
    //
    memset(pBufr, 0x00, count+2);
    maddev_program_stream_io(&pmaddevobj->devlock, pmadregs, 
                             CntlReg, IntEnable, virt_to_phys(pBufr), (U32)-1,0);

    //Wait for the i-o to complete
    rc = maddev_get_io_status((int)pmaddevobj->devnum, &pmaddevobj->read_q,
                              &pmaddevobj->read_f, &pmaddevobj->devlock,
                              &pmadregs->Status, &StatusReg);
    if (rc != 0)
        {//Release the device context semaphore and return the error code
    	up(&pmaddevobj->devsem);
    	return rc;
        }

    rdcount = maddev_get_io_count(StatusReg, MAD_STATUS_READ_COUNT_MASK,
    		                      MAD_STATUS_READ_COUNT_SHIFT, MAD_UNITIO_SIZE_BYTES);

    //Indicate the read queue is available
    pmaddevobj->read_f = eIoReset;

    //Release the device context semaphore
    up(&pmaddevobj->devsem);

    remains =
    copy_to_user(usrbufr, pBufr, (u32)rdcount); //possibly paged memory 
    if (remains > 0)
    {
    	PDEBUG("maddev_queued_read:copy_to_user returns (%d) bytes remaining... devnum=%d\n",
               (int)remains, (int)pmaddevobj->devnum);
    	return -EFAULT;
    }

    piov->count = rdcount;

	PDEBUG("maddev_queued_read completes... devnum=%d, (%d)bytes\n",
		   (int)pmaddevobj->devnum, (int)rdcount);

	return rdcount;
}

//This function implements a queued write through a kernel io control block
ssize_t maddev_queued_write(struct kiocb *pkiocb, struct iov_iter *piov)
{
	static ulong IntEnable = (MAD_INT_BUFRD_OUTPUT_BIT | MAD_INT_STATUS_ALERT_BIT);
	//
	PMADDEVOBJ pmaddevobj  = (PMADDEVOBJ)pkiocb->ki_filp->private_data;
	PMADREGS   pmadregs = pmaddevobj->pDevBase;
    size_t     count    = piov->count;
	u8* __user usrbufr  = piov->iov->iov_base;
    void*      pBufr = IoDataWr;
	//
	U32      CntlReg = 0;
    U32      StatusReg = 0;
	U32      CountBits  = 0;
	ssize_t  remains = 0;
    ssize_t  wrcount = 0;
    int      rc = 0;

	PNOTICE("maddev_queued_write... devnum=%d, kiocb=%px, piov=%px, count=%d, ub[1.2.3]=%c.%c.%c\n",
		   (int)pmaddevobj->devnum, pkiocb, piov, (int)count, usrbufr[0], usrbufr[1], usrbufr[2]);

    //If the write queue is not free we return
    if (pmaddevobj->write_f != eIoReset)
        {return -EAGAIN;}

    //Xfer the data...
    //
    #ifdef _MAD_SIMULATION_MODE_ 
    //Use the kmalloc'd buffer provided by the simulator
    pBufr = pmaddevobj->pSimParms->pOutBufr;
    ASSERT((int)(pBufr != NULL));
    #endif
    //
    memset(pBufr, 0x00, count+2);
    remains =
	copy_from_user(pBufr, usrbufr, (u32)count); //possibly paged memory - copy outside of spinlock
	if (remains > 0)
	    {
	    PDEBUG("maddev_queued_write:copy_from_user returns bytes remaining (%d), devnum=%d\n",
               (int)remains, (int)pmaddevobj->devnum);
	    return -EFAULT;
	    }

    //Acquire the semaphore for the device context
    down(&pmaddevobj->devsem);

    //Indicate the write queue is busy
    pmaddevobj->write_f = eIoPending;

    //Program the i/o
	CountBits = maddev_set_count_bits(count, MAD_CONTROL_IO_COUNT_MASK,
			                          MAD_CONTROL_IO_COUNT_SHIFT, MAD_UNITIO_SIZE_BYTES);
	CntlReg = (CountBits | MAD_CONTROL_IOSIZE_BYTES_BIT);

    maddev_program_stream_io(&pmaddevobj->devlock, pmadregs, 
                             CntlReg, IntEnable, virt_to_phys(pBufr), (U32)-1, 1);

    //Wait for the i-o to complete
    rc = maddev_get_io_status((int)pmaddevobj->devnum, &pmaddevobj->write_q,
                              &pmaddevobj->write_f, &pmaddevobj->devlock, 
                              &pmadregs->Status, &StatusReg);
    if (rc != 0)
        {//Release the device context semaphore and return the error code
        up(&pmaddevobj->devsem);
        return rc;
        }

    wrcount = 
    maddev_get_io_count(StatusReg, MAD_STATUS_WRITE_COUNT_MASK,
                        MAD_STATUS_WRITE_COUNT_SHIFT, MAD_UNITIO_SIZE_BYTES);

    //Indicate the write queue is available
    pmaddevobj->write_f = eIoReset;
    piov->count = wrcount;

    //Release the device context semaphore
    up(&pmaddevobj->devsem);

	PDEBUG("maddev_queued_write completes... devnum=%d, (%d)bytes\n",
		   (int)pmaddevobj->devnum, (int)wrcount);

    return wrcount;
}
#endif

//Do we have one fragmented page of sectors in various bio:bvecs ?
//One sector per bio_vec: one bio_vec per bio
int 
maddevb_collect_biovecs(struct mad_dev_obj *pmaddevobj, struct bio* pbio,
                        u32 nr_sectors, struct bio_vec* pbiovecs)
{
    struct bio* pbiot = pbio;
    struct bio_vec* pbiovect  = pbiot->bi_io_vec; 
    struct bio_vec* pbiovecst = pbiovecs; 
    //
    u32 j;
    u32 totl_sectors= 0;
    int totl_bvecs = 0;

    //PDEBUG("maddevb_collect_biovecs... dev#=%d bio=%px veccnt=%d len0=%d offset0=%d\n",
    //       pmaddevobj->devnum, pbio, pbio->bi_vcnt, 
    //       pbiovect->bv_len, pbiovect->bv_offset);

    while (pbiot != NULL)
        {
        pbiovect = pbiot->bi_io_vec;

        for (j=0; j < pbiot->bi_vcnt; j++)
            {
            ASSERT((int)((pbiovect->bv_len % MAD_SECTOR_SIZE) == 0));
            ASSERT((int)((pbiovect->bv_offset % MAD_SECTOR_SIZE) == 0));

            //Accumulate an array of biovecs for sgdma processing
            memcpy(pbiovecst, pbiovect, sizeof(struct bio_vec));
            totl_bvecs++;
            totl_sectors += (pbiovect->bv_len / MAD_SECTOR_SIZE);

            pbiovect++; 
            pbiovecst++; 
            }

        pbiot = pbiot->bi_next;
        }

    ASSERT((int)(totl_sectors == nr_sectors));

    PDEBUG("maddevb_collect_biovecs... dev#=%d bio=%px total_bvecs=%d, totl_sctrs=%d\n",
           pmaddevobj->devnum, pbio, (int)totl_bvecs, (int)totl_sectors);

    return totl_bvecs;
}

//Build the hardware scatter-gather list from the set of biovecs
ssize_t 
maddevb_xfer_sgdma_bvecs(PMADDEVOBJ pmaddevobj, struct bio_vec* biovecs,
                         long nr_bvecs, sector_t sector, u32 nr_sectors,
                         bool bWr)
{
    struct bio_vec* pbiovect = biovecs;
    U32 DevLoclAddr = (sector * MAD_SECTOR_SIZE);
    PMAD_DMA_CHAIN_ELEMENT pSgDmaElement = &pmaddevobj->SgDmaElements[0];
    U32    IntEnable = 
           (bWr) ? (MAD_INT_STATUS_ALERT_BIT | MAD_INT_DMA_OUTPUT_BIT) :
                   (MAD_INT_STATUS_ALERT_BIT | MAD_INT_DMA_INPUT_BIT);
    //
    U32    DmaCntl   = MAD_DMA_CNTL_INIT;
    U32    CntlReg   = MAD_CONTROL_CHAINED_DMA_BIT;
    dma_addr_t HostPA = 0;
    ssize_t iocount = 0;
    size_t  bvlen_totl = 0;
    u32  totl_sectors = 0;
    u32 flags1 = 0; 
    U32 flags2 = 0;
    U64    CDPP;
    long   iostat;
    U32 j;

    if (bWr)
        {DmaCntl |= MAD_DMA_CNTL_H2D;} //Write == Host2Disk

    PINFO("maddev_xfer_sgdma_bvecs... dev#=%d nr_bvecs=%d #sectors=%ld wr=%d\n",
          (int)pmaddevobj->devnum, nr_bvecs, nr_sectors, bWr);

    pmaddevobj->ioctl_f = eIoPending;

    maddev_acquire_lock_disable_ints(&pmaddevobj->devlock, flags1);
    //
    maddev_program_io_regs(pmaddevobj->pDevBase, CntlReg, IntEnable, HostPA);

    //Set the beginning of the Chained-DMA-Pkt-Pntr list
    writeq(virt_to_phys(pSgDmaElement), &pmaddevobj->pDevBase->BCDPP);

    for (j=1; j <= nr_bvecs; j++)
        {
        HostPA = maddev_page_to_dma(pmaddevobj->pdevnode, 
                                    pbiovect->bv_page, PAGE_SIZE, bWr);
        if (dma_mapping_error(pmaddevobj->pdevnode, HostPA))
            {
            PERR("maddev_xfer_sgdma_bvecs... dev#=%d returning -EADDRNOTAVAIL\n",
                  (int)pmaddevobj->devnum);
            return -EADDRNOTAVAIL;
            }

        HostPA += pbiovect->bv_offset;

        //The set of hardware SG elements is defined as a linked-list even though
        //it is created as an array
        //Set the next Chained-DMA-Pkt_Pntr address
        CDPP = (j == nr_bvecs) ? MAD_DMA_CDPP_END : 
                                 virt_to_phys(&pmaddevobj->SgDmaElements[j]);

        maddev_program_sgdma_regs(pSgDmaElement, HostPA, DevLoclAddr,
                                  DmaCntl, pbiovect->bv_len, CDPP);

        bvlen_totl += pbiovect->bv_len;
        totl_sectors += (pbiovect->bv_len / MAD_SECTOR_SIZE);
        DevLoclAddr += pbiovect->bv_len;
        ASSERT((int)((DevLoclAddr % MAD_SECTOR_SIZE) == 0));

        //We could use the next array element but instead we use the 
        //Chained-Dma-Pkt-Pntr because we treat the chained list as a
        //linked-list to be proper 
        //pSgDmaElement = &pmaddevobj->SgDmaElements[j];
        if (j < nr_bvecs)
            {pSgDmaElement = phys_to_virt(CDPP);}

        pbiovect++;
        }

    //Finally - let's go
    CntlReg |= MAD_CONTROL_DMA_GO_BIT;
    iowrite32(CntlReg, &pmaddevobj->pDevBase->Control);
    //
    maddev_enable_ints_release_lock(&pmaddevobj->devlock, flags1);

    //Wait and process the results
    iostat = maddev_get_io_status(pmaddevobj, &pmaddevobj->ioctl_q,
                                  &pmaddevobj->ioctl_f,
                                  &pmaddevobj->devlock);

    iocount = (iostat < 0) ? iostat : pmaddevobj->pDevBase->DTBC;
    if (iostat == 0)
        {
        ASSERT((int)(iocount == bvlen_totl));
        ASSERT((int)((bvlen_totl % MAD_SECTOR_SIZE) == 0));
        ASSERT((int)(totl_sectors == nr_sectors));
        }

    PDEBUG("maddev_xfer_sgdma_bvecs... dev#=%d #bvecs=%d #sectors=%d iostat=%ld iocount=%ld\n",
           (int)pmaddevobj->devnum, (int)nr_bvecs, (int)nr_sectors,
           (int)iostat, iocount);
    pmaddevobj->ioctl_f = eIoReset;

    return iocount;
}

//This is the Interrupt Service Routine which is established by a successful
//call of request_irq
//
irqreturn_t maddevb_isr(int irq, void* dev_id)
{
	PMADDEVOBJ pmaddevobj = (PMADDEVOBJ)dev_id;
	PMADREGS pmadregs;
	//
	u32 flags1 = 0;
    U32 flags2 = 0;
	u32 IntID = 0;

    ASSERT((int)(pmaddevobj != NULL));
    pmadregs = pmaddevobj->pDevBase;
	PDEBUG("maddevb_isr... dev#=%d pmaddevobj=%px irq=%d IntID=x%X\n",
           (int)pmaddevobj->devnum, pmaddevobj, irq, pmadregs->IntID);

    maddev_acquire_lock_disable_ints(&pmaddevobj->devlock, flags1);

    //Copy the device register state for the DPC
    memcpy_fromio(&pmaddevobj->IntRegState, pmadregs, sizeof(MADREGS));

    //Disable interrupts on this device
    iowrite32(MAD_ALL_INTS_DISABLED, &pmadregs->IntEnable);
    iowrite32(0, &pmadregs->MesgID);

    IntID = ioread32(&pmadregs->IntID);
    if (IntID == (u32)MAD_ALL_INTS_CLEARED)
        {   //This is not our IRQ
        maddev_enable_ints_release_lock(&pmaddevobj->devlock, flags1);

        PERR("maddevb_isr... invalid int recv'd dev#=%d IntID=x%X\n",
             (int)pmaddevobj->devnum, IntID);
    	return IRQ_NONE;
        }

    if (IntID == (u32)MAD_INT_INVALID_BYTEMODE_MASK)//Any / all undefined int conditions
        {
        maddev_enable_ints_release_lock(&pmaddevobj->devlock, flags1);

        PERR("maddevb_isr... undefined int-id from device dev#=%d IntID=x%X\n",
             (int)pmaddevobj->devnum, IntID);
    	return IRQ_HANDLED;
        }

	/* Invoke the DPC handler */
    #ifdef _MAD_SIMULATION_MODE_ 
    //Release the spinlock *NOT* at device-irql BEFORE enqueueing the DPC
    maddev_enable_ints_release_lock(&pmaddevobj->devlock, flags1);
    #endif

    /* Invoke the DPC handler */
    tasklet_hi_schedule(&pmaddevobj->dpctask);

    #ifndef _MAD_SIMULATION_MODE_(real hardware)
    //With real hardware release the spinlock at device-irql  AFTER enqueueing the DPC
    maddev_enable_ints_release_lock(&pmaddevobj->devlock, flags1);
    #endif

    PDEBUG("maddevc_isr... normal return dev#=%d IntID=x%X rc=%d\n",
           (int)pmaddevobj->devnum, IntID, IRQ_HANDLED);

	return IRQ_HANDLED;
}

//This is the DPC/software interrupt handler invoked by scheduling a tasklet
//for a task struct configured with this function as the DPC task
//
void maddevb_dpctask(ulong indx)
{
	PMADDEVOBJ  pmaddevobj = 
                (PMADDEVOBJ)((U8*)mad_dev_objects + (PAGE_SIZE * indx)); 
	PMADREGS    pmadregs   = pmaddevobj->pDevBase;
	u32 IntID     = pmaddevobj->IntRegState.IntID;
    u32 IntEnable = pmaddevobj->IntRegState.IntEnable;
	u32 Status    = pmaddevobj->IntRegState.Status & MAD_STATUS_ERROR_MASK;
	u32 Control   = pmaddevobj->IntRegState.Control;
	//
	long rc = 0;

	PINFO("maddevb_dpc... dev#=%d pmaddevobj=%px Control=x%X Status=x%X IntEnable=x%X IntID=x%X\n",
		  (int)pmaddevobj->devnum, pmaddevobj, Control, Status, IntEnable, IntID);

	rc = maddev_status_to_errno(pmaddevobj->devnum, &pmaddevobj->IntRegState);

    if ((IntID & MAD_INT_ALL_VALID_MASK) != 0)
        maddev_reset_io_registers(pmadregs, &pmaddevobj->devlock);

    if ((IntID & MAD_INT_DMA_INPUT_BIT) || (IntID & MAD_INT_DMA_OUTPUT_BIT))
        {
        if (pmaddevobj->ioctl_f == eIoPending)
            {
            pmaddevobj->ioctl_f = rc;
            wake_up(&pmaddevobj->ioctl_q);
            }
        else
            {
            PWARN("maddevb_dpc... dev#=%d Int unexpected! ioctl_f=%d\n",
                  (int)pmaddevobj->devnum, pmaddevobj->ioctl_f);
            ASSERT((int)false);
            }
        return;
        }

    //If we got here we got trouble
    PERR("maddevb_dpc... dev#=%d IntID not recognized! x%X\n",
              (int)pmaddevobj->devnum, IntID);
    ASSERT((int)false);
    BUG_ON(true);
    return;
}

#if 0
//These two work-items (passive-mode tasklets) are not used
void maddev_dpcwork_rd(struct work_struct *dpcwork)
{
	PMADDEVOBJ pmaddevobj = 
               container_of(dpcwork, struct mad_dev_obj, dpc_work_rd);
	struct async_work  *rd_workq = &pmaddevobj->rd_workq;
	//
	size_t iocount = 0;
    size_t remains = 0;
   	u8*  __user  usrbufr;
    int rc = 0;

    //Do we have a pending read iocb
    if (rd_workq->piocb == NULL)
        {PDEBUG("maddev_dpcwork: no active read iocb:iov_iter... dev=%d\n",
                (int)pmaddevobj->devnum);}
    else
        {
        rc = maddev_status_to_errno(pmaddevobj->devnum, &pmaddevobj->IntRegState);
        iocount = maddev_get_io_count(pmaddevobj->IntRegState.Status, MAD_STATUS_READ_COUNT_MASK,
        		                      MAD_STATUS_READ_COUNT_SHIFT, MAD_UNITIO_SIZE_BYTES);
        PDEBUG("maddev_dpcwork: (read) devnum=%d, piocb=%px, piov=%px, iocount=%d, rc=%d\n",
               (int)pmaddevobj->devnum, rd_workq->piocb, rd_workq->piov, (int)iocount, rc);
        
        if (rc == 0)
            {
            usrbufr = rd_workq->piov->iov->iov_base;
            remains = copy_to_user(usrbufr, IoDataRd, (U32)iocount); //possibl
            rc = (remains > 0) ? -EFAULT : 0;
            }

        rd_workq->piov->count = iocount;
        rd_workq->piocb->ki_complete(rd_workq->piocb, rc, 0);
        //
        rd_workq->piocb = NULL;
        rd_workq->piov  = NULL;
        }
 
	return;
}
//
void maddev_dpcwork_wr(struct work_struct *dpcwork)
{
	PMADDEVOBJ pmaddevobj = 
               container_of(dpcwork, struct mad_dev_obj, dpc_work_wr);
	struct async_work  *wr_workq = &pmaddevobj->wr_workq;
	//
	size_t iocount = 0;
    int rc = 0;

    //Do we have a pending write iocb
    if (wr_workq->piocb == NULL)
        {PDEBUG("maddev_dpcwork: no active write iocb:iov_iter... dev=%d\n",
                (int)pmaddevobj->devnum);}
    else
        {
        rc = maddev_status_to_errno(pmaddevobj->devnum, &pmaddevobj->IntRegState);
        iocount = maddev_get_io_count(pmaddevobj->IntRegState.Status, MAD_STATUS_WRITE_COUNT_MASK,
        		                      MAD_STATUS_WRITE_COUNT_SHIFT, MAD_UNITIO_SIZE_BYTES);
        PDEBUG("maddev_dpcwork: (write) devnum=%d, piocb=%px, piov=%px, iocount=%d, rc=%d\n",
               (int)pmaddevobj->devnum, wr_workq->piocb, wr_workq->piov, (int)iocount, rc);
        
        wr_workq->piov->count = iocount;
        //wr_workq->piov->iov->iov_len = iocount;
        wr_workq->piocb->ki_complete(wr_workq->piocb, rc, 0);
        //
        wr_workq->piocb = NULL;
        wr_workq->piov  = NULL;
        }
 
	return;
}
//#if 0
void maddev_complete_io(struct work_struct *work)
{
	struct async_work *qio_work = container_of(work, struct async_work, work.work);
	//aio_complete(qio_work->iocb, qio_work->result, 0);
	qio_work->iocb->ki_complete(qio_work->iocb, 0, 0);
} 
#endif



