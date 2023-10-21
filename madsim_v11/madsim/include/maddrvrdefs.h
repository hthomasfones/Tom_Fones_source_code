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
/*  Exe files   : maddevc.ko, maddevb.ko                                       */ 
/*                                                                             */
/*  Module NAME : maddrvrdefs.h                                                */
/*                                                                             */
/*  DESCRIPTION : Function prototypes & definitions for the MAD device drivers */
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
/* $Id: maddrvrdefs.h, v 1.0 2021/01/01 00:00:00 htf $                         */
/*                                                                             */
/*******************************************************************************/

#ifndef _MAD_DRVR_DEVS_
//
#define _MAD_DRVR_DEFS_

//drivers/base/base.h
struct driver_private
{
	struct kobject kobj;
	struct klist klist_devices;
	struct klist_node knode_bus;
	struct module_kobject *mkobj;
	struct device_driver *driver;
};
#define to_driver(obj) container_of(obj, struct driver_private, kobj)

/*
 * A simple asynchronous I/O implementation.
 */
struct async_work
{
	int    result;
	struct kiocb    *piocb;
    struct iov_iter *piov;
	struct delayed_work work;
	void*  pmaddevobj;
};

// The device context structure
static struct mad_dev_obj
{
	U32                devnum;
	//U32                size;       /* amount of data stored here */
    int                irq;
    struct mutex       devmutex;     
	spinlock_t         devlock;
    struct pci_dev*    pPciDev;
    struct device*     pdevnode;
    U32                bReady;
    U32                bDevRegstrd;
    phys_addr_t        MadDevPA;
	struct page*       pfn_pg;
	PMADREGS           pDevBase;
	PMAD_SIMULATOR_PARMS pSimParms;

	struct tasklet_struct dpctask;
	MADREGS            IntRegState;
	wait_queue_head_t  read_q;
	long               read_f;
	wait_queue_head_t  write_q;
	long               write_f;

    //ioctl variables are also used for dma
	wait_queue_head_t  ioctl_q;
	long               ioctl_f;
    //
   	struct work_struct dpc_work_rd;
   	struct work_struct dpc_work_wr;
	struct async_work  rd_workq;
    struct async_work  wr_workq;

    MAD_DMA_CHAIN_ELEMENT SgDmaElements[MAD_SGDMA_MAX_SECTORS];
    //
    char               pci_config_space[MAD_PCI_CFG_SPACE_SIZE];

    #ifdef _CDEV_
    struct cdev      cdev_str;	  /* Char device structure */
    char       junk[sizeof(struct list_head)];
    #endif

    #ifdef _BLOCKIO_
    struct maddevblk_device *maddevblk_dev;
    #endif
};
//
typedef struct mad_dev_obj MADDEVOBJ;
typedef struct mad_dev_obj* PMADDEVOBJ;

//Function prototypes
static ssize_t
maddev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
///*static*/ ssize_t
//maddev_read_bufrd(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t
maddev_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
//static ssize_t
//maddev_write_bufrd(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);

static loff_t  maddev_llseek(struct file *filp, loff_t off, int whence);
bool  maddev_need_sg(struct page* pPages[], u32 num_pgs);
void maddev_put_user_pages(struct page* page_list[], u32 num_pgs);

static void maddev_cleanup_module(void);
static int  maddev_probe(struct pci_dev *pcidev, const struct pci_device_id *ids);
static void maddev_shutdown(struct pci_dev *pcidev);
static void maddev_remove(struct pci_dev *pcidev);
static int  maddev_setup_device(PMADDEVOBJ pmaddevobj, 
                                struct pci_dev** ppPciDvTmp, U8 HPL);
static void maddev_remove_device(PMADDEVOBJ pmaddevobj);

void maddev_program_stream_io(spinlock_t *splock, PMADREGS pmadregs,
		                     U32 ControlReg, U32 IntEnableReg,
                             phys_addr_t HostAddr, U32 offset, bool bWr);
void maddev_reset_io_registers(PMADREGS pmadregs, spinlock_t *splock);
int maddev_status_to_errno(int devnum, PMADREGS pmadregs);
long maddev_get_io_status(struct mad_dev_obj *pmaddev, wait_queue_head_t* io_q,
                          long* io_f, spinlock_t* plock0); 
void maddev_get_pciconfig_sim(void* pPciCnfg, U32 Len);
int  maddev_claim_pci_resrcs(struct pci_dev* pPciDev, 
                             PMADDEVOBJ pmaddevobj, char* Devname, U32 NumIrqs);
int maddev_release_pci_resrcs(PMADDEVOBJ pmaddevobj);
int  maddev_xchange_sim_parms(struct pci_dev* pPciDev, PMADDEVOBJ pmaddevobj);
void maddev_init_io_parms(PMADDEVOBJ pmaddevobj, U32 indx);
void* maddev_getkva(phys_addr_t PhysAddr, struct page** ppPgStr);
void mad_putkva(struct page* pPgStr);
ssize_t 
maddev_xfer_pages_direct(struct mad_dev_obj *pmaddev, int num_pgs,
                         struct page* page_list[], U32 offset, bool bWrite);
ssize_t maddev_xfer_dma_page(PMADDEVOBJ pmaddev, struct page* pPage,
                             sector_t sector, bool bWrite);
ssize_t maddev_xfer_sgdma_pages(PMADDEVOBJ pmaddev, 
                               long num_pgs, struct page* page_list[],
                               sector_t sector, bool bWrite);
long maddev_get_user_pages(U64 usrbufr, U32 num_pgs,
                           struct page** pPages, struct vm_area_struct** pVMAs,
                           bool bUpdate);
void maddev_put_user_pages(struct page** ppPages, u32 num_pgs);
//
int maddev_kobject_init(struct kobject* pkobj, struct kobject* pPrnt,
                     struct kset* kset, struct kobj_type* ktype, const char *objname);
//int mad_kset_add(void);
int maddev_kobject_register(struct kobject* pkobj, struct kobject* pPrnt, 
                            const char *objname);
void maddev_kobject_unregister(struct kobject* pkobj);
int maddev_kset_create(void);

//inline functions
static inline void 
maddev_program_sgdma_regs(PMAD_DMA_CHAIN_ELEMENT pSgDmaElement, 
                          U64 HostPA, U32 DevLoclAddr, U32 DmaCntl,
                          U32 DXBC, U64 CDPP)
{
    pSgDmaElement->HostAddr    = HostPA;
    pSgDmaElement->DevLoclAddr = DevLoclAddr;
    pSgDmaElement->DmaCntl     = DmaCntl;
    pSgDmaElement->DXBC        = DXBC;
    pSgDmaElement->CDPP        = CDPP;
}

static inline void 
maddev_set_dirty_pages(struct page* page_list[], u32 num_pgs)
{
    u32 j;

    for (j=0; j < num_pgs; j++)
        {
        if (!PageReserved(page_list[j]))
            {SetPageDirty(page_list[j]);}
        }

    return;
}

static inline void maddev_program_io_regs(PMADREGS pmadregs, ULONG ControlReg, 
                                          ULONG IntEnableReg, phys_addr_t HostAddr)
{
    BUG_ON(!(virt_addr_valid(pmadregs)));
    ASSERT((int)(virt_addr_valid(pmadregs)));
//
    writeq(HostAddr, &pmadregs->HostPA);
    iowrite32(0, &pmadregs->IntID);
    iowrite32(ControlReg, &pmadregs->Control);
    iowrite32(IntEnableReg, &pmadregs->IntEnable);
}

//This function computes the transfer count from the input parameters
static inline U32 
maddev_get_io_count(U32 CountBits, U32 CountMask, U32 ShiftVal, U32 Mltpl)
{
U32 IoCount = (CountBits & CountMask); 
    IoCount = (IoCount >> ShiftVal);
    IoCount++; // 0..N-1 --> 1..N
    IoCount = (IoCount * Mltpl);
    return IoCount;
}

//This function sets the transfer count to be programmed into the control register
static inline U32 
maddev_set_count_bits(U32 Count, U32 CountMask, U32 ShiftVal, U32 Mltpl)
{
	U32 CountBits = (Count / Mltpl);
	CountBits--; // 1..N --> 0..N-1
    CountBits = (CountBits << ShiftVal);
    return (CountBits & CountMask);
}

//This function 1st acquires the device spinlock and then disables interrupts
//on the current processor
static inline void maddev_acquire_lock_disable_ints(spinlock_t* splock, U32 F1)
{
    #ifdef _MAD_SIMULATION_MODE_
    spin_lock(splock);
    F1 = 0;
    #else
    spin_lock_irqsave(splock, F1);
    #endif
    ASSERT((int)(spin_is_locked(splock)));
    //local_irq_save(F2);
}

//This function 1st reenables interrupts on the current processor
//and then releases the device spinlock  
static inline void maddev_enable_ints_release_lock(spinlock_t* splock, U32 F1)
{
    //local_irq_restore(F2);
    ASSERT((int)(spin_is_locked(splock)));
    #ifdef _MAD_SIMULATION_MODE_
    spin_unlock(splock); 
    #else
    spin_unlock_irqrestore(splock, F1); 
    #endif
    ASSERT((int)(spin_is_locked(splock) == 0));
}

static inline dma_addr_t
maddev_page_to_dma(struct device* pdev, struct page* pPage, U32 size, bool bWr)
{
    #ifdef _MAD_SIMULATION_MODE_
        return (dma_addr_t)page_to_phys(pPage);
    #else 
        //Real hardware - get a bus-relative dma address
        BUG_ON(dma_get_mask(pdev) == 0); //If dma mask not set
        enum dma_data_direction dir = (bWr) ? DMA_TO_DEVICE : DMA_FROM_DEVICE;
        return dma_map_single(pdev, page_to_virt(pPage), size, dir);
    #endif
}

#if 0
int maddev_kobject_register(struct kobject* pkobj, struct kobject* pPrnt, const char *objname)
{
    int rc; 

    //rc = kobject_set_name(pkobj, objname);
    //if (rc == 0)
    //{rc = kobject_register(pkobj);}
    rc = kobject_init_and_add(pkobj, &mad_ktype, pPrnt, objname);
    if (rc != 0)
        {PDEBUG( "mad_kobject_register failed!... rc=%d\n", rc);}

    return rc;
}
//
void maddev_kobject_unregister(struct kobject* pkobj)
{
    //kobject_unregister(pkobj);
    kobject_put(pkobj);
    kobject_del(pkobj);
}
//
void maddev_kset_unregister(void)
{
    if (bMadKset)
        {maddev_kobject_unregister((struct kobject*)&mad_kset);}
}
//
int maddev_kset_create()
{
    int rc;

    rc = maddev_kobject_register((struct kobject*)&mad_kset, NULL, MadKsetNm);
    bMadKset = (rc == 0);

    return rc;
}
//
endif //0
//
#endif  //_MAD_DRIVER_MAIN_
#endif //_MAD_DRVR_DEFS_
