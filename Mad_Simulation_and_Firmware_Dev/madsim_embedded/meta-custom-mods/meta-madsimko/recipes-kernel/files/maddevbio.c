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

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,15,999)
int maddevb_open(struct block_device *bdev, fmode_t mode)
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,6,0)
//int (*open)(struct gendisk *disk, blk_mode_t mode);
int maddevb_open(struct gendisk *gdisk, fmode_t mode)
#endif
{
    #if LINUX_VERSION_CODE <= KERNEL_VERSION(5,15,999)
	struct gendisk *gdisk = bdev->bd_disk;
    #endif
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(6,6,0)
    struct block_device *bdev = gdisk->part0;
    #endif

    PMADDEVOBJ pmaddev = maddevb_get_parent_from_bdev(bdev);
 
	int rc = 0;

	ASSERT((int)(pmaddev != NULL));
	PINFO("maddevb_open... dev#=%u bdev=%px disk=%px Qctx=%px mode=x%X\n", 
          pmaddev->devnum, bdev, gdisk, bdev_get_queue(bdev), mode);

	if (READ_ONCE(pmaddev->bReady) != true)
	    {
        PWARN("maddevb_open... dev#=%u not open-ready! rc=-EBUSY\n", 
              pmaddev->devnum);
        //ASSERT((int)false);
 		rc = -EBUSY;
	    }

    return rc;
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,15,999)
void maddevb_release(struct gendisk *gdisk, fmode_t mode)
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,6,0)
void maddevb_release(struct gendisk *gdisk)
#endif
{
    struct block_device *bdev = gdisk->part0;
	struct maddev_blk_dev *pmdblkdev = gdisk->private_data;
	PMADDEVOBJ pmaddev = pmdblkdev->pmaddev;

    PINFO("maddevb_release... dev#=%u disk=%px bdev=%px\n",
          pmaddev->devnum, gdisk, bdev);

    //mutex_lock(&pmaddev->devmutex);
    //Nothing to do
    //mutex_unlock(&pmaddev->devmutex);
	schedule();

    return;
}

static int maddevb_system_ioctl(struct block_device* bdev, fmode_t mode,
                                unsigned int cmd, unsigned long arg)
{
    struct maddev_blk_dev *pmdblkdev = bdev->bd_disk->private_data;
    struct mad_dev_obj* pmaddev = (PMADDEVOBJ)pmdblkdev->pmaddev;
    int rc = -ENOSYS; 
    int flags = 0;

    //PINFO("maddevb_system_ioctl... dev#=%d blkdev=%px fmode=x%X cmd=x%X arg=x%X\n",
    //      (int)pmaddev->devnum, bdev, mode, cmd, arg);

    switch (cmd)
        {
        case CDROM_GET_CAPABILITY:
            flags = (int)MADDEVB_GENHD_FLAGS;
            PINFO("maddevb_system_ioctl... cdrom_get_caps; dev#=%d flags=x%X\n",
                  (int)pmaddev->devnum, flags);
            break;

        case CDROMCLOSETRAY:
        case CDROM_SET_OPTIONS: 
        case CDROM_CLEAR_OPTIONS:
        case CDROM_SELECT_SPEED:
        case CDROM_SELECT_DISC:
        case CDROM_MEDIA_CHANGED:
        case CDROM_DRIVE_STATUS:
        case CDROM_DISC_STATUS:
        case CDROM_CHANGER_NSLOTS:
        case CDROM_LOCKDOOR:	
        case CDROM_DEBUG: 
            PINFO("maddevb_io_system_ioctl... cdrom_ioctl dev#=%d rc=%d\n",
                  (int)pmaddev->devnum, rc);
            break;

        default:;
            PINFO("maddevb_io_system_ioctl... dev#=%d cmd=x%X not a system ioctl\n",
                  (int)pmaddev->devnum, cmd);
        }

    if (rc < 0)
        {PINFO("maddevb_io_system_ioctl... dev#=%d rc=%d\n",
                (int)pmaddev->devnum, rc);}

    return rc;
}

/* The ioctl() implementation
 */
extern MADREGS gMadRegsRst; 
int maddevb_ioctl(struct block_device* bdev, fmode_t mode,
                  unsigned int cmd, unsigned long arg)
{
	struct maddev_blk_dev *pmdblkdev = bdev->bd_disk->private_data;
	struct mad_dev_obj *pmaddev =  (PMADDEVOBJ)pmdblkdev->pmaddev;
	__iomem MADREGS    *pmadregs  = (PMADREGS)pmaddev->pDevBase;
	int err = 0;
	long retval = 0;
    int  sysrc = 0;
    U32  flags1 = 0;

	ASSERT((int)(pmaddev != NULL));
    ASSERT((int)(pmdblkdev->gdisk == bdev->bd_disk));
	//ASSERT(bdev_inode(bdev));
    PINFO("maddevb_ioctl... dev#=%d blkdev=%px fmode=x%X cmd=x%X arg=x%X\n",
		  (int)pmaddev->devnum, bdev, mode, cmd, (int)arg);
  
    mutex_lock(&pmaddev->devmutex);

    sysrc = maddevb_system_ioctl(bdev, mode, cmd, arg);
    if (sysrc != -ENOSYS)
        {
        retval = sysrc;
		goto pmdblkio_ioctl_end;
        }

    /* Extract the type and number bitfields, and don't decode
	 * wrong cmds: return (inappropriate ioctl) before access_ok() */
	if ((_IOC_TYPE(cmd) != MADDEV_IOCTL_MAGIC)  ||
        (_IOC_NR(cmd) > MADDEV_IOCTL_MAX_NBR))
	    {
        PWARN("maddevb_ioctl... dev#=%d cmd=x%X rc=-EACCES\n", 
              (int)pmaddev->devnum, (unsigned)cmd);
		retval = -EACCES;
		goto pmdblkio_ioctl_end;
	    }

    err = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
	if (err)
	    {
        PERR( "maddevb_ioctl... dev#=%d rc=-EINVAL\n", (int)pmaddev->devnum);
		retval = -EINVAL;
		goto pmdblkio_ioctl_end;
	    }
 
    //If the ioctl queue is not free we return
    if (pmaddev->ioctl_f != eIoReset)
        {
        retval = -EAGAIN;
		goto pmdblkio_ioctl_end;
        }

    switch(cmd)
	    {
	    case MADDEV_IOCTL_INIT: //Initialize the device in a standard way
	    	PINFO("maddevb_ioctl: MADDEV_IOCTL_INIT\n");
            maddev_acquire_lock_disable_ints(&pmaddev->devlock, &flags1);
            memcpy_toio(&gMadRegsRst, pmadregs, sizeof(MADREGS)); //Hard-coded init values
            maddev_enable_ints_release_lock(&pmaddev->devlock, &flags1);
            break;

	    case MADDEV_IOCTL_RESET: //Reset all index registers
	    	PINFO("maddevb_ioctl: MADDEV_IOCTL_RESET\n");
            maddev_acquire_lock_disable_ints(&pmaddev->devlock, &flags1);
	    	iowrite32(0, &pmadregs->ByteIndxRd);
	    	iowrite32(0, &pmadregs->ByteIndxWr);
	    	iowrite32(0, &pmadregs->CacheIndxRd);
	    	iowrite32(0, &pmadregs->CacheIndxWr);
	    	iowrite32(MAD_STATUS_CACHE_INIT_MASK, &pmadregs->Status);
            maddev_enable_ints_release_lock(&pmaddev->devlock, &flags1);
            break;

	    default:
		    retval = -EINVAL;
	    } /* */

pmdblkio_ioctl_end:;		
    mutex_unlock(&pmaddev->devmutex);   
    
    if (retval != 0)
       {PERR("maddevb_io_ioctl... devnum=%d rc=%ld\n", 
		     (int)pmaddev->devnum, retval);}

	return (int)retval;
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,15,999)
int maddevb_rw_page(struct block_device *bdev, sector_t sector,
		            struct page *pPage, unsigned int op)
//#endif     
//#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,6,0)
//int maddevb_rw_page(struct block_device *bdev, sector_t sector,
//		            struct page *pPage, unsigned int op)
//#endif                        
{
    PMADDEVOBJ pmaddev = maddevb_get_parent_from_bdev(bdev);
    bool bH2D = op_is_write(op);
    ssize_t iocount = 0;
    blk_status_t rc = 0;

	ASSERT((int)(pmaddev != NULL));
	//ASSERT(bdev_inode(bdev));

    if (atomic_cmpxchg(&pmaddev->batomic, 0, 1))
	    {
		PWARN("maddevb_rw_page... dev#=%d concurrent requests: rc=-EAGAIN\n",
			 (int)pmaddev->devnum);
	    return BLK_STS_RESOURCE;  //errno_to_blk_status(-EAGAIN);
		}

    PINFO("maddevb_rw_page... dev#=%d bdev=%px pPage=x%px sector=%ld op=%d\n",
          (int)pmaddev->devnum, bdev, pPage, (long int)sector, op);

    iocount = maddev_xfer_dma_page(pmaddev, pPage, sector, bH2D);
    if (iocount < 0)
        {rc = errno_to_blk_status(iocount);}

    //Specific to a read - each page needs to be marked 'dirty' (updated)
    // *if* it is not in the reserved region - (never swapped out to disk)
    //PINFO("maddevb_rw_page... set the dirty bit if read page(s) reserved? wr=%d\n",
    //       bH2D);
    if (bH2D==false)
        {
        if (!PageReserved(pPage))
            {SetPageDirty(pPage);}
        }
        
	//if (rc != 0)
    //    {BUG_ON(1);}
	/*
	 * The ->rw_page interface is subtle and tricky.  The core
	 * retries on any error, so we can only invoke page_endio() in
	 * the successful completion case.  Otherwise, we'll see crashes
	 * caused by double completion.
	 */
    if (rc == 0)
        {maddevb_page_endio(pPage, op_is_write(op), 0);}

    atomic_set(&pmaddev->batomic, 0);

    PINFO("maddevb_rw_page... dev#=%d rc=%d iocount=%ld\n",
          (int)pmaddev->devnum, rc, (long int)iocount);

    return rc;
}
#endif

//This function implements a direct-io read into a user mode buffer
ssize_t
maddevb_direct_io(struct file *filp, const char __user *usrbufr, 
                  size_t count, loff_t *f_pos, bool bH2D)
{
    PMADDEVOBJ pmaddev = filp->private_data;
    u32        pg_cnt = (MAD_MODULO_ADJUST(count, PAGE_SIZE) / PAGE_SIZE);
    u32        offset = MAD_MODULO_ADJUST(*f_pos, MAD_SECTOR_SIZE);
    sector_t   sector = (offset / MAD_SECTOR_SIZE);
    U64        start_page = MAD_MODULO_ADJUST((U64)usrbufr, PAGE_SIZE);
    U8         IoQbusy = (bH2D) ? (pmaddev->write_f != eIoReset) :
                                 (pmaddev->read_f != eIoReset);
    bool       bUpdate = (bH2D==true) ? false : true;
    U8     bNeedSg = false;   
    ssize_t lrc = -EFAULT; //bad address
    //
	struct page* pPages[MAD_DIRECT_XFER_MAX_PAGES]; 
    struct vm_area_struct* pVMAs[MAD_DIRECT_XFER_MAX_PAGES];
	u32    num_pgs;
    ssize_t  iocount;

    BUG_ON(!(virt_addr_valid(pmaddev)));

    if (atomic_cmpxchg(&pmaddev->batomic, 0, 1))
	    {
		PWARN("maddevb_rw_page... dev#=%d concurrent requests: rc=-EAGAIN\n",
			  (int)pmaddev->devnum);
	    return -EAGAIN;
		}

    PINFO("maddevb_direct_io... dev#=%d fp=%px offset=%ld sector=%ld count=%ld\n",
		  (int)pmaddev->devnum, filp, (long int)offset, 
          (long int)sector, (long int)count);

    if (((U32)count + offset) >= MAD_DEVICE_DATA_SIZE)
        {return lrc;}

    //If the io queue is not free we return
    if (IoQbusy)
        {
        lrc = -EAGAIN;
        PERR("maddevb_direct_io... dev#=%d Io-Q busy rc=%ld\n",
             (int)pmaddev->devnum, (long int)lrc);
        return lrc;
        }

    num_pgs = maddev_get_user_pages(start_page, pg_cnt, pPages, pVMAs, bUpdate);
    if (num_pgs != pg_cnt)
        {
        PERR("maddev_direct_io:get_user_pages... dev#=%d num_pgs=%ld rc=%ld\n",
		     (int)pmaddev->devnum, (long int)num_pgs, (long int)lrc);
        return lrc;
        }

    bNeedSg = maddev_need_sg(pPages, num_pgs);

    //Acquire the device context semaphore, call the helper function,
    //release the device context semaphore
    if (bNeedSg) 
        {iocount = 
         maddev_xfer_sgdma_pages(pmaddev, num_pgs, pPages, sector, bH2D);}
    else
        {iocount = maddev_xfer_dma_page(pmaddev, pPages[0], sector, bH2D);}

    if (bH2D==false)
        {maddev_set_dirty_pages(pPages, num_pgs);}

    maddev_put_user_pages(pPages, num_pgs);

    atomic_set(&pmaddev->batomic, 0);

    PINFO("maddev_direct_io... dev#=%d offset=%ld lrc(iocount)=%ld \n",
          (int)pmaddev->devnum, (long int)offset, (long int)iocount);

    return iocount;
}

//Build the hardware scatter-gather list from the os scatterlist
int maddevb_init_sglist_io(PMADDEVOBJ pmaddev, struct scatterlist *sglist, 
	                       U32 sgl_size, sector_t tsector, sector_t nr_sectors, 
                           bool bH2D, bool bInitIO)
{
    MADREGS __iomem *pmadregs = pmaddev->pDevBase;
    phys_addr_t BCDPP = ioread64(&pmadregs->BCDPP); //(U64)virt_to_phys(&pmaddev->SgDmaUnits[0]);
    phys_addr_t CDPP  = BCDPP;
    MAD_DMA_CHAIN_ELEMENT __iomem *pHwSgle = pmadregs->SgDmaUnits; //phys_to_virt(CDPP);
    struct scatterlist *psgle = sglist;
    U32    IntEnable = (bH2D) ? 
                   (MAD_INT_STATUS_ALERT_BIT | MAD_INT_DMA_OUTPUT_BIT) : 
                   (MAD_INT_STATUS_ALERT_BIT | MAD_INT_DMA_INPUT_BIT);
    //
    U32    DevDataOfst = (tsector * MAD_SECTOR_SIZE);
    U32    DmaCntl   = (bH2D) ? (MAD_DMA_CNTL_INIT | MAD_DMA_CNTL_H2D) :
                                 MAD_DMA_CNTL_INIT;
    U32    CntlReg   = MAD_CONTROL_CHAINED_DMA_BIT;
    U32    flags1    = 0; 
    U32    j = 0;
  
    PINFO("maddevb_init_sglist_io... dev#=%d BCDPP=%llx sgl_size=%u #sectors=%u wr=%d\n",
          (int)pmaddev->devnum, BCDPP, (uint32_t)sgl_size, 
          (uint32_t)nr_sectors, bH2D);

    pmaddev->ioctl_f = eIoPending;
    maddev_acquire_lock_disable_ints(&pmaddev->devlock, &flags1);
    maddev_program_io_regs(pmadregs, CntlReg, IntEnable, (phys_addr_t)0);

    for (j=1; j <= sgl_size; j++)
        {
        //The set of hardware SG elements is defined as a linked-list even though
        //it is created as an array
        //Set the next Chained-DMA-Pkt_Pntr address
        CDPP = (j == sgl_size) ? MAD_DMA_CDPP_END : Next_CDPP(CDPP);
                                 //virt_to_phys(&pmadregs->SgDmaUnits[j]);

        maddevb_init_cde_from_sgle(pHwSgle, psgle, DevDataOfst, DmaCntl, CDPP);
         
        //We should we use the Chained-Dma-Pkt-Pntr
        // because we treat the chained list as a linked-list to be proper 
        if (CDPP != MAD_DMA_CDPP_END)
            {
            DevDataOfst += psgle->length;
            pHwSgle = Next_HWSGLE(pHwSgle, CDPP);
            psgle++;
            }
        }

    //Finally - initiate the i/o
    if (bInitIO)
        {
        CntlReg |= MAD_CONTROL_DMA_GO_BIT;
        iowrite32(CntlReg, &pmadregs->Control);
        }

    maddev_enable_ints_release_lock(&pmaddev->devlock, &flags1);

    return 0;
 }
 
 //Wait for the hardware io completion and then cleanup
ssize_t 
maddevb_complete_sglist_io(PMADDEVOBJ pmaddev, struct maddevb_cmd *cmd)
{
    __iomem MADREGS  *pmadregs  = (PMADREGS)pmaddev->pDevBase;
    struct request *req = cmd->req;
    sector_t nr_sectors = blk_rq_sectors(req);
    //enum req_opf op = GET_OPF_FROM_REQ(req);
    blk_opf_t op = GET_OPF_FROM_REQ(req);
    U32 dma_size = cmd->dma_size;
    ssize_t iocount = 0;
    U32   iostat;
   
    PINFO("maddev_complete_sglist_io... dev#=%d cmd=%px sgl_size=%u #sectors=%u op=%d\n",
          (int)pmaddev->devnum, cmd, (uint32_t)dma_size, (uint32_t)nr_sectors, op);
    
    //Wait for the i-o completion and process the results
    iostat = maddev_wait_get_io_status(pmaddev, &pmaddev->ioctl_q,
                                       &pmaddev->ioctl_f, &pmaddev->devlock);
    iocount = (iostat < 0) ? iostat : ioread32(&pmadregs->DTBC);

    if (iostat < 0)
        PWARN("maddev_complete_sglist_io... \n    \
              dev#=%d cmd=%px sgl_size=%u #sectors=%u op=%d iostat=%u\n",
              (int)pmaddev->devnum, cmd, (uint32_t)dma_size, 
              (uint32_t)nr_sectors, op, (uint32_t)iostat);

    pmaddev->ioctl_f = eIoReset;

    return iocount;
}

//This is the common worker function for all (msi & legacy interrupt functions
irqreturn_t maddevb_isr_worker_fn(int irq, void* pdevobj, int msinum)
{
    PMADDEVOBJ pmaddev = (PMADDEVOBJ)pdevobj;
	__iomem MADREGS  *pmadregs = pmaddev->pDevBase;
    U32 devnum = pmadregs->Devnum;
	//
	U32 flags1 = 0;

    maddev_acquire_lock_disable_ints(&pmaddev->devlock, &flags1);
    u32 IntID = ioread32(&pmadregs->IntID);
    u32 Control = ioread32(&pmadregs->Control);

    PINFO("maddevb_isr... dev#=%d pmaddev=%px irq=%d msinum=%d IntID=x%x Control=x%x\n",
          (int)devnum, pmaddev, irq, msinum, IntID, Control);

    //Disable interrupts on this device
    iowrite32(MAD_ALL_INTS_DISABLED, &pmadregs->IntEnable);
    iowrite32(0, &pmadregs->MesgID);

    //Copy the device register state for the DPC
    memcpy_fromio(&pmaddev->IntRegState, pmadregs, MADREGS_BASE_SIZE);

    IntID = ioread32(&pmadregs->IntID);
    if (IntID == (u32)MAD_ALL_INTS_CLEARED)
        {   //This is not our IRQ
        maddev_enable_ints_release_lock(&pmaddev->devlock, &flags1);

        PERR("maddevb_isr... invalid int recv'd: dev#=%d IntID=x%x\n",
             (int)devnum, (uint32_t)IntID);
    	return IRQ_NONE;
        }

    if (IntID == (u32)MAD_INT_INVALID_BYTEMODE_MASK)//Any / all undefined int conditions
        {
        maddev_enable_ints_release_lock(&pmaddev->devlock, &flags1);

        PERR("maddevb_isr... undefined int-id from device: dev#=%d IntID=x%x\n",
             (int)devnum, (uint32_t)IntID);
    	return IRQ_HANDLED;
        }

    pmaddev->dpctask.data = devnum;
	/* Invoke the DPC handler */
    #ifdef _MAD_SIMULATION_MODE_ 
    //Release the spinlock *NOT* at device-irql BEFORE enqueueing the DPC
    maddev_enable_ints_release_lock(&pmaddev->devlock, &flags1);
    #endif

    /* Invoke the DPC handler */
    tasklet_hi_schedule(&pmaddev->dpctask);

    #ifndef _MAD_SIMULATION_MODE_ //(real hardware)
    //With real hardware release the spinlock at device-irql  AFTER enqueueing the DPC
    maddev_enable_ints_release_lock(&pmaddev->devlock, flags1);
    #endif

    PINFO("maddevb_isr... normal exit: dev#=%d IntID=x%x irq_ok=%d\n",
          (int)devnum, IntID, IRQ_HANDLED);

	return IRQ_HANDLED;
}

//This is the Interrupt Service Routine which is established by a successful
//call of request_irq
irqreturn_t maddevb_legacy_isr(int irq, void* pdevobj)
{
    return maddevb_isr_worker_fn(irq, pdevobj, 0);
}

//These are the msi Interrupt Service Routines established by
// calls to indx=%drequest_irq
irqreturn_t maddevb_msi_one_isr(int irq, void* pdevobj)
{
    return maddevb_isr_worker_fn(irq, pdevobj, 1);
}

irqreturn_t maddevb_msi_two_isr(int irq, void* pdevobj)
{
    return maddevb_isr_worker_fn(irq, pdevobj, 2);
}

irqreturn_t maddevb_msi_three_isr(int irq, void* pdevobj)
{
    return maddevb_isr_worker_fn(irq, pdevobj, 3);
}

irqreturn_t maddevb_msi_four_isr(int irq, void* pdevobj)
{
    return maddevb_isr_worker_fn(irq, pdevobj, 4);
}

irqreturn_t maddevb_msi_five_isr(int irq, void* pdevobj)
{
    return maddevb_isr_worker_fn(irq, pdevobj, 5);
}

irqreturn_t maddevb_msi_six_isr(int irq, void* pdevobj)
{
    return maddevb_isr_worker_fn(irq, pdevobj, 6);
}

irqreturn_t maddevb_msi_seven_isr(int irq, void* pdevobj)
{
    return maddevb_isr_worker_fn(irq, pdevobj, 7);
}

irqreturn_t maddevb_msi_eight_isr(int irq, void* pdevobj)
{
    return maddevb_isr_worker_fn(irq, pdevobj, 8);
}

//This is the DPC/software interrupt handler invoked by scheduling a tasklet
//for a task struct configured with this function as the DPC task
//
void maddevb_dpctask(ulong indx)
{
	PMADDEVOBJ  pmaddev = 
                (PMADDEVOBJ)((u8*)mad_dev_objects + (MADDEV_UNIT_SIZE * indx));  
	__iomem MADREGS  *pmadregs   = pmaddev->pDevBase;
    PMADREGS    pIntState = (PMADREGS)&pmaddev->IntRegState;
	u32 IntID     = pIntState->IntID;
	u32 Status    = pIntState->Status & MAD_STATUS_ERROR_MASK;
	u32 Control   = pIntState->Control;
	//
	long rc = 0;

	PINFO("maddevb_dpc... indx=%d pmaddev=%px dev#=%d IntID=x%lX Control=x%lX Status=x%lX \n",
		  (int)indx, (void *)pmaddev, (int)pmaddev->devnum, 
          (unsigned long)IntID, (unsigned long)Control, (unsigned long)Status);

	rc = maddev_status_to_errno(pmaddev->devnum, 
                                (PMADREGS)&pmaddev->IntRegState);

    if ((IntID & MAD_INT_ALL_VALID_MASK) != 0)
        maddev_reset_io_registers(pmadregs, &pmaddev->devlock);

    if ((IntID & MAD_INT_DMA_INPUT_BIT) || (IntID & MAD_INT_DMA_OUTPUT_BIT))
        {
        if (pmaddev->ioctl_f == eIoPending)
            {
            pmaddev->ioctl_f = rc;
            wake_up(&pmaddev->ioctl_q);
            }
        else
            {
            PWARN("maddevb_dpc... dev#=%d Int unexpected! ioctl_f=%u\n",
                  (int)pmaddev->devnum, (uint32_t)pmaddev->ioctl_f);
            ASSERT((int)false);
            }
        return;
        }

    //If we got here we got trouble
    PERR("maddevb_dpc... dev#=%d IntID not recognized! x%X\n",
              (int)pmaddev->devnum, IntID);
    BUG_ON(true);
    return;
}

#if 0 //void maddev_dpcwork_rd(struct work_struct *dpcwork)
//These two work-items (passive-mode tasklets) are not used
void maddev_dpcwork_rd(struct work_struct *dpcwork)
{
	PMADDEVOBJ pmaddev = 
               container_of(dpcwork, struct mad_dev_obj, dpc_work_rd);
	struct async_work  *rd_workq = &pmaddev->rd_workq;
	//
	size_t iocount = 0;
    size_t remains = 0;
   	u8*  __user  usrbufr;
    int rc = 0;

    //Do we have a pending read iocb
    if (rd_workq->piocb == NULL)
        {PINFO("maddev_dpcwork: no active read iocb:iov_iter... dev=%d\n",
                (int)pmaddev->devnum);}
    else
        {
        rc = maddev_status_to_errno(pmaddev->devnum, (PMADREGS)&pmaddev->IntRegState);
        iocount = maddev_get_io_count(pmaddev->IntRegState.Status, MAD_STATUS_READ_COUNT_MASK,
        		                      MAD_STATUS_READ_COUNT_SHIFT, MAD_UNITIO_SIZE_BYTES);
        PINFO("maddev_dpcwork: (read) devnum=%d, piocb=%px, piov=%px, iocount=%d, rc=%d\n",
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
	PMADDEVOBJ pmaddev = 
               container_of(dpcwork, struct mad_dev_obj, dpc_work_wr);
	struct async_work  *wr_workq = &pmaddev->wr_workq;
	//
	size_t iocount = 0;
    int rc = 0;

    //Do we have a pending write iocb
    if (wr_workq->piocb == NULL)
        {PINFO("maddev_dpcwork: no active write iocb:iov_iter... dev=%d\n",
                (int)pmaddev->devnum);}
    else
        {
        rc = maddev_status_to_errno(pmaddev->devnum, (PMADREGS)&pmaddev->IntRegState);
        iocount = maddev_get_io_count(pmaddev->IntRegState.Status, MAD_STATUS_WRITE_COUNT_MASK,
        		                      MAD_STATUS_WRITE_COUNT_SHIFT, MAD_UNITIO_SIZE_BYTES);
        PINFO("maddev_dpcwork: (write) devnum=%d, piocb=%px, piov=%px, iocount=%d, rc=%d\n",
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
//#if 0
void maddev_complete_io(struct work_struct *work)
{
	struct async_work *qio_work = container_of(work, struct async_work, work.work);
	//aio_complete(qio_work->iocb, qio_work->result, 0);
	qio_work->iocb->ki_complete(qio_work->iocb, 0, 0);
} 
#endif

#if 0 //int maddevb_collect_biovecs(struct mad_dev_obj *pmaddev, struct bio* pbio,
                                    u32 nr_sectors, struct bio_vec* pbiovecs)
//Do we have one fragmented page of sectors in various bio:bvecs ?
//One sector per bio_vec: one bio_vec per bio
int 
maddevb_collect_biovecs(struct mad_dev_obj *pmaddev, struct bio* pbio,
                        u32 nr_sectors, struct bio_vec* pbiovecs)
{
    struct bio* pbiot = pbio;
    struct bio_vec* pbiovect  = pbiot->bi_io_vec; 
    struct bio_vec* pbiovecst = pbiovecs; //A passed in contiguous array - or NULL
    //
    u32 j;
    u32 totl_sectors= 0;
    int totl_bvecs = 0;

    while (pbiot != NULL)
        {
        pbiovect = pbiot->bi_io_vec;

        for (j=0; j < pbiot->bi_vcnt; j++)
            {
            ASSERT((int)((pbiovect->bv_len % MAD_SECTOR_SIZE) == 0));
            ASSERT((int)((pbiovect->bv_offset % MAD_SECTOR_SIZE) == 0));

            //If an array is passed -- accumulate an array of biovecs for sgdma processing
            if (pbiovecs != NULL)
                {memcpy(pbiovecst, pbiovect, sizeof(struct bio_vec));}

            totl_bvecs++;
            totl_sectors += (pbiovect->bv_len / MAD_SECTOR_SIZE);

            pbiovect++; 
            pbiovecst++; 
            }

        pbiot = pbiot->bi_next;
        }

    ASSERT((int)(totl_sectors == nr_sectors));

    PINFO("maddevb_collect_biovecs... dev#=%d bio=%px total_bvecs=%d totl_sctrs=%ld\n",
          (int)pmaddev->devnum, (void *)pbio, totl_bvecs, (long int)totl_sectors);

    return totl_bvecs;
}
#endif

#if 0 //ssize_t maddevb_xfer_sgdma_bvecs(PMADDEVOBJ pmaddev, 
                                 struct bio_vec* biovecs, long nr_bvecs, 
                                 sector_t sector, u32 nr_sectors, bool bH2D)
//Build the hardware scatter-gather list from the set of biovecs
ssize_t maddevb_xfer_sgdma_bvecs(PMADDEVOBJ pmaddev, 
                                 struct bio_vec* biovecs, long nr_bvecs, 
                                 sector_t sector, u32 nr_sectors, bool bH2D)
{
    struct bio_vec* pbiovect = biovecs;
    U32 DevDataOfst = (sector * MAD_SECTOR_SIZE);
    PMAD_DMA_CHAIN_ELEMENT pHwSgle = &pmaddev->SgDmaUnits[0];
    U32    IntEnable = 
           (bH2D) ? (MAD_INT_STATUS_ALERT_BIT | MAD_INT_DMA_OUTPUT_BIT) :
                   (MAD_INT_STATUS_ALERT_BIT | MAD_INT_DMA_INPUT_BIT);
    //
    U32    DmaCntl   = MAD_DMA_CNTL_INIT;
    U32    CntlReg   = MAD_CONTROL_CHAINED_DMA_BIT;
    dma_addr_t HostPA = 0;
    //ssize_t iocount = 0;
    size_t  bvlen_totl = 0;
    u32  totl_sectors = 0;
    U32 flags1 = 0; 
    U64    CDPP;
    long int lrc = -EADDRNOTAVAIL;
    U32 j;

    if (bH2D)
        {DmaCntl |= MAD_DMA_CNTL_H2D;} //Write == Host2Disk

    PINFO("maddevb_xfer_sgdma_bvecs... dev#=%d nr_bvecs=%ld #sectors=%ld wr=%d\n",
          (int)pmaddev->devnum, nr_bvecs, (long int)nr_sectors, bH2D);

    pmaddev->ioctl_f = eIoPending;
    maddev_acquire_lock_disable_ints(&pmaddev->devlock, flags1);
    //
    maddev_program_io_regs(pmaddev->pDevBase, CntlReg, IntEnable, HostPA);

    //Set the beginning of the Chained-DMA-Pkt-Pntr list
    iowrite64(virt_to_phys(pHwSgle), &pmaddev->pDevBase->BCDPP);

    for (j=1; j <= nr_bvecs; j++)
        {
        HostPA = maddev_dma_map_page(pmaddev->pdevnode, 
                                     pbiovect->bv_page, pbiovect->bv_len, //PAGE_SIZE, 0
                                     pbiovect->bv_offset, bH2D);
        if (dma_mapping_error(pmaddev->pdevnode, HostPA))
            {
            maddev_enable_ints_release_lock(&pmaddev->devlock, flags1);
            PERR("maddev_xfer_sgdma_bvecs... dev#=%d rc=%ld\n",
                 (int)pmaddev->devnum, lrc);
            return lrc;
            }

        //HostPA += pbiovect->bv_offset;

        //The set of hardware SG elements is defined as a linked-list even though
        //it is created as an array
        //Set the next Chained-DMA-Pkt_Pntr address
        CDPP = (j == nr_bvecs) ? MAD_DMA_CDPP_END : 
                                 virt_to_phys(&pmaddev->SgDmaUnits[j]);

        maddev_program_sgdma_regs(pHwSgle, HostPA, DevDataOfst,
                                  DmaCntl, pbiovect->bv_len, CDPP);

        bvlen_totl += pbiovect->bv_len;
        totl_sectors += (pbiovect->bv_len / MAD_SECTOR_SIZE);
        DevDataOfst += pbiovect->bv_len;
     
        //We could use the next array element but instead we use the 
        //Chained-Dma-Pkt-Pntr because we treat the chained list as a
        //linked-list to be proper 
        //pHwSgle = &pmaddev->SgDmaUnits[j];
        if (j < nr_bvecs)
            {pHwSgle = phys_to_virt(CDPP);}

        pbiovect++;
        }

    //Finally - initiate the i/o
    CntlReg |= MAD_CONTROL_DMA_GO_BIT;
    iowrite32(CntlReg, &pmaddev->pDevBase->Control);
    maddev_enable_ints_release_lock(&pmaddev->devlock, flags1);

    return 0;
 }
#endif

#if 0 // ssize_t maddevb_xfer_sgdma_bvecs_completion(PMADDEVOBJ pmaddev, 
                                            struct maddevb_cmd *cmd)
//Build the hardware scatter-gather list from the set of biovecs
ssize_t maddevb_xfer_sgdma_bvecs_completion(PMADDEVOBJ pmaddev, 
                                            struct maddevb_cmd *cmd)
{
    struct request *req = cmd->req;
    sector_t nr_sectors = blk_rq_sectors(req);
	
    enum req_opf op = req_op(req);
    bool bH2D = (op == REQ_OP_WRITE);
    struct bio *pbio = cmd->req->bio;
    int nr_bvecs = maddevb_collect_biovecs(pmaddev, pbio, nr_sectors, NULL);
    ssize_t iocount = 0;
    long   iostat;
     U32 j;

    //Wait for the i-o completion and process the results
    iostat = maddev_wait_get_io_status(pmaddev, &pmaddev->ioctl_q,
                                  &pmaddev->ioctl_f,
                                  &pmaddev->devlock);
    iocount = (iostat < 0) ? iostat : pmaddev->pDevBase->DTBC;
    if (iostat == 0)
        {
        //ASSERT((int)(iocount == bvlen_totl));
        //ASSERT((int)((bvlen_totl % MAD_SECTOR_SIZE) == 0));
        //ASSERT((int)(totl_sectors == nr_sectors));
        }
  
    for (j=0; j < nr_bvecs; j++)
        {
        maddev_dma_unmap_page(pmaddev->pdevnode, 
                              pmaddev->SgDmaUnits[j].HostAddr, 
                              PAGE_SIZE, bH2D);
        }

    PINFO("maddev_xfer_sgdma_bvecs_completion... \ndev#=%d cmd=%px #bvecs=%d #sectors=%d iostat=%ld iocount=%ld\n",
          (int)pmaddev->devnum, cmd, (int)nr_bvecs, (int)nr_sectors,
          iostat, iocount);
    pmaddev->ioctl_f = eIoReset;

    return iocount;
}
#endif

#if 0 //This function implements a queued read through a kernel io control block
#endif