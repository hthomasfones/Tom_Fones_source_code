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

//static u8   IoDataRd[MAD_UNITIO_SIZE_BYTES * (1 << MAD_BUFRD_IO_COUNT_BITS)];
//static u8   IoDataWr[MAD_UNITIO_SIZE_BYTES * (1 << MAD_BUFRD_IO_COUNT_BITS)];

//This function implements a queued read through a kernel io control block
#if 0
#endif


//This function implements a direct-io read into a user mode buffer
ssize_t
maddevb_direct_io(struct file *filp, const char __user *usrbufr, 
                 size_t count, loff_t *f_pos, bool bWr)
{
    PMADDEVOBJ pmaddevobj = filp->private_data;
    u32        pg_cnt = (MAD_MODULO_ADJUST(count, PAGE_SIZE) / PAGE_SIZE);
    u32        offset = MAD_MODULO_ADJUST(*f_pos, MAD_SECTOR_SIZE);
    sector_t   sector = (offset / MAD_SECTOR_SIZE);
    U64        start_page = MAD_MODULO_ADJUST((U64)usrbufr, PAGE_SIZE);
    U8         IoQbusy = (bWr) ? (pmaddevobj->write_f != eIoReset) :
                                 (pmaddevobj->read_f != eIoReset);
    bool       bUpdate = (bWr==true) ? false : true;
    U8     bNeedSg = false;   
    ssize_t lrc = -EFAULT; //bad address
    //
	struct page* pPages[MAD_DIRECT_XFER_MAX_PAGES]; 
    struct vm_area_struct* pVMAs[MAD_DIRECT_XFER_MAX_PAGES];
	u32    num_pgs;
    ssize_t  iocount;

    BUG_ON(!(virt_addr_valid(pmaddevobj)));

    PINFO("maddev_direct_io... dev#=%d fp=%px offset=%ld sector=%ld count=%ld\n",
		  (int)pmaddevobj->devnum, filp, (long int)offset, 
          (long int)sector, (long int)count);

    if (((U32)count + offset) >= MAD_DEVICE_DATA_SIZE)
        {return lrc;}

    //If the io queue is not free we return
    if (IoQbusy)
        {
        lrc = -EAGAIN;
        PERR("maddev_direct_io... dev#=%d Io-Q busy rc=%ld\n",
             (int)pmaddevobj->devnum, (long int)lrc);
        return lrc;
        }

    num_pgs = maddev_get_user_pages(start_page, pg_cnt, pPages, pVMAs, bUpdate);
    if (num_pgs != pg_cnt)
        {
        PERR("maddev_direct_io:get_user_pages... dev#=%d num_pgs=%ld rc=%ld\n",
		     (int)pmaddevobj->devnum, (long int)num_pgs, (long int)lrc);
        return lrc;
        }

    bNeedSg = maddev_need_sg(pPages, num_pgs);

    //Acquire the device context semaphore, call the helper function,
    //release the device context semaphore
    if (bNeedSg) 
        {iocount = maddev_xfer_sgdma_pages(pmaddevobj, num_pgs, pPages, sector, bWr);}
    else
        //{iocount = maddev_xfer_pages_direct(pmaddevobj, num_pgs, pPages, offset, bWr);}
        {iocount = maddev_xfer_dma_page(pmaddevobj, pPages[0], sector, bWr);}

    if (bWr==false)
        {maddev_set_dirty_pages(pPages, num_pgs);}

    maddev_put_user_pages(pPages, num_pgs);

    PDEBUG("maddev_direct_io... dev#=%d offset=%ld lrc(iocount)=%ld \n",
           (int)pmaddevobj->devnum, (long int)offset, (long int)iocount);

    return iocount;
}

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

    PDEBUG("maddevb_collect_biovecs... dev#=%d bio=%px total_bvecs=%d, totl_sctrs=%ld\n",
           (int)pmaddevobj->devnum, (void *)pbio, totl_bvecs, (long int)totl_sectors);

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
    U64    CDPP;
    long   iostat;
    long int lrc = -EADDRNOTAVAIL;
    U32 j;

    if (bWr)
        {DmaCntl |= MAD_DMA_CNTL_H2D;} //Write == Host2Disk

    PINFO("maddev_xfer_sgdma_bvecs... dev#=%d nr_bvecs=%ld #sectors=%ld wr=%d\n",
          (int)pmaddevobj->devnum, nr_bvecs, (long int)nr_sectors, bWr);

    pmaddevobj->ioctl_f = eIoPending;

    maddev_acquire_lock_disable_ints(&pmaddevobj->devlock, flags1);
    //
    maddev_program_io_regs(pmaddevobj->pDevBase, CntlReg, IntEnable, HostPA);

    //Set the beginning of the Chained-DMA-Pkt-Pntr list
    writeq(virt_to_phys(pSgDmaElement), &pmaddevobj->pDevBase->BCDPP);

    for (j=1; j <= nr_bvecs; j++)
        {
        HostPA = maddev_dma_map_page(pmaddevobj->pdevnode, 
                                     pbiovect->bv_page, PAGE_SIZE, 0, bWr);
        if (dma_mapping_error(pmaddevobj->pdevnode, HostPA))
            {
            maddev_enable_ints_release_lock(&pmaddevobj->devlock, flags1);
            PERR("maddev_xfer_sgdma_bvecs... dev#=%d rc=%ld\n",
                  (int)pmaddevobj->devnum, lrc);
            return lrc;
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

    //Finally - initiate the i/o
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

    for (j=0; j < nr_bvecs; j++)
        {
        maddev_dma_unmap_page(pmaddevobj->pdevnode, 
                              pmaddevobj->SgDmaElements[j].HostAddr, PAGE_SIZE, bWr);
        }

    PDEBUG("maddev_xfer_sgdma_bvecs... dev#=%d #bvecs=%d #sectors=%d iostat=%ld iocount=%ld\n",
           (int)pmaddevobj->devnum, (int)nr_bvecs, (int)nr_sectors,
           iostat, iocount);
    pmaddevobj->ioctl_f = eIoReset;

    return iocount;
}

//This is the Interrupt Service Routine which is established by a successful
//call of request_irq
irqreturn_t maddevb_legacy_isr(int irq, void* dev_id)
{
    return maddevb_isr_worker_fn(irq, dev_id, 0);
}

//These are the msi Interrupt Service Routines established by
// calls to indx=%drequest_irq
irqreturn_t maddevb_msi_one_isr(int irq, void* dev_id)
{
    return maddevb_isr_worker_fn(irq, dev_id, 1);
}

irqreturn_t maddevb_msi_two_isr(int irq, void* dev_id)
{
    return maddevb_isr_worker_fn(irq, dev_id, 2);
}

irqreturn_t maddevb_msi_three_isr(int irq, void* dev_id)
{
    return maddevb_isr_worker_fn(irq, dev_id, 3);
}

irqreturn_t maddevb_msi_four_isr(int irq, void* dev_id)
{
    return maddevb_isr_worker_fn(irq, dev_id, 4);
}

irqreturn_t maddevb_msi_five_isr(int irq, void* dev_id)
{
    return maddevb_isr_worker_fn(irq, dev_id, 5);
}

irqreturn_t maddevb_msi_six_isr(int irq, void* dev_id)
{
    return maddevb_isr_worker_fn(irq, dev_id, 6);
}

irqreturn_t maddevb_msi_seven_isr(int irq, void* dev_id)
{
    return maddevb_isr_worker_fn(irq, dev_id, 7);
}

irqreturn_t maddevb_msi_eight_isr(int irq, void* dev_id)
{
    return maddevb_isr_worker_fn(irq, dev_id, 8);
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
	u32 Status    = pmaddevobj->IntRegState.Status & MAD_STATUS_ERROR_MASK;
	u32 Control   = pmaddevobj->IntRegState.Control;
	//
	long rc = 0;

	PINFO("maddevb_dpc... indx=%d pmaddevobj=%px dev#=%d IntID=x%lX Control=x%lX Status=x%lX \n",
		  (int)indx, (void *)pmaddevobj, (int)pmaddevobj->devnum, 
          (unsigned long)IntID, (unsigned long)Control, (unsigned long)Status);

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
                  (int)pmaddevobj->devnum, (int)pmaddevobj->ioctl_f);
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



