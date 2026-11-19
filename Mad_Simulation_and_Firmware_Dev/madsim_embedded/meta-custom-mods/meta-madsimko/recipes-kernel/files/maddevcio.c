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
	struct mad_dev_obj *pmaddev = fp->private_data;
	__iomem MADREGS  *pmadregs = pmaddev->pDevBase;
    PMADREGS  pIntState = (PMADREGS)&pmaddev->IntRegState;
    //
    void*    pBufr = IoData;
    U32      CntlReg = 0;
	U32      CountBits = 0;
	ssize_t  rdcount = 0;
	size_t   remains = 0;
	int      rc = 0;

    PINFO("maddev_read_bufrd... dev#=%d fp=%p count=%u\n",
		  (int)pmaddev->devnum, fp, (uint32_t)count);

    //If the read queue is not free we return
    if (pmaddev->read_f != eIoReset)
         {return -EAGAIN;}

    //Indicate the read queue is busy
   	pmaddev->read_f = eIoPending;

    //Program the i/o
	CountBits = 
    maddev_set_count_bits((U32)count, MAD_CONTROL_IO_COUNT_MASK,
                          MAD_CONTROL_IO_COUNT_SHIFT, MAD_UNITIO_SIZE_BYTES);
    CntlReg = CountBits | MAD_CONTROL_IOSIZE_BYTES_BIT;

    #ifdef _MAD_SIMULATION_MODE_ 
    //Use the kmalloc'd buffer provided by the simulator
    pBufr = pmaddev->pSimParms->pInBufr;
    ASSERT((int)(pBufr != NULL));
    #endif
    
    memset(pBufr, 0x00, count+2);
    maddevc_program_stream_io(&pmaddev->devlock, pmadregs, CntlReg, IntEnable,
                              virt_to_phys(pBufr), offset, false);

    //Wait and process the results
    rc = maddev_wait_get_io_status(pmaddev, &pmaddev->read_q, 
                                   &pmaddev->read_f, &pmaddev->devlock);
    if (rc != 0)
        {return rc;}

    rdcount = maddev_get_io_count(pIntState->Status, 
                                  MAD_STATUS_READ_COUNT_MASK,
                                  MAD_STATUS_READ_COUNT_SHIFT,
                                  MAD_UNITIO_SIZE_BYTES);

    //Indicate the read queue is available
    pmaddev->read_f = eIoReset;

    remains =
    copy_to_user(usrbufr, pBufr, (u32)rdcount); //possibly paged memory - copy outside of spinlock
    if (remains > 0)
        {
    	PERR("maddev_read:copy_to_user... dev#=%d returns (%d) bytes remaining\n",
              (int)pmaddev->devnum, (int)remains);
    	return -EFAULT;
        }

    PINFO("maddev_read_bufrd completes... dev#=%d count=%ld\n",
           (int)pmaddev->devnum, rdcount);

	return rdcount;
}

//This function implements a queued read through a kernel io control block
ssize_t maddev_queued_read(struct kiocb *pkiocb, struct iov_iter *piov)
{
	static u32  IntEnable = (MAD_INT_BUFRD_INPUT_BIT | MAD_INT_STATUS_ALERT_BIT);
    static U32 offset = 0;
	//
	PMADDEVOBJ pmaddev = (PMADDEVOBJ)pkiocb->ki_filp->private_data;
	__iomem MADREGS   *pmadregs = pmaddev->pDevBase;
    PMADREGS  pIntState = (PMADREGS)&pmaddev->IntRegState;
    size_t    sz_req = iov_iter_count(piov); //size requested
    void*      pBufr = IoDataRd;
    //
	U32      CntlReg = 0;
	U32      CountBits = 0;
	long     iostat = 0;
    ssize_t  rdcount;
    size_t   copy_len = 0;

    PNOTICE("maddev_queued_read... dev#=%d kiocb=%px piov=%px requested=%d\n",
            (int)pmaddev->devnum, pkiocb, piov, (int)sz_req);

    //If the read queue is not free we return
    if (pmaddev->read_f != eIoReset)
        {return -EAGAIN;}

    mutex_lock(&pmaddev->devmutex);

    //Indicate the read queue is busy
    pmaddev->read_f = eIoPending;

    //Program the i/o
	CountBits = maddev_set_count_bits(sz_req, MAD_CONTROL_IO_COUNT_MASK,
			                          MAD_CONTROL_IO_COUNT_SHIFT,
                                      MAD_UNITIO_SIZE_BYTES);
	CntlReg = (CountBits | MAD_CONTROL_IOSIZE_BYTES_BIT);

    #ifdef _MAD_SIMULATION_MODE_ 
    //Use the kmalloc'd buffer provided by the simulator
    pBufr = pmaddev->pSimParms->pInBufr;
    ASSERT((int)(pBufr != NULL));
    #endif
    //
    memset(pBufr, 0x00, sz_req+2);
    maddevc_program_stream_io(&pmaddev->devlock, pmadregs, CntlReg,IntEnable,
                              virt_to_phys(pBufr), offset, false);

    //Wait for the i-o to complete
    iostat = maddev_wait_get_io_status(pmaddev, &pmaddev->read_q,
                                       &pmaddev->read_f, &pmaddev->devlock);
    if (iostat < 0)
        {//Release the device context semaphore and return the error code
        mutex_unlock(&pmaddev->devmutex);
        pmaddev->read_f = eIoReset;
        return iostat;
        }

    rdcount = maddev_get_io_count(pIntState->Status, 
                                  MAD_STATUS_READ_COUNT_MASK,
                                  MAD_STATUS_READ_COUNT_SHIFT,
                                  MAD_UNITIO_SIZE_BYTES);

    //Indicate the read queue is available
    pmaddev->read_f = eIoReset;
    mutex_unlock(&pmaddev->devmutex);

    copy_len = min_t(size_t, (size_t)rdcount, iov_iter_count(piov));
    copy_len = copy_to_iter(pBufr, copy_len, piov);
    if (copy_len == 0)
        {
    	PERR("maddev_queued_read:copy_to_iter returns... dev#=%d bytes_remaining=%ld rc=-EFAULT\n",
             (int)pmaddev->devnum, rdcount);
    	return -EFAULT;
        }

 	PINFO("maddev_queued_read completes... dev#=%d count=%zu\n",
		  (int)pmaddev->devnum, copy_len);

	return (ssize_t)copy_len;
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
	struct mad_dev_obj *pmaddev = fp->private_data;
	__iomem MADREGS  *pmadregs = pmaddev->pDevBase;
    PMADREGS  pIntState = (PMADREGS)&pmaddev->IntRegState;
	//
    u8*     pBufr = IoData;
	U32      CntlReg = 0;
	U32      CountBits  = 0;
	size_t   wrcount;
	ssize_t  remains = 0;
    int      rc;

    PINFO("maddev_write_bufrd... dev#=%d fp=%p count=%ld\n",
		  (int)pmaddev->devnum, fp, count);

    //If the write queue is not free we return
    if (pmaddev->write_f != eIoReset)
        {return -EAGAIN;}

    //Initialize & Xfer the data
    #ifdef _MAD_SIMULATION_MODE_ 
    //Use the kmalloc'd buffer provided by the simulator
    pBufr = pmaddev->pSimParms->pOutBufr;
    ASSERT((int)(pBufr != NULL));
    #endif

    memset(pBufr, 0x00, count+2);
    remains =
	copy_from_user(pBufr, usrbufr, (u32)count); //possibly paged memory
	if (remains > 0)
	   {
	   PERR("maddev_write:copy_from_user... dev#=%d - (%d) bytes remaining\n",
            (int)pmaddev->devnum, (int)count);
	   return -EFAULT;
	   }

    //PINFO("maddev_write_bufrd... dev#=%d fp=%p count=%ld pBufr[0.1.2.3]=%c.%c.%c.%c\n",
    //       (int)pmaddev->devnum, fp, count, pBufr[0], pBufr[1], pBufr[2], pBufr[3]);

    //Indicate the write queue is busy
    pmaddev->write_f = eIoPending;

    //Program the i/o
    CountBits = 
    maddev_set_count_bits(count, MAD_CONTROL_IO_COUNT_MASK,
                          MAD_CONTROL_IO_COUNT_SHIFT, MAD_UNITIO_SIZE_BYTES);
    CntlReg = CountBits | MAD_CONTROL_IOSIZE_BYTES_BIT;
    //
    maddevc_program_stream_io(&pmaddev->devlock, pmadregs, CntlReg, IntEnable,
                              virt_to_phys(pBufr), offset, true);

    //Wait and process the results
    rc = maddev_wait_get_io_status(pmaddev, &pmaddev->write_q,
                                   &pmaddev->write_f, &pmaddev->devlock);
    if (rc != 0)
        {return rc;}

    wrcount = maddev_get_io_count(pIntState->Status, 
                                  MAD_STATUS_WRITE_COUNT_MASK,
                                  MAD_STATUS_WRITE_COUNT_SHIFT,
                                  MAD_UNITIO_SIZE_BYTES);

    pmaddev->write_f = eIoReset;

	PINFO("maddev_write_bufrd completes... dev#=%d count=%ld\n",
		   (int)pmaddev->devnum, wrcount);

	return wrcount;
}

//This function implements a queued write through a kernel io control block
ssize_t maddev_queued_write(struct kiocb *pkiocb, struct iov_iter *piov)
{
	static ulong IntEnable = (MAD_INT_BUFRD_OUTPUT_BIT | MAD_INT_STATUS_ALERT_BIT);
    static U32 offset = 0;
	//
	PMADDEVOBJ pmaddev  = (PMADDEVOBJ)pkiocb->ki_filp->private_data;
	__iomem MADREGS   *pmadregs = pmaddev->pDevBase;
    PMADREGS  pIntState = (PMADREGS)&pmaddev->IntRegState;
    size_t    sz_req = iov_iter_count(piov);
    void*      pBufr = IoDataWr;
	//
	U32      CntlReg = 0;
	U32      CountBits  = 0;
	ssize_t  copy_len = 0;
    ssize_t  wrcount = 0;
    long     iostat = 0;

	PINFO("maddev_queued_write... dev#=%d kiocb=%px piov=%px requested=%zu\n",
		  (int)pmaddev->devnum, pkiocb, piov, sz_req);

    //If the write queue is not free we return
    if (pmaddev->write_f != eIoReset)
        {return -EAGAIN;}

    //Xfer the data...
    #ifdef _MAD_SIMULATION_MODE_ 
    //Use the kmalloc'd buffer provided by the simulator
    pBufr = pmaddev->pSimParms->pOutBufr;
    ASSERT((int)(pBufr != NULL));
    #endif

    memset(pBufr, 0x00, sz_req+2);
    copy_len = copy_from_iter(pBufr, sz_req, piov);
	if (copy_len == 0)
	    {
	    PERR("maddev_queued_write:copy_from_iter... dev#=%d bytes_remaining=%zu rc=-EFAULT \n",
             (int)pmaddev->devnum, sz_req);
	    return -EFAULT;
	    }

    mutex_lock(&pmaddev->devmutex);

    //Indicate the write queue is busy
    pmaddev->write_f = eIoPending;

    //Program the i/o
	CountBits = 
    maddev_set_count_bits(copy_len, MAD_CONTROL_IO_COUNT_MASK,
                          MAD_CONTROL_IO_COUNT_SHIFT, MAD_UNITIO_SIZE_BYTES);
	CntlReg = (CountBits | MAD_CONTROL_IOSIZE_BYTES_BIT);

    maddevc_program_stream_io(&pmaddev->devlock, pmadregs, CntlReg, IntEnable,
                              virt_to_phys(pBufr), offset, true);

    //Wait for the i-o to complete
    iostat = maddev_wait_get_io_status(pmaddev, &pmaddev->write_q,
                                       &pmaddev->write_f, &pmaddev->devlock);
    if (iostat < 0)
        {//Release the device context semaphore and return the error code
        pmaddev->write_f = eIoReset;
        mutex_unlock(&pmaddev->devmutex);
        return iostat;
        }

    wrcount = maddev_get_io_count(pIntState->Status, 
                                  MAD_STATUS_WRITE_COUNT_MASK,
    		                      MAD_STATUS_WRITE_COUNT_SHIFT,
                                  MAD_UNITIO_SIZE_BYTES);

    //Indicate the write queue is available
    pmaddev->write_f = eIoReset;
    mutex_unlock(&pmaddev->devmutex);

	PINFO("maddev_queued_write completes... dev#=%d count=%ld\n",
		  (int)pmaddev->devnum, wrcount);

    return wrcount;
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
	PMADDEVOBJ  pmaddev =  
                (PMADDEVOBJ)((u8*)mad_dev_objects + (MADDEV_UNIT_SIZE * indx)); 
	__iomem struct _MADREGS  *pmadregs = pmaddev->pDevBase;

    PMADREGS  pIntState = (PMADREGS)&pmaddev->IntRegState;
	u32 IntID   = pIntState->IntID;
	u32 Status  = pIntState->Status & MAD_STATUS_ERROR_MASK;
	u32 Control = pIntState->Control;
	//
	long rc = 0;

	PINFO("maddevc_dpc... indx=%d pmaddev=%px dev#=%u IntID=x%X Control=x%X Status=x%X\n",
		   (int)indx, pmaddev, (int)pmaddev->devnum, IntID, Control, Status);

	rc = maddev_status_to_errno(pmaddev->devnum, 
                                (PMADREGS)&pmaddev->IntRegState);

    if ((IntID & MAD_INT_ALL_VALID_MASK) != 0)
        {maddev_reset_io_registers(pmadregs, &pmaddev->devlock);}

	//Is the completing i/o an ioctl ?
	if ((Control & MAD_CONTROL_CACHE_XFER_BIT)  ||
        (Control & MAD_CONTROL_CHAINED_DMA_BIT) ||
		(IntID & MAD_INT_ALIGN_INPUT_BIT)       ||
		(IntID & MAD_INT_ALIGN_OUTPUT_BIT))    
	    {
	    pmaddev->ioctl_f = rc;
	    wake_up(&pmaddev->ioctl_q);
	    return;
	    }

	//Is the completing i/o a buffered read ?
	if (IntID & MAD_INT_BUFRD_INPUT_BIT)
	    {
        if (pmaddev->read_f == eIoPending)
            {
       	    pmaddev->read_f = rc;
	        wake_up(&pmaddev->read_q);
            return;
            }
	    }

	//Is the completing i/o a buffered write ?
	if (IntID & MAD_INT_BUFRD_OUTPUT_BIT)
	    {
        if (pmaddev->write_f == eIoPending)
            {
	        pmaddev->write_f = rc;
	        wake_up(&pmaddev->write_q);
            return;
            }
	    }

    //If we got here we got trouble
    PERR("maddevc_dpc... dev#=%d IntID not recognized! x%X\n",
         (int)pmaddev->devnum, IntID);
    //ASSERT((int)false);
    //BUG_ON(true);
	return;
}

#if 0
//These two work-items (passive-mode tasklets) are not used
//
void maddev_dpcwork_rd(struct work_struct *dpcwork)
{
	PMADDEVOBJ pmaddev = container_of(dpcwork, struct mad_dev_obj, dpc_work_rd);
	struct async_work  *rd_workq = &pmaddev->rd_workq;
	//
	size_t iocount = 0;
    size_t remains = 0;
   	u8*  __user  usrbufr;
    int rc = 0;

    //Do we have a pending read iocb
    if (rd_workq->piocb == NULL)
        {PINFO(
               "maddev_dpcwork: no active read iocb:iov_iter... dev=%d\n",
                (int)pmaddev->devnum);}
    else
        {
        rc = maddev_status_to_errno(pmaddev->devnum, (PMADREGS)&pmaddev->IntRegState);
        iocount = maddev_get_io_count(pmaddev->IntRegState.Status, MAD_STATUS_READ_COUNT_MASK,
        		             MAD_STATUS_READ_COUNT_SHIFT, MAD_UNITIO_SIZE_BYTES);
        PINFO( 
               "maddev_dpcwork: (read) dev#=%d, piocb=%p, piov=%p, iocount=%d, rc=%d\n",
               (int)pmaddev->devnum, rd_workq->piocb, rd_workq->piov, (int)iocount, rc);
        
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
	PMADDEVOBJ pmaddev = container_of(dpcwork, struct mad_dev_obj, dpc_work_wr);
	struct async_work  *wr_workq = &pmaddev->wr_workq;
	//
	size_t iocount = 0;
    int rc = 0;

    //Do we have a pending write iocb
    if (wr_workq->piocb == NULL)
        {PINFO(
                "maddev_dpcwork: no active write iocb:iov_iter... dev=%d\n",
                (int)pmaddev->devnum);}
    else
        {
        rc = maddev_status_to_errno(pmaddev->devnum, (PMADREGS)&pmaddev->IntRegState);
        iocount = maddev_get_io_count(pmaddev->IntRegState.Status, MAD_STATUS_WRITE_COUNT_MASK,
        		             MAD_STATUS_WRITE_COUNT_SHIFT, MAD_UNITIO_SIZE_BYTES);
        PINFO( 
               "maddev_dpcwork: (write) dev#=%d, piocb=%p, piov=%p, iocount=%d, rc=%d\n",
               (int)pmaddev->devnum, wr_workq->piocb, wr_workq->piov, (int)iocount, rc);
        
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
