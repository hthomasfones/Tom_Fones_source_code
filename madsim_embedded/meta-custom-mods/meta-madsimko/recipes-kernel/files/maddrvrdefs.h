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

#include <asm/page.h>	
#include <linux/mm.h>

#ifndef _MAD_DRVR_DEFS_
//
#define _MAD_DRVR_DEFS_

#define DMADIR(bH2D) (((bool)bH2D) ? DMA_TO_DEVICE : DMA_FROM_DEVICE)

//extern struct bus_type madbus_type;

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
	void*  pmaddev;
};

// The device context structure
struct mad_dev_obj
{
	uint32_t           devnum;
	//U32                size;       /* amount of data stored here */
    int                base_irq;
    struct mutex       devmutex;     
	spinlock_t         devlock;
    struct pci_dev*    pPcidev;
    struct device*     pdevnode;
    struct device*     pclassdev;
    U32                bReady;
    U32                bDevRegstrd;
    phys_addr_t        MadDevPA;
	struct page*       pfn_pg;
	__iomem MADREGS   *pDevBase;
 	PMAD_SIMULATOR_PARMS pSimParms;
	struct tasklet_struct dpctask;
 	wait_queue_head_t  read_q;
	U32                read_f;
	wait_queue_head_t  write_q;
	U32                write_f;

    //ioctl variables are also used for dma
	wait_queue_head_t  ioctl_q;
	U32                ioctl_f;
    //
   	struct work_struct dpc_work_rd;
   	struct work_struct dpc_work_wr;
	struct async_work  rd_workq;
    struct async_work  wr_workq;
    struct vm_area_struct* vma;

    //MAD_DMA_CHAIN_ELEMENT SgDmaUnits[MAD_SGDMA_MAX_SECTORS];
    struct scatterlist *sglist; //[MAD_SGDMA_MAX_PAGES];
    //
    char               pci_config_space[MAD_PCI_CFG_SPACE_SIZE];

    #ifdef _BLOCKIO_
        struct maddev_blk_dev *pmdblkdev;
        struct kmem_cache *pcmd_cache;
        atomic_t     batomic;
     #endif

    struct cdev   cdev_str;	  /* Char device structure */
    char          junk[sizeof(struct cdev)];
    bool    cdev_added;
    ULONG   IntRegState[NUM_BASE_MADREGS] __aligned(8);
};
typedef struct mad_dev_obj MADDEVOBJ;
typedef struct mad_dev_obj *PMADDEVOBJ;

#define MADDEV_UNIT_SIZE  \
        (((sizeof(struct mad_dev_obj) + PAGE_SIZE) / PAGE_SIZE) * PAGE_SIZE)

//Function prototypes
ssize_t
maddev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
///*static*/ ssize_t
//maddev_read_bufrd(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t
maddev_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
//static ssize_t
//maddev_write_bufrd(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
int maddev_mmap(struct file *fp, struct vm_area_struct* vma);
//#ifdef _BLOCKIO_
 //   static int maddevr_setup_cdev(void* pvoid, int indx);
//#endif

int maddev_setup_devices(int num_devs, U8 bHPL, u8 bMSI); 
void maddev_remove_devices(int num_devices);
void maddev_cleanup_module(void);
void maddev_release_device(struct device *pdev);
int  maddev_probe(struct pci_dev *pcidev, const struct pci_device_id *ids);
void maddev_shutdown(struct pci_dev *pcidev);
void maddev_remove_pcidev(struct pci_dev *pcidev);
int  maddev_setup_device(PMADDEVOBJ pmaddev, 
                         struct pci_dev** ppPciDvTmp, U8 HPL, u8 bMSI);
void maddev_remove_device(PMADDEVOBJ pmaddev);
loff_t  maddev_llseek(struct file *filp, loff_t off, int whence);
bool  maddev_need_sg(struct page* pPages[], u32 num_pgs);
void maddevc_program_stream_io(spinlock_t *splock, __iomem MADREGS *pmadregs,
		                       U32 ControlReg, U32 IntEnableReg,
                             phys_addr_t HostAddr, U32 offset, bool bH2D);
void maddev_reset_io_registers(__iomem MADREGS *pmadregs, spinlock_t *splock);
int maddev_status_to_errno(int devnum, __iomem MADREGS *pmadregs);
U32 maddev_wait_get_io_status(struct mad_dev_obj *pmaddev, wait_queue_head_t* io_q,
                              U32* io_f, spinlock_t* plock0); 
void maddev_get_pciconfig_sim(void* pPciCnfg, U32 Len);
int  maddev_claim_pci_resrcs(struct pci_dev* pPcidev, 
                             PMADDEVOBJ pmaddev, char* Devname, U16 NumIrqs);
int maddev_release_pci_resrcs(PMADDEVOBJ pmaddev);
int  maddev_exchange_sim_parms(struct pci_dev** ppPcidev, PMADDEVOBJ pmaddev);
void maddev_init_io_parms(PMADDEVOBJ pmaddev, uint32_t indx);
void* maddev_getkva(phys_addr_t PhysAddr, struct page** ppPgStr);
void maddev_putkva(struct page* pPgStr);
ssize_t 
maddev_xfer_pages_direct(struct mad_dev_obj *pmaddev, int num_pgs,
                         struct page* page_list[], U32 offset, bool bH2D);
ssize_t maddev_xfer_dma_page(PMADDEVOBJ pmaddev, struct page* pPage,
                             sector_t sector, bool bH2D);
ssize_t maddev_xfer_sgdma_pages(PMADDEVOBJ pmaddev, 
                               long num_pgs, struct page* page_list[],
                               sector_t sector, bool bH2D);
U32 maddev_dma_map_sglist(struct device* pdev, struct page* page_list[], 
                          struct scatterlist sgl[],
                          U32 num_pgs, enum dma_data_direction dir);
long maddev_get_user_pages(U64 usrbufr, U32 num_pgs,
                           struct page** pPages, struct vm_area_struct** pVMAs,
                           bool bUpdate);
void maddev_put_user_pages(struct page** ppPages, u32 num_pgs);
//void maddev_put_user_pages(struct page* page_list[], u32 num_pgs);
//
int maddev_kobject_init(struct kobject* pkobj, struct kobject* pPrnt,
                        struct kset* kset, struct kobj_type* ktype, const char *objname);
//int mad_kset_add(void);
int maddev_kobject_register(struct kobject* pkobj, struct kobject* pPrnt, 
                            const char *objname);
void maddev_kobject_unregister(struct kobject* pkobj);
int maddev_kset_create(void);
irq_handler_t maddev_select_isr_function(int msinum);
void maddev_vma_open(struct vm_area_struct* vma);
void maddev_vma_close(struct vm_area_struct* vma);
void maddev_kset_unregister(void);

//inline functions
#ifdef _MAD_SIMULATION_MODE_
// Set sgl parms to enable the simulator to work with the "CPU-view" 
// and use kmap_local_page(pSgle->hpage);
static inline void maddev_assign_hwsgle(__iomem MAD_DMA_CHAIN_ELEMENT *pHwSgle, 
                                        struct scatterlist *psgle)
{
    WARN_ON_ONCE(pHwSgle->hoffset + pHwSgle->hlen > PAGE_SIZE);
    iowrite64((U64)sg_page(psgle), &pHwSgle->hpage);
    iowrite32(psgle->offset,  &pHwSgle->hoffset);
    iowrite32( psgle->length, &pHwSgle->hlen);
}                                        
#endif

static inline void 
maddev_program_sgdma_regs(__iomem MAD_DMA_CHAIN_ELEMENT *pHwSgle, 
                          struct scatterlist *psgle,
                          dma_addr_t HostAddr, U64 DevDataOfst, U32 DmaCntl,
                          U32 DXBC, U64 CDPP)
{
    PINFO("maddev_program_sgdma_regs... pHwSgle=%px HostPA=x%llX DevDataOfst=x%llX DXBC=x%X CDPP=x%llX\n",
          pHwSgle, HostAddr, DevDataOfst, (uint32_t)DXBC, CDPP);
    
    ASSERT((int)((DXBC % MAD_SECTOR_SIZE) == 0));
    iowrite64(HostAddr,    &pHwSgle->HostAddr);
    iowrite32(DevDataOfst, &pHwSgle->DevDataOfst);
    iowrite32(DmaCntl,     &pHwSgle->DmaCntl); 
    iowrite32(DXBC,        &pHwSgle->DXBC);
    iowrite64(CDPP,        &pHwSgle->CDPP);
   
    #ifdef _MAD_SIMULATION_MODE_
    maddev_assign_hwsgle(pHwSgle, psgle);
    #endif
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

static inline void maddev_program_io_regs(__iomem MADREGS *pmadregs, ULONG ControlReg, 
                                          ULONG IntEnableReg, phys_addr_t HostAddr)
{
    iowrite32(0, &pmadregs->IntID);
    iowrite32(IntEnableReg, &pmadregs->IntEnable);
    iowrite64(HostAddr, &pmadregs->HostPA);
    iowrite32(ControlReg, &pmadregs->Control);
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
static inline void 
maddev_acquire_lock_disable_ints(spinlock_t* splock, U32* Flags)
{
    ulong flags; 
    ASSERT((int)(spin_is_locked(splock) == 0));
    spin_lock_irqsave(splock, flags);
    lockdep_assert_held(splock);
    *Flags = flags;
}

//This function 1st restores the previous interrupt state on the current processor
//and then releases the device spinlock  
static inline void 
maddev_enable_ints_release_lock(spinlock_t* splock, U32* Flags)
{
    lockdep_assert_held(splock);
    spin_unlock_irqrestore(splock, (const ulong)*Flags); 
}

static inline void 
maddev_sg_set_pages(u32 num_pgs, struct page* page_list[], 
                    struct scatterlist sglist[])
{    
    ulong j;

    sg_init_table(sglist, num_pgs);

    for (j=0; j < num_pgs; j++)
        {
        sg_set_page(&sglist[j], page_list[j], PAGE_SIZE, 0);

        // Because we don't know the state of the kernel #define CONFIG_NEED_SG_DMA_LENGTH
        //Just belt and suspenders
        #ifdef _MAD_SIMULATION_MODE_
            sglist[j].dma_length = PAGE_SIZE;
            sglist[j].length = PAGE_SIZE;
        #endif
        }
}

static inline void 
maddev_dma_unmap_sglist(struct device* pdev, struct scatterlist sgl[],
                        u32 num_pgs, enum dma_data_direction dir)
{
    dma_unmap_sg(pdev, sgl, num_pgs, dir); 
}

static inline dma_addr_t
maddev_dma_map_page(struct device* pdev, struct page* pPage, u32 offset, u32 size, bool bH2D)
{
    dma_addr_t dma_addr;
    //BUG_ON(!(VIRT_ADDR_VALID(page_to_virt(pPage), size)));
    BUG_ON(dma_get_mask(pdev) == 0); //If dma mask not set
    dma_addr = dma_map_page(pdev, pPage, offset, size, DMADIR(bH2D));

    return dma_addr;
}

static inline void
maddev_dma_unmap_page(struct device* pdev, dma_addr_t dma_addr, U32 size, bool bH2D)
{
    dma_unmap_page(pdev, dma_addr, size, DMADIR(bH2D));
}

static inline phys_addr_t Next_CDPP(U64 CDPP)
{
    return (phys_addr_t)(CDPP + sizeof(MAD_DMA_CHAIN_ELEMENT));
}

static inline __iomem MAD_DMA_CHAIN_ELEMENT  *
Next_HWSGLE(MAD_DMA_CHAIN_ELEMENT __iomem *pDevSgle, phys_addr_t NextCDPP)
{
    ASSERT((int)(NextCDPP != MAD_DMA_CDPP_END));    
    MAD_DMA_CHAIN_ELEMENT __iomem *pHwSgle = pDevSgle;
    return  (pHwSgle++);
}

#if 0 //int maddev_kobject_register(struct kobject* pkobj, struct kobject* pPrnt, const char *objname)
{
    int rc; 

    //rc = kobject_set_name(pkobj, objname);
    //if (rc == 0)
    //{rc = kobject_register(pkobj);}
    rc = kobject_init_and_add(pkobj, &mad_ktype, pPrnt, objname);
    if (rc != 0)
        {PINFO( "mad_kobject_register failed!... rc=%d\n", rc);}

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
#endif //0
//
#endif //_MAD_DRVR_DEFS_
