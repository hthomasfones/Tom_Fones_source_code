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
/*  Exe files   : maddevc.ko                                                   */ 
/*                                                                             */
/*  Module NAME : maddevcio.c                                                  */
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
/* $Id: maddevcio.c, v 1.0 2021/01/01 00:00:00 htf $                           */
/*                                                                             */
/*******************************************************************************/

#include "maddevc.h"		/* local definitions */

extern struct mad_dev_obj *mad_dev_objects;

static u8   IoDataRd[MAD_UNITIO_SIZE_BYTES * (1 << MAD_BUFRD_IO_COUNT_BITS)];
static u8   IoDataWr[MAD_UNITIO_SIZE_BYTES * (1 << MAD_BUFRD_IO_COUNT_BITS)];

//This is the buffered read function
ssize_t maddev_read_bufrd(struct file *fp, char __user *usrbufr,
                          size_t count, loff_t *f_pos)
{
	static u8   IoData[MAD_UNITIO_SIZE_BYTES * (1 << MAD_BUFRD_IO_COUNT_BITS)];
	static u32  IntEnable = (MAD_INT_BUFRD_INPUT_BIT | MAD_INT_STATUS_ALERT_BIT);
    static U32 offset = 0;
	//
	struct mad_dev_obj *pmaddevobj = fp->private_data;
	PMADREGS  pmadregs = pmaddevobj->pDevBase;
    //
    void*    pBufr = IoData;
    U32      CntlReg = 0;
	U32      CountBits = 0;
	ssize_t  rdcount = 0;
	size_t   remains = 0;
	int      rc = 0;

    PDEBUG("maddev_read_bufrd... dev#=%d fp=%p count=%ld\n",
		   (int)pmaddevobj->devnum, fp, (U32)count);

    //If the read queue is not free we return
    if (pmaddevobj->read_f != eIoReset)
         {return -EAGAIN;}

    //Indicate the read queue is busy
   	pmaddevobj->read_f = eIoPending;

    //Program the i/o
	CountBits = 
    maddev_set_count_bits((U32)count, MAD_CONTROL_IO_COUNT_MASK,
                          MAD_CONTROL_IO_COUNT_SHIFT, MAD_UNITIO_SIZE_BYTES);
    CntlReg = CountBits | MAD_CONTROL_IOSIZE_BYTES_BIT;

    #ifdef _MAD_SIMULATION_MODE_ 
    //Use the kmalloc'd buffer provided by the simulator
    pBufr = pmaddevobj->pSimParms->pInBufr;
    ASSERT((int)(pBufr != NULL));
    #endif
    
    memset(pBufr, 0x00, count+2);
    maddevc_program_stream_io(&pmaddevobj->devlock, pmadregs, CntlReg, IntEnable,
                             virt_to_phys(pBufr), offset, false);

    //Wait and process the results
    rc = maddev_get_io_status(pmaddevobj, &pmaddevobj->read_q, 
                              &pmaddevobj->read_f, &pmaddevobj->devlock);
    if (rc != 0)
        {return rc;}

    rdcount = maddev_get_io_count(pmaddevobj->IntRegState.Status, 
                                  MAD_STATUS_READ_COUNT_MASK,
                                  MAD_STATUS_READ_COUNT_SHIFT,
                                  MAD_UNITIO_SIZE_BYTES);

    //Indicate the read queue is available
    pmaddevobj->read_f = eIoReset;

    remains =
    copy_to_user(usrbufr, pBufr, (u32)rdcount); //possibly paged memory - copy outside of spinlock
    if (remains > 0)
        {
    	PERR("maddev_read:copy_to_user... dev#=%d returns (%d) bytes remaining\n",
              (int)pmaddevobj->devnum, (int)remains);
    	return -EFAULT;
        }

    PDEBUG("maddev_read_bufrd completes... dev#=%d count=%ld\n",
           (int)pmaddevobj->devnum, rdcount);

	return rdcount;
}

//This function implements a queued read through a kernel io control block
ssize_t maddev_queued_read(struct kiocb *pkiocb, struct iov_iter *piov)
{
	static u32  IntEnable = (MAD_INT_BUFRD_INPUT_BIT | MAD_INT_STATUS_ALERT_BIT);
    static U32 offset = 0;
	//
	PMADDEVOBJ pmaddevobj = (PMADDEVOBJ)pkiocb->ki_filp->private_data;
	PMADREGS   pmadregs = pmaddevobj->pDevBase;
    size_t     count   = piov->count;
    u8* __user usrbufr = piov->iov->iov_base;
    void*      pBufr = IoDataRd;
    //
	U32      CntlReg = 0;
	U32      CountBits = 0;
	long     iostat = 0;
    ssize_t  rdcount;
    size_t   remains = 0;

    PNOTICE("maddev_queued_read... dev#=%d kiocb=%px piov=%px count=%d\n",
           (int)pmaddevobj->devnum, pkiocb, piov, (int)count);

    //If the read queue is not free we return
    if (pmaddevobj->read_f != eIoReset)
        {return -EAGAIN;}

    mutex_lock(&pmaddevobj->devmutex);

    //Indicate the read queue is busy
    pmaddevobj->read_f = eIoPending;

    //Program the i/o
	CountBits = maddev_set_count_bits(count, MAD_CONTROL_IO_COUNT_MASK,
			                          MAD_CONTROL_IO_COUNT_SHIFT,
                                      MAD_UNITIO_SIZE_BYTES);
	CntlReg = (CountBits | MAD_CONTROL_IOSIZE_BYTES_BIT);

    #ifdef _MAD_SIMULATION_MODE_ 
    //Use the kmalloc'd buffer provided by the simulator
    pBufr = pmaddevobj->pSimParms->pInBufr;
    ASSERT((int)(pBufr != NULL));
    #endif
    //
    memset(pBufr, 0x00, count+2);
    maddevc_program_stream_io(&pmaddevobj->devlock, pmadregs, CntlReg,IntEnable,
                             virt_to_phys(pBufr), offset, false);

    //Wait for the i-o to complete
    iostat = maddev_get_io_status(pmaddevobj, &pmaddevobj->read_q,
                                  &pmaddevobj->read_f, &pmaddevobj->devlock);
    if (iostat < 0)
        {//Release the device context semaphore and return the error code
        mutex_unlock(&pmaddevobj->devmutex);
        return iostat;
        }

    rdcount = maddev_get_io_count(pmaddevobj->IntRegState.Status, 
                                  MAD_STATUS_READ_COUNT_MASK,
                                  MAD_STATUS_READ_COUNT_SHIFT,
                                  MAD_UNITIO_SIZE_BYTES);

    //Indicate the read queue is available
    pmaddevobj->read_f = eIoReset;

    mutex_unlock(&pmaddevobj->devmutex);

    remains =
    copy_to_user(usrbufr, pBufr, (u32)rdcount); //possibly paged memory 
    if (remains > 0)
        {
    	PERR("maddev_queued_read:copy_to_user returns... dev#=%d bytes_remaining=%ld rc=-EFAULT\n",
             (int)pmaddevobj->devnum, remains);
    	return -EFAULT;
        }

    piov->count = rdcount;

	PINFO("maddev_queued_read completes... dev#=%d count=%ld\n",
		  (int)pmaddevobj->devnum, rdcount);

	return rdcount;
}

//This is the buffered write function
ssize_t maddev_write_bufrd(struct file *fp, const char __user *usrbufr, 
                           size_t count, loff_t *f_pos)
{
	static 	u8   IoData[MAD_UNITIO_SIZE_BYTES * (1 << MAD_BUFRD_IO_COUNT_BITS)];
	static ulong IntEnable = 
                 (MAD_INT_BUFRD_OUTPUT_BIT | MAD_INT_STATUS_ALERT_BIT);
    static U32 offset = 0;
	//
	struct mad_dev_obj *pmaddevobj = fp->private_data;
	PMADREGS  pmadregs = pmaddevobj->pDevBase;
	//
    u8*     pBufr = IoData;
	U32      CntlReg = 0;
	U32      CountBits  = 0;
	size_t   wrcount;
	ssize_t  remains = 0;
    int      rc;

    PINFO("maddev_write_bufrd... dev#=%d fp=%p count=%ld\n",
		  (int)pmaddevobj->devnum, fp, count);

    //If the write queue is not free we return
    if (pmaddevobj->write_f != eIoReset)
        {return -EAGAIN;}

    //Initialize & Xfer the data
    #ifdef _MAD_SIMULATION_MODE_ 
    //Use the kmalloc'd buffer provided by the simulator
    pBufr = pmaddevobj->pSimParms->pOutBufr;
    ASSERT((int)(pBufr != NULL));
    #endif

    memset(pBufr, 0x00, count+2);
    remains =
	copy_from_user(pBufr, usrbufr, (u32)count); //possibly paged memory
	if (remains > 0)
	   {
	   PERR("maddev_write:copy_from_user... dev#=%d - (%d) bytes remaining\n",
            (int)pmaddevobj->devnum, (int)count);
	   return -EFAULT;
	   }

    //PDEBUG("maddev_write_bufrd... dev#=%d fp=%p count=%ld pBufr[0.1.2.3]=%c.%c.%c.%c\n",
    //       (int)pmaddevobj->devnum, fp, count, pBufr[0], pBufr[1], pBufr[2], pBufr[3]);

    //Indicate the write queue is busy
    pmaddevobj->write_f = eIoPending;

    //Program the i/o
    CountBits = 
    maddev_set_count_bits(count, MAD_CONTROL_IO_COUNT_MASK,
                          MAD_CONTROL_IO_COUNT_SHIFT, MAD_UNITIO_SIZE_BYTES);
    CntlReg = CountBits | MAD_CONTROL_IOSIZE_BYTES_BIT;
    //
    maddevc_program_stream_io(&pmaddevobj->devlock, pmadregs, CntlReg, IntEnable,
                             virt_to_phys(pBufr), offset, true);

    //Wait and process the results
    rc = maddev_get_io_status(pmaddevobj, &pmaddevobj->write_q,
                              &pmaddevobj->write_f, &pmaddevobj->devlock);
    if (rc != 0)
        {return rc;}

    wrcount = maddev_get_io_count(pmaddevobj->IntRegState.Status, 
                                  MAD_STATUS_WRITE_COUNT_MASK,
                                  MAD_STATUS_WRITE_COUNT_SHIFT,
                                  MAD_UNITIO_SIZE_BYTES);

    pmaddevobj->write_f = eIoReset;

	PDEBUG("maddev_write_bufrd completes... dev#=%d count=%ld\n",
		   (int)pmaddevobj->devnum, wrcount);

	return wrcount;
}

//This function implements a queued write through a kernel io control block
//
ssize_t maddev_queued_write(struct kiocb *pkiocb, struct iov_iter *piov)
{
	static ulong IntEnable = (MAD_INT_BUFRD_OUTPUT_BIT | MAD_INT_STATUS_ALERT_BIT);
    static U32 offset = 0;
	//
	PMADDEVOBJ pmaddevobj  = (PMADDEVOBJ)pkiocb->ki_filp->private_data;
	PMADREGS   pmadregs = pmaddevobj->pDevBase;
    size_t     count    = piov->count;
	u8* __user usrbufr  = piov->iov->iov_base;
    void*      pBufr = IoDataWr;
	//
	U32      CntlReg = 0;
	U32      CountBits  = 0;
	ssize_t  remains = 0;
    ssize_t  wrcount = 0;
    long     iostat = 0;

	PINFO("maddev_queued_write... dev#=%d kiocb=%px piov=%px count=%ld\n",
		  (int)pmaddevobj->devnum, pkiocb, piov, count);

    //If the write queue is not free we return
    if (pmaddevobj->write_f != eIoReset)
        {return -EAGAIN;}

    //Xfer the data...
    #ifdef _MAD_SIMULATION_MODE_ 
    //Use the kmalloc'd buffer provided by the simulator
    pBufr = pmaddevobj->pSimParms->pOutBufr;
    ASSERT((int)(pBufr != NULL));
    #endif

    memset(pBufr, 0x00, count+2);
    remains =
	copy_from_user(pBufr, usrbufr, (u32)count); //possibly paged memory - copy outside of spinlock
	if (remains > 0)
	    {
	    PERR("maddev_queued_write:copy_from_user... dev#=%d bytes_remaining=%ld rc=-EFAULT \n",
             (int)pmaddevobj->devnum, remains);
	    return -EFAULT;
	    }

    //PDEBUG("maddev_queued_write... dev#=%d kiocb=%px piov=%px count=%ld ub[0.1.2.3]=%c.%c.%c.%c\n",
    //       (int)pmaddevobj->devnum, pkiocb, piov, count,
    //       usrbufr[0], usrbufr[1], usrbufr[2], usrbufr[3]);

    mutex_lock(&pmaddevobj->devmutex);

    //Indicate the write queue is busy
    pmaddevobj->write_f = eIoPending;

    //Program the i/o
	CountBits = 
    maddev_set_count_bits(count, MAD_CONTROL_IO_COUNT_MASK,
                          MAD_CONTROL_IO_COUNT_SHIFT, MAD_UNITIO_SIZE_BYTES);
	CntlReg = (CountBits | MAD_CONTROL_IOSIZE_BYTES_BIT);

    maddevc_program_stream_io(&pmaddevobj->devlock, pmadregs, CntlReg, IntEnable,
                             virt_to_phys(pBufr), offset, true);

    //Wait for the i-o to complete
    iostat = maddev_get_io_status(pmaddevobj, &pmaddevobj->write_q,
                                  &pmaddevobj->write_f, &pmaddevobj->devlock);
    if (iostat < 0)
        {//Release the device context semaphore and return the error code
        mutex_unlock(&pmaddevobj->devmutex);
        return iostat;
        }

    wrcount = maddev_get_io_count(pmaddevobj->IntRegState.Status, 
                                  MAD_STATUS_WRITE_COUNT_MASK,
    		                      MAD_STATUS_WRITE_COUNT_SHIFT,
                                  MAD_UNITIO_SIZE_BYTES);

    //Indicate the write queue is available
    pmaddevobj->write_f = eIoReset;
    piov->count = wrcount;

    mutex_unlock(&pmaddevobj->devmutex);

	PINFO("maddev_queued_write completes... dev#=%d count=%ld\n",
		  (int)pmaddevobj->devnum, wrcount);

    return wrcount;
}

//This function implements a direct-io read into a user mode buffer
//
ssize_t
maddev_direct_io(struct file *filp, const char __user *usrbufr, 
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
    //
	struct page* pPages[MAD_DIRECT_XFER_MAX_PAGES]; 
    struct vm_area_struct* pVMAs[MAD_DIRECT_XFER_MAX_PAGES];
	u32    num_pgs;
    ssize_t  iocount;
    U8     bNeedSg = false;   
    ssize_t lrc = -EFAULT; //bad address

    BUG_ON(!(virt_addr_valid(pmaddevobj)));

    PINFO("maddev_direct_io... dev#=%d fp=%px count=%ld offset=%ld\n",
		  (int)pmaddevobj->devnum, filp, (long int)count, (long int)offset);

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
        {iocount = 
         maddev_xfer_sgdma_pages(pmaddevobj, num_pgs, pPages, sector, bWr);}
    else
        {iocount = 
         maddev_xfer_pages_direct(pmaddevobj, num_pgs, pPages, offset, bWr);}

    if (bWr==false)
        {maddev_set_dirty_pages(pPages, num_pgs);}

    maddev_put_user_pages(pPages, num_pgs);

    PDEBUG("maddev_direct_io... dev#=%d iocount=%ld offset=%ld\n",
           (int)pmaddevobj->devnum, (long int)iocount, (long int)offset);

    return iocount;
}

//This is the Interrupt Service Routine which is established by a successful
//call of request_irq
irqreturn_t maddevc_legacy_isr(int irq, void* dev_id)
{
    return maddevc_isr_worker_fn(irq, dev_id, 0);
}

//These are the msi Interrupt Service Routines established by multiple 
// calls of request_irq
irqreturn_t maddevc_msi_one_isr(int irq, void* dev_id)
{
    return maddevc_isr_worker_fn(irq, dev_id, 1);
}

irqreturn_t maddevc_msi_two_isr(int irq, void* dev_id)
{
    return maddevc_isr_worker_fn(irq, dev_id, 2);
}

irqreturn_t maddevc_msi_three_isr(int irq, void* dev_id)
{
    return maddevc_isr_worker_fn(irq, dev_id, 3);
}

irqreturn_t maddevc_msi_four_isr(int irq, void* dev_id)
{
    return maddevc_isr_worker_fn(irq, dev_id, 4);
}

irqreturn_t maddevc_msi_five_isr(int irq, void* dev_id)
{
    return maddevc_isr_worker_fn(irq, dev_id, 5);
}

irqreturn_t maddevc_msi_six_isr(int irq, void* dev_id)
{
    return maddevc_isr_worker_fn(irq, dev_id, 6);
}

irqreturn_t maddevc_msi_seven_isr(int irq, void* dev_id)
{
    return maddevc_isr_worker_fn(irq, dev_id, 7);
}

irqreturn_t maddevc_msi_eight_isr(int irq, void* dev_id)
{
    return maddevc_isr_worker_fn(irq, dev_id, 8);
}

//This is the DPC/software interrupt handler invoked by scheduling a tasklet
//for a task struct configured with this function as the DPC task
//
void maddevc_dpctask(ulong indx)
{
	PMADDEVOBJ  pmaddevobj =  
                (PMADDEVOBJ)((U8*)mad_dev_objects + (PAGE_SIZE * indx)); 
	PMADREGS    pmadregs   = pmaddevobj->pDevBase;
	u32 IntID   = pmaddevobj->IntRegState.IntID;
	u32 Status  = pmaddevobj->IntRegState.Status & MAD_STATUS_ERROR_MASK;
	u32 Control = pmaddevobj->IntRegState.Control;
	//
	long rc = 0;

	PDEBUG("maddevc_dpc... indx=%d pmaddevobj=%px dev#=%d IntID=x%X Control=x%X Status=x%X\n",
		   (int)indx, pmaddevobj, (int)pmaddevobj->devnum, IntID, Control, Status);

	rc = maddev_status_to_errno(pmaddevobj->devnum, &pmaddevobj->IntRegState);

    if ((IntID & MAD_INT_ALL_VALID_MASK) != 0)
        {maddev_reset_io_registers(pmadregs, &pmaddevobj->devlock);}

	//Is the completing i/o an ioctl ?
	if ((Control & MAD_CONTROL_CACHE_XFER_BIT)  ||
        (Control & MAD_CONTROL_CHAINED_DMA_BIT) ||
		(IntID & MAD_INT_ALIGN_INPUT_BIT)       ||
		(IntID & MAD_INT_ALIGN_OUTPUT_BIT))    
	    {
	    pmaddevobj->ioctl_f = rc;
	    wake_up(&pmaddevobj->ioctl_q);
	    return;
	    }

	//Is the completing i/o a buffered read ?
	if (IntID & MAD_INT_BUFRD_INPUT_BIT)
	    {
        if (pmaddevobj->read_f == eIoPending)
            {
       	    pmaddevobj->read_f = rc;
	        wake_up(&pmaddevobj->read_q);
            return;
            }
	    }

	//Is the completing i/o a buffered write ?
	if (IntID & MAD_INT_BUFRD_OUTPUT_BIT)
	    {
        if (pmaddevobj->write_f == eIoPending)
            {
	        pmaddevobj->write_f = rc;
	        wake_up(&pmaddevobj->write_q);
            return;
            }
	    }

    //If we got here we got trouble
    PERR("maddevc_dpc... dev#=%d IntID not recognized! x%X\n",
         (int)pmaddevobj->devnum, IntID);
    //ASSERT((int)false);
    //BUG_ON(true);
	return;
}

#if 0
//These two work-items (passive-mode tasklets) are not used
//
void maddev_dpcwork_rd(struct work_struct *dpcwork)
{
	PMADDEVOBJ pmaddevobj = container_of(dpcwork, struct mad_dev_obj, dpc_work_rd);
	struct async_work  *rd_workq = &pmaddevobj->rd_workq;
	//
	size_t iocount = 0;
    size_t remains = 0;
   	u8*  __user  usrbufr;
    int rc = 0;

    //Do we have a pending read iocb
    if (rd_workq->piocb == NULL)
        {PDEBUG(
               "maddev_dpcwork: no active read iocb:iov_iter... dev=%d\n",
                (int)pmaddevobj->devnum);}
    else
        {
        rc = maddev_status_to_errno(pmaddevobj->devnum, &pmaddevobj->IntRegState);
        iocount = maddev_get_io_count(pmaddevobj->IntRegState.Status, MAD_STATUS_READ_COUNT_MASK,
        		             MAD_STATUS_READ_COUNT_SHIFT, MAD_UNITIO_SIZE_BYTES);
        PDEBUG( 
               "maddev_dpcwork: (read) dev#=%d, piocb=%p, piov=%p, iocount=%d, rc=%d\n",
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
	PMADDEVOBJ pmaddevobj = container_of(dpcwork, struct mad_dev_obj, dpc_work_wr);
	struct async_work  *wr_workq = &pmaddevobj->wr_workq;
	//
	size_t iocount = 0;
    int rc = 0;

    //Do we have a pending write iocb
    if (wr_workq->piocb == NULL)
        {PDEBUG(
                "maddev_dpcwork: no active write iocb:iov_iter... dev=%d\n",
                (int)pmaddevobj->devnum);}
    else
        {
        rc = maddev_status_to_errno(pmaddevobj->devnum, &pmaddevobj->IntRegState);
        iocount = maddev_get_io_count(pmaddevobj->IntRegState.Status, MAD_STATUS_WRITE_COUNT_MASK,
        		             MAD_STATUS_WRITE_COUNT_SHIFT, MAD_UNITIO_SIZE_BYTES);
        PDEBUG( 
               "maddev_dpcwork: (write) dev#=%d, piocb=%p, piov=%p, iocount=%d, rc=%d\n",
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
void maddev_complete_io(struct work_struct *work)
{
	struct async_work *qio_work = container_of(work, struct async_work, work.work);
	//aio_complete(qio_work->iocb, qio_work->result, 0);
	qio_work->iocb->ki_complete(qio_work->iocb, 0, 0);
} 
#endif



