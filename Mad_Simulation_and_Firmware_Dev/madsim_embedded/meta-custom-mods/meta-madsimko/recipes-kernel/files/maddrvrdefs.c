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
/*  Module NAME : maddrvrdefs.c                                                */
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
/* $Id: maddrvrdefs.c, v 1.0 2021/01/01 00:00:00 htf $                         */
/*                                                                             */
/*******************************************************************************/

#include <asm/io.h>

extern int mad_pci_devid;
extern int maddev_max_devs;
extern int reg_blkdev_major;

static struct driver_private maddrvr_priv_data =
{
    .driver = NULL,
};

static struct pci_driver maddev_driver =
{
	.driver = {.name = maddrvrname, .owner = THIS_MODULE, .p = &maddrvr_priv_data,},
    //
	.id_table = pci_ids,
	.probe    = maddev_probe,
    .suspend  = NULL,
    .resume   = NULL,
	.remove   = maddev_remove_pcidev,
    .shutdown = maddev_shutdown,
};

#if 0
static struct klist maddev_klist;
static struct device_private maddev_priv_data = 
{
    .knode_driver = {.n_klist = &maddev_klist,},
    .dead = 0,
};
#endif

//Function definitions - only to appear in the main module of multiple device drivers
int maddev_probe(struct pci_dev *pcidev, const struct pci_device_id *ids)
{
    int rc = MAD_NO_VENDOR_DEVID_MATCH; //-ECONNREFUSED;
    U32 devnum;
    PMADDEVOBJ pmaddev = NULL;
    bool bMSI = 0;

    ASSERT((int)(VIRT_ADDR_VALID(pcidev, sizeof(struct pci_dev))));
    devnum = pcidev->dev.id;
    pmaddev = (PMADDEVOBJ)((U8*)mad_dev_objects + (MADDEV_UNIT_SIZE * devnum)); 

	PINFO("maddev_probe... slotnum=%d pcidev=%px vendor=x%X pci_devid=x%X pmaddev=%px\n",
          (int)devnum, pcidev, pcidev->vendor, pcidev->device, pmaddev);

    if (pcidev->vendor != MAD_PCI_VENDOR_ID)
        {return rc;}

    #ifdef _CDEV_
     if ((pcidev->device < MAD_PCI_CHAR_INT_DEVICE_ID) || 
        (pcidev->device > MAD_PCI_CHAR_MSI_DEVICE_ID)) 
        {return rc;}

    bMSI = (pcidev->device == MAD_PCI_CHAR_MSI_DEVICE_ID);
    #endif

    #ifdef _BLOCKIO_
    if ((pcidev->device < MAD_PCI_BLOCK_INT_DEVICE_ID) || 
        (pcidev->device > MAD_PCI_BLOCK_MSI_DEVICE_ID)) 
        {return rc;}

    bMSI = (pcidev->device == MAD_PCI_BLOCK_MSI_DEVICE_ID);
    #endif

    pmaddev->devnum = devnum;
    pmaddev->pPcidev = pcidev;

    // Build the device object... 
	return maddev_setup_device(pmaddev, &pmaddev->pPcidev, true, bMSI);
}
//
void maddev_shutdown(struct pci_dev *pcidev)
{
    ASSERT((int)(VIRT_ADDR_VALID(pcidev, sizeof(struct pci_dev))));
   	PINFO("maddev_shutdown... pcidev=%px\n", pcidev);

    //What else to do other than remove ?
    maddev_remove_pcidev(pcidev);

    return;
}

void maddev_remove_pcidev(struct pci_dev *pcidev)
{
    U32 devnum; 
    struct mad_dev_obj *pmaddev; 
    ASSERT((int)(VIRT_ADDR_VALID(pcidev, sizeof(struct pci_dev))));

    devnum = pcidev->dev.id;
    pmaddev = (PMADDEVOBJ)((u8*)mad_dev_objects + (MADDEV_UNIT_SIZE * devnum)); 
   
	PINFO("maddev_remove... devnum=%d pmaddev=%px pcidev=%px\n",
          (int)devnum, pmaddev, pcidev);

    if (devnum > 0)
        if (devnum <= maddev_max_devs)
            {maddev_remove_device(pmaddev);}
}

void maddev_release_device(struct device *pdev)
{
  
    struct pci_dev *ppcidev = container_of(pdev, struct pci_dev, dev);
    //struct mad_dev_obj *pmaddev = 
    //                    container_of(ppcidev, struct mad_dev_obj, pPcidev); 
    u32 devnum =  ppcidev->dev.id;
    struct mad_dev_obj *pmaddev = 
           (PMADDEVOBJ)((u8*)mad_dev_objects + (MADDEV_UNIT_SIZE * devnum)); 
   
	PINFO("maddev_release_device... devnum=%d pmaddev=%px pcidev=%px pdev=%px\n",
          (int)devnum, pmaddev, ppcidev, pdev);
    ASSERT((int)(devnum==pmaddev->devnum));
    ASSERT((int)(ppcidev==pmaddev->pPcidev));

    //if (devnum > 0)
    //    if (pmaddev->bReady)
    //        {maddev_remove_device(pmaddev);}
}

static struct kset mad_kset;
//
static char MadKsetNm[] = "madkset";
//
static U8  bMadKset = 0;
//
//#include <linux/kobject.h>
//#include <linux/sysfs.h>

/* Helper macro: choose the right field name for sysfs default attributes */
#if defined(HAVE_KOBJ_TYPE_DEFAULT_GROUPS)
/* If you define this in older trees yourself, skip the probe below */
#elif defined(CONFIG_SYSFS)
/* Probe for the member by relying on designated initializers:
 * we can't test struct members in the preprocessor, so define a wrapper macro.
 */
struct __kobj_type_member_probe { int default_groups; };
#define HAVE_KOBJ_TYPE_DEFAULT_GROUPS 1
#endif
//
#ifdef HAVE_KOBJ_TYPE_DEFAULT_GROUPS
#  define KOBJ_DEFAULT_GROUPS(g) .default_groups = (g)
#else
#  define KOBJ_DEFAULT_GROUPS(g) .default_attrs  = (g)  /* legacy */
#endif

static struct kobj_type mad_ktype = 
{
    .release = NULL,
    .sysfs_ops = NULL,
    //.default_attrs = NULL,
    //KOBJ_DEFAULT_GROUPS(NULL),
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0)
        .default_groups = NULL,
    #else
        .default_attrs  = NULL,
    #endif
};

//This is the open function for the virtual memory area struct
void maddev_vma_open(struct vm_area_struct* vma)
{
	struct mad_dev_obj *pmaddev = vma->vm_private_data;
	PINFO( "maddev_vma_open... dev#=%u\n", pmaddev->devnum);
}

//This is the close function for the virtual memory area struct
void maddev_vma_close(struct vm_area_struct* vma)
{
	struct mad_dev_obj *pmaddev = vma->vm_private_data;
	PINFO( "maddev_vma_close... dev#=%u\n", pmaddev->devnum);
}
//This table specifies the entry points for VM mapping operations
struct vm_operations_struct maddev_remap_vm_ops =
{
		.open  = maddev_vma_open,
		.close = maddev_vma_close,
		//.fault = maddev_vma_fault,
};

/*
 * The "extended" operations -- only seek
 */
//The random access seek function is not currently used
loff_t maddev_llseek(struct file *fp, loff_t off, int whence)
{
	struct mad_dev_obj *pmaddev = fp->private_data;
    __iomem MADREGS *pmadregs = pmaddev->pDevBase; 
	loff_t newpos = 0;
   	U32 flags1 = 0;
    int rc = 0;

	PINFO("maddev_llseek... dev#=%d lseek=%ld\n",
          (int)pmaddev->devnum, (ulong)off);

    switch(whence)
        {
	    case 0: /* SEEK_SET */
		    newpos = off;
		    break;

	    case 1: /* SEEK_CUR */
		    newpos = fp->f_pos + off;
		    break;

	    case 2: /* SEEK_END */
		    //newpos = pmaddev->size + off;
		    break;

	    default: /* can't happen */
		    rc = -EINVAL;
	    }

	if (newpos < 0) 
        {rc = -EINVAL;}

    if (rc < 0)
        {
        PWARN("maddev_llseek... dev#=%d rc=-%d\n", (int)pmaddev->devnum, rc);
        return rc;
        }

    //Align to unit-io size
    newpos = (newpos / MAD_UNITIO_SIZE_BYTES);
    newpos = (newpos * MAD_UNITIO_SIZE_BYTES);

    //Update the device registers
    maddev_acquire_lock_disable_ints(&pmaddev->devlock, &flags1);
    iowrite32(newpos, &pmadregs->ByteIndxRd);
    iowrite32(newpos, &pmadregs->ByteIndxWr);
    maddev_enable_ints_release_lock(&pmaddev->devlock, &flags1);

    //Sets the file_pos for the next read/write 
    fp->f_pos = newpos;

    PINFO("maddev_llseek... dev#=%d location=%ld fpos=%ld\n",
          (int)pmaddev->devnum, (ulong)newpos, (ulong)fp->f_pos);

	return newpos;
}

//This is the function invoked when an application calls the mmap function
//on the device. It returns a virtual mode address for the memory-mapped device
int maddev_mmap(struct file *fp, struct vm_area_struct* vma)
{
	struct mad_dev_obj *pmaddev = fp->private_data;
	//struct inode* inode_str = fp->f_inode;
    U32    pfn              = phys_to_pfn(pmaddev->MadDevPA);
    size_t MapSize          = vma->vm_end - vma->vm_start;
	//
	int rc = 0;

	PINFO("maddev_mmap... dev#=%d fp=%px pfn=x%llX PA=x%llX MapSize=%ld\n",
          (int)pmaddev->devnum, (void *)fp, (unsigned long long)pfn, 
          pmaddev->MadDevPA, (long int)MapSize);

    mutex_lock(&pmaddev->devmutex);

    //Map/remap the Page Frame Number of the phys addr of the device into
    //user mode virtual addr. space
    rc = remap_pfn_range(vma, vma->vm_start, pfn, MapSize, vma->vm_page_prot);
    if (rc != 0)
        {
        mutex_unlock(&pmaddev->devmutex);
        PERR("maddev_mmap:remap_pfn_range... dev#=%d rc=%d\n",
             (int)pmaddev->devnum, rc);
        return rc;
        }

	vma->vm_ops = &maddev_remap_vm_ops;
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(6,3,0)
        vm_flags_set(vma, VM_IO);
    #else
        vma->vm_flags |= VM_IO;
    #endif
	vma->vm_private_data = fp->private_data;
    pmaddev->vma = vma;

    //Increment the reference count on first use
	maddev_vma_open(vma);
    mutex_unlock(&pmaddev->devmutex);

    PINFO("maddev_mmap()... dev#=%d start=%px rc=%d\n",
           (int)pmaddev->devnum, (void *)vma->vm_start, rc);

    return rc;
}

//This function sets up one pci device object
int maddev_setup_device(PMADDEVOBJ pmaddev, struct pci_dev** ppPcidevTmp, 
    U8 bHPL, u8 bMSI)
{
    uint32_t i = pmaddev->devnum;
    dev_t devno = MKDEV(maddev_major, i);
    //
    char devname[30] = "";
    struct pci_dev* pPcidevTmp;
    struct pci_dev* pPcidev;
    struct device   *pdev = NULL;
    int rc = 0;
    u16 num_irqs = 0;
    U32 flags1 = 0;

	PINFO("maddev_setup_device... dev#=%d pmaddev=%px mkdev=%x bHPL=%d bMSI=%d\n",
		  (int)i, pmaddev, devno, bHPL, bMSI);

    WRITE_ONCE(pmaddev->bReady, false);
    mutex_init(&pmaddev->devmutex);
    mutex_lock(&pmaddev->devmutex);

    spin_lock_init(&pmaddev->devlock);
    //Sanity check
    maddev_acquire_lock_disable_ints(&pmaddev->devlock, &flags1);
    maddev_enable_ints_release_lock(&pmaddev->devlock, &flags1);

    #ifdef _MAD_SIMULATION_MODE_
    //Exchange parameters w/ the simulator
    rc = maddev_exchange_sim_parms(&pPcidevTmp, pmaddev);
    if (rc != 0)
		{
        mutex_unlock(&pmaddev->devmutex);
        PERR("maddev_setup_device:mad_exchange_sim_parms... dev#=%d rc=%d\n",
             (int)i, rc);
		return rc;
		}
    #endif

    //Get the PCI device struct if this is not a discover - not a plugin
    if (!bHPL)
        {
        *ppPcidevTmp = 
        pci_get_device(MAD_PCI_VENDOR_ID, mad_pci_devid, *ppPcidevTmp);
        if ((*ppPcidevTmp == NULL) || (IS_ERR(*ppPcidevTmp)))
            {
            mutex_unlock(&pmaddev->devmutex);
            PERR("maddev_setup_device... dev#=%d pci_get_device rc=%d\n",
                 (int)i, (int)PTR_ERR(*ppPcidevTmp));
            return (int)PTR_ERR(*ppPcidevTmp);
            }
        }
    //
    pPcidevTmp = *ppPcidevTmp;
    pPcidevTmp->msi_cap = bMSI;

    //Assign the device name for this device before acquiring resources
    #ifdef _CDEV_
        snprintf(devname, sizeof(devname), "maddevc_obj%d", (int)i);
    #endif
    #ifdef _BLOCKIO_
        snprintf(devname, sizeof(devname), "maddevb_obj%d", (int)i);
    #endif
    
    num_irqs = (bMSI) ? MAD_NUM_MSI_IRQS : 0;
    num_irqs++;
    rc = maddev_claim_pci_resrcs(pPcidevTmp, pmaddev, devname, num_irqs);
    if (rc != 0)
		{
        mutex_unlock(&pmaddev->devmutex);
        PERR("maddev_setup_device:maddev_claim_pci_resrcs... dev#=%u rc=%d\n",
		     (int)i, rc);
		return rc;
		}

    //If we discovered the device at startup - not hotplugged
    if (!bHPL) 
        {pmaddev->pPcidev = pPcidevTmp;}
 
    mutex_unlock(&pmaddev->devmutex);

    pPcidev = pmaddev->pPcidev;
    pdev = &pPcidev->dev;
    mutex_lock(&pmaddev->devmutex);

    #ifdef _MAD_SIMULATION_MODE_
    //We must completely implement what the PCI core would have set up by now
    pPcidev->driver = &maddev_driver;
    pPcidev->msi_cap = bMSI;
   
    //Configure & register the generic device w/in pci_dev for the sysfs tree
    sim_init_sysfs_device(pdev, pmaddev->devnum, devname, 
                          (struct device_driver *)&maddev_driver, pmaddev,
                          maddev_release_device);
    rc = register_device(pdev
                         #ifdef _MAD_SIMULATION_MODE_ 
                         // The simulator needs the device # & name
                         , i, devname  //MadDevNames[i]
                         #endif
                         );
	if (rc == 0) 
        {pmaddev->bDevRegstrd = true;}
    else
		{
        pmaddev->bDevRegstrd = false;
        mutex_unlock(&pmaddev->devmutex);
        maddev_release_pci_resrcs(pmaddev);
        PERR("maddev_setup_device:register_device... dev#=%d rc=%d\n",
              (int)i, rc);
        return rc;
        }
     #endif // _MAD_SIMULATION_MODE_
    
    pmaddev->pdevnode = pdev;
    #ifdef _BLOCKIO_
        dma_set_max_seg_size(pmaddev->pdevnode, PAGE_SIZE);
        dma_set_seg_boundary(pmaddev->pdevnode, PAGE_SIZE - 1);
    #endif

    pmaddev->pdevnode->dma_mask = &pmaddev->pdevnode->coherent_dma_mask;
    rc = dma_set_mask_and_coherent(pmaddev->pdevnode, DMA_BIT_MASK(64));
    if (rc != 0)
        {
        maddev_release_pci_resrcs(pmaddev);
        mutex_unlock(&pmaddev->devmutex);
        unregister_device(pmaddev->pdevnode, pmaddev->devnum);
        PERR("maddev_setup_device:dma_set_mask... dev#=%d rc=%d\n", (int)i, rc);
        return rc;
        }
 
    #ifdef _CDEV_
	    rc = maddevc_setup_cdev(pmaddev, i);
        mutex_unlock(&pmaddev->devmutex);
    #endif

    #ifdef _BLOCKIO_
        {
        //Set up a target device for fs-open, mmap, ioctls
        char cache_name[20];
        snprintf(cache_name, 20, "maddevb_cmd_cache%d", (int)pmaddev->devnum);
        pmaddev->pcmd_cache = 
        kmem_cache_create(cache_name, sizeof(struct maddevb_cmd),
                          __alignof__(struct maddevb_cmd), 
                          (SLAB_HWCACHE_ALIGN|REQ_NOMERGE), NULL);
        
        rc = (pmaddev->pcmd_cache==NULL) ? -ENOMEM : 0;
        if (rc == 0)
    	    rc = maddevr_setup_cdev(pmaddev, i);

        if (rc == 0)
            {
            mutex_unlock(&pmaddev->devmutex);
            rc = maddevb_create_blockdev(pmaddev);
            }
        
        atomic_set(&pmaddev->batomic, 0);
        if (mutex_trylock(&pmaddev->devmutex))
            {mutex_unlock(&pmaddev->devmutex);}
        }
    #endif

    if (rc != 0)
		{
        maddev_release_pci_resrcs(pmaddev);
        PERR("maddev_setup_device... dev#=%d rc=%d\n", (int)i, rc);
        //maddev_remove_device(pmaddev);
        return rc;
		}

     //Create a device node in the mad_class... the equivalent to mknod in BASH
    pmaddev->pclassdev = device_create(mad_class, pdev, /* The parent device */ 
		                               devno, NULL, /* no additional data */
                                       devname);
    if (IS_ERR(pmaddev->pclassdev)) 
        {
        rc = PTR_ERR(pmaddev->pclassdev);
        pmaddev->pclassdev = NULL;
		PWARN("maddev_setup_device:device_create... dev#=%d mkdev=%u rc=%d\n",
              (int)i, devno, rc);
        //device_create failure may not be fatal
        }

    maddev_init_io_parms(pmaddev, i);
    WRITE_ONCE(pmaddev->bReady, true);

    PINFO("maddev_setup_device... dev#=%d pdevobj=%px mkdev=%x devVA=%px devPA=x%llX rc=%d\n",
          (int)i, (void *)pmaddev,  (uint32_t)devno, 
          pmaddev->pDevBase, pmaddev->MadDevPA, rc);

    //Release our quantum - let waiting threads run
  	schedule(); 

    return rc;
}

int maddev_setup_devices(int num_devs, U8 bHPL, u8 bMSI) 
{
    int i;
    int devcount = 0;
    int rc;
    PMADDEVOBJ  pmaddev;
    struct pci_dev* pPcidevTmp = NULL;

	for (i = 1; i <= num_devs; i++)
	    {
        pmaddev = (PMADDEVOBJ)((u8*)mad_dev_objects + (MADDEV_UNIT_SIZE * i));
        pmaddev->devnum = i;
        rc = maddev_setup_device(pmaddev, &pPcidevTmp, false, bMSI); 
        if (rc != 0)
            {
            PWARN("maddev_init_module:maddev_setup_device... dev#=%d rc=%d\n",
                  (int)pmaddev->devnum, rc);
            continue;
            }

        devcount++;
	    }

    PINFO("maddev_setup_devices()... #devices=%d\n\n", devcount);

    return devcount;
}

//This function tears down one device object
void maddev_remove_device(PMADDEVOBJ pmaddev)
{
    int rc;
    int devnum = (int)pmaddev->devnum;
    dev_t devno = MKDEV(maddev_major, devnum);

	PINFO("maddev_remove_device()... dev#=%d pmaddev=%px\n",
		  devnum, (void *)pmaddev);

    if ((devnum < 1) || (devnum > maddev_max_devs)) 
        {
        PERR("maddev_remove_device()... invalid dev#=%d\n", devnum);
        return;
        }

    pmaddev->bReady = false;
    tasklet_kill(&pmaddev->dpctask); 

    #ifdef _CDEV_
	//cdev_del(&pmaddev->cdev_str);
    #endif
   
    #ifdef _BLOCKIO_
    if (pmaddev->pmdblkdev != NULL)
        maddevb_delete_blockdev(pmaddev->pmdblkdev);
    
    if (pmaddev->pcmd_cache != NULL)
        {
        kmem_cache_shrink(pmaddev->pcmd_cache);
        kmem_cache_destroy(pmaddev->pcmd_cache);
        pmaddev->pcmd_cache = NULL;
        }
    
    if (pmaddev->cdev_added) 
        cdev_del(&pmaddev->cdev_str);
    #endif

   if (pmaddev->pclassdev != NULL)
        {device_destroy(mad_class, devno);}

    if (pmaddev->bDevRegstrd)
        {
        unregister_device(&pmaddev->pPcidev->dev, pmaddev->devnum);
        pmaddev->bDevRegstrd = false;
        }
    else
        {PWARN("maddev_remove_device... dev#=%d not registered! (%px)\n",
              (int)pmaddev->devnum, (void *)pmaddev);}

    #if 0
    maddev_kobject_unregister(&pmaddev->pPcidev->dev.kobj);
    #endif

    rc = maddev_release_pci_resrcs(pmaddev);

    PINFO("maddev_remove_device... dev#=%u exit\n", pmaddev->devnum);
}

void maddev_remove_devices(int num_devices)
{
	U32 i;
    PMADDEVOBJ pmaddev = NULL;

	PINFO("maddev_remove_devices()... num devices=%d\n", num_devices);
    for (i = 1; i <= num_devices; i++)
        {
        pmaddev = (PMADDEVOBJ)((u8*)mad_dev_objects + (MADDEV_UNIT_SIZE * i));
        if (pmaddev == NULL)
            {continue;}

        if (pmaddev->devnum > 0) //If maddev_setup_device assigned a device#
            {
            ASSERT((int)(i == pmaddev->devnum));
            if (pmaddev->bReady == true) //if not unplugged
                {maddev_remove_device(pmaddev);}
            else
                {PWARN("maddev_remove_devices... i=%d dev#=%d pmadobj=%px device not active!\n",
                       (int)i, (int)pmaddev->devnum, (void *)pmaddev);}
            }
        }

    PINFO("maddev_remove_devices() exit... \n");
}

/* The cleanup function is used to handle initialization failures as well.
 * Thefore, it must be careful to work correctly even if some of the items
 * have not been initialized 
 */ 
void maddev_cleanup_module(void)
{
    #ifdef _CDEV_
	dev_t devno = MKDEV(maddev_major, maddev_minor);
    #endif

	PINFO("maddev_cleanup_module... mjr=%d mnr=%d\n", maddev_major, maddev_minor);

	/* Release our char dev entries */
	if (mad_dev_objects != NULL)
        {
        maddev_remove_devices(maddev_nbr_devs);
	    kfree(mad_dev_objects);
        }

#ifdef MADDEVOBJ_DEBUG /* use proc only if debugging */
	maddev_remove_proc();
#endif

    if (mad_class != NULL)
        {class_destroy(mad_class);}

    if (maddev_driver.driver.p->driver != NULL)    
	    {pci_unregister_driver(&maddev_driver);}

	/* cleanup_module is never called if registering failed */
    #ifdef _CDEV_
	unregister_chrdev_region(devno, maddev_nbr_devs);
    #endif

    #ifdef _BLOCKIO_
    unregister_blkdev(reg_blkdev_major, MAD_MAJOR_DEVICE_NAME);
    #endif

   	PINFO("maddev_cleanup_module... exit :)\n\n");
}   
 
   
irq_handler_t maddev_select_isr_function(int msinum)
{
    irq_handler_t isr_function = NULL;

    switch (msinum)
        {
        #ifdef  _CDEV_
        case 0:
            isr_function = maddevc_legacy_isr;
            break; 

        case 1:
            isr_function = maddevc_msi_one_isr;
            break; 

        case 2:                                              
            isr_function = maddevc_msi_two_isr; 
            break;

        case 3:
            isr_function = maddevc_msi_three_isr; 
            break;

        case 4:
            isr_function = maddevc_msi_four_isr; 
            break;

         case 5:
             isr_function = maddevc_msi_five_isr; 
             break;

         case 6:
            isr_function = maddevc_msi_six_isr; 
            break;

          case 7:
            isr_function = maddevc_msi_seven_isr; 
            break;

          case 8:
              isr_function = maddevc_msi_eight_isr; 
              break;
        #endif

        #ifdef  _BLOCKIO_
        case 0:
            isr_function = maddevb_legacy_isr;
            break; 

        case 1:
            isr_function = maddevb_msi_one_isr;
            break; 

        case 2:                                              
            isr_function = maddevb_msi_two_isr; 
            break;

        case 3:
            isr_function = maddevb_msi_three_isr; 
            break;

        case 4:
            isr_function = maddevb_msi_four_isr; 
            break;

         case 5:
             isr_function = maddevb_msi_five_isr; 
             break;

         case 6:
            isr_function = maddevb_msi_six_isr; 
            break;

          case 7:
            isr_function = maddevb_msi_seven_isr; 
            break;

          case 8:
              isr_function = maddevb_msi_eight_isr; 
              break;
        #endif

          default:;
             PERR("maddev_claim_pci_resrcs:maddev_select_isr_function() invalid msi#(%d)\n", msinum);
        }

    return isr_function;

}

int maddev_claim_pci_resrcs(struct pci_dev* pPcidev, PMADDEVOBJ pmaddev,
                            char* DevName, u16 NumIrqs)
{
    long int irqflags = IRQF_SHARED; //IRQF_TRIGGER_HIGH | IRQF_TRIGGER_LOW;
    long int alloc_irq_flags = NR_IRQS_LEGACY;
    U32 j = 0;
    //
    U8 bMSI = pPcidev->msi_cap;
    irq_handler_t isr_function = NULL;
    int rc = 0;
    phys_addr_t BaseAddr = 0;
    phys_addr_t BCDPP = 0;
    phys_addr_t CachePA = 0;
    U64 Bar0end       = 0;
    __iomem MADREGS *pmadregs = NULL;
    //uint32_t regval;

    uint32_t ResFlags;
    U64 PciCnfgU64_0 = 0;
    U64 PciCnfgU64_2 = 0;
    U64 PciCnfgU64_4 = 0;
    uint32_t PciCnfgLo = 0;
    uint32_t PciCnfgHi = 0;
    U8  PciCnfgU8;

    rc = pci_enable_device(pPcidev);
	if (rc != 0)
		{
		PERR("maddev_claim_pci_resrcs:pci_enable_device... dev=%d rc=%d\n",
             (int)pmaddev->devnum, rc);
        return rc;
        }

    // Determine the device phys addr from Pci-Core *AND* PCI config space
    BaseAddr = pci_resource_start(pPcidev, 0);
    Bar0end  = pci_resource_end(pPcidev, 0);
    //MapLen   = (U32)Bar0end - BaseAddr + 1;
    ResFlags = pci_resource_flags(pPcidev, 0);
    
    BCDPP    = pci_resource_start(pPcidev, 2);
    CachePA  = pci_resource_start(pPcidev, 4);

    PINFO("pci_resource_start... dev#=%d, BaseAddr=x%llX BCDPP=x%llX CachePA=x%llX\n",
          (int)pmaddev->devnum, BaseAddr, BCDPP, CachePA);

    rc = pci_read_config_dword(pPcidev, PCI_BASE_ADDRESS_0, &PciCnfgLo);
    if (rc == 0)
        {rc = pci_read_config_dword(pPcidev, (PCI_BASE_ADDRESS_1), &PciCnfgHi);}
    if (rc == 0)    
        {PciCnfgU64_0 = ((U64)((U64)PciCnfgHi << 32) + PciCnfgLo);}

    if (rc == 0)
        {rc = pci_read_config_dword(pPcidev, PCI_BASE_ADDRESS_2, &PciCnfgLo);}
    if (rc == 0)    
        {rc = pci_read_config_dword(pPcidev, (PCI_BASE_ADDRESS_3), &PciCnfgHi);}
    if (rc == 0)
        {PciCnfgU64_2 = ((U64)((U64)PciCnfgHi << 32) + PciCnfgLo);}

    if (rc == 0)
        {rc = pci_read_config_dword(pPcidev, (PCI_BASE_ADDRESS_4), &PciCnfgLo);}
    if (rc == 0)
        {rc = pci_read_config_dword(pPcidev, (PCI_BASE_ADDRESS_5), &PciCnfgHi);}
    if (rc == 0)
        {PciCnfgU64_4 = ((U64)((U64)PciCnfgHi << 32) + PciCnfgLo);}

    if (rc != 0)
        {PWARN("maddev_claim_pci_resrcs:pci_read_config_dword... dev#=%d rc=%d\n",
               (int)pmaddev->devnum, rc);}
    
    PINFO("pci_read_config_dword(s)... dev#=%d PciCnfgU64_0=x%llX PciCnfgU64_2=x%llX PciCnfgU64_4=x%llX\n",
          (int)pmaddev->devnum, PciCnfgU64_0, PciCnfgU64_2, PciCnfgU64_4);

    //Confirm that we read the same values two different ways
    ASSERT((int)(BaseAddr == PciCnfgU64_0));      
    ASSERT((int)(BCDPP == PciCnfgU64_2));   
    ASSERT((int)(CachePA == PciCnfgU64_4));

    //Confirm that the simulator defines the device the way we expect
    SIMULATION_ASSERT((int)(BCDPP == (BaseAddr + MADREGS_BASE_SIZE)));
    SIMULATION_ASSERT((int)(CachePA == (BaseAddr + MAD_MAPD_READ_OFFSET)));

    rc = pci_request_region(pPcidev, 0, DevName);
    if (rc != 0)
        {
	    PERR("maddev_claim_pci_resrcs:pci_request_region... dev=%d rc=%d\n",
             (int)pmaddev->devnum, rc);
        return rc;
        }

    //Get a kernel virt addr for the device
    pmadregs = pci_iomap(pPcidev, 0, sizeof(MADREGS));
	if (pmadregs == NULL)
        {
        pci_release_region(pPcidev, 0);
        PERR("maddev_claim_pci_resrcs:pci_iomap... dev=%d rc=%d\n",
             (int)pmaddev->devnum, rc);
        return -ENOMEM;
        }

    pmaddev->MadDevPA = BaseAddr;
    //SIMULATION_ASSERT((int)(pmadregs == phys_to_virt(pmaddev->MadDevPA)));
    iowrite32(pmaddev->devnum, &pmadregs->Devnum);
    iowrite64(BCDPP, &pmadregs->BCDPP);
    pmaddev->pDevBase  = pmadregs;
 
    rc = pci_read_config_byte(pPcidev, PCI_INTERRUPT_LINE, &PciCnfgU8);
    if (rc != 0)
        {
	    PERR("maddev_claim_pci_resrcs:pci_read_config_dword... dev=%d rc=%d\n",
             (int)pmaddev->devnum, rc);
        pci_iounmap(pPcidev, pmaddev->pDevBase);
        return rc;
        }

    //Optional integrity check - verfiying one retrieve of Irq vs another
    //ASSERT((int)((int)PciCnfgU8 == pPcidev->irq));

    if (bMSI)
        {
        //rc = pci_enable_msi_block(pPcidev, NumIrqs);
        rc = pci_alloc_irq_vectors(pPcidev, 1, NumIrqs,
                                   (alloc_irq_flags | PCI_IRQ_MSI));
        if (rc != 0)
	        {
            pci_iounmap(pPcidev, pmaddev->pDevBase);
            pci_release_region(pPcidev, 0);
            PERR("maddev_claim_pci_resrcs:pci_enable_alloc_irq_vectors... dev=%d rc=%d\n",
		         (int)pmaddev->devnum, rc);
	        return rc;
	        }
        }

    pmaddev->base_irq = pPcidev->irq;

    //We need to wire the multiple irqs to the corresponding isr function
    for (j=0; j < NumIrqs; j++)
        {
        isr_function = maddev_select_isr_function((int)j);
        if (isr_function != NULL)
            {rc = request_irq((pmaddev->base_irq + j), isr_function,
                              irqflags, DevName, pmaddev);}
        if (rc != 0)
            {break;} //Any request_irq failure is fatal
        }

    if (rc != 0)
        {
        pci_free_irq_vectors(pPcidev);
        pci_iounmap(pPcidev, pmaddev->pDevBase);
        pci_release_region(pPcidev, 0);

        PERR("maddev_claim_pci_resrcs:request_irq(%d)... dev=%d j=%d rc=%d\n",
             (int)(pPcidev->irq+j), (int)pmaddev->devnum, (int)j, rc);
        }

    PINFO("maddev_claim_pci_resrcs... dev#=%u rc=%d\n", pmaddev->devnum, rc);

    return rc;
}

// This function releases PCI-related resources
int maddev_release_pci_resrcs(PMADDEVOBJ pmaddev)
{
    register U32 j = 0;
    int rc = 0;

    PINFO("maddev_release_pci_resrcs... dev#=%d\n", (int)pmaddev->devnum);

    rc = free_irq(pmaddev->base_irq, pmaddev);
    if (pmaddev->pPcidev->msi_cap != 0)
        {
        for (j=1; j <= (MAD_NUM_MSI_IRQS); j++)
            {rc = free_irq((pmaddev->base_irq+j), pmaddev);}

        pci_disable_msi(pmaddev->pPcidev);
        //pci_free_irq_vectors()
        }

    pci_free_irq_vectors(pmaddev->pPcidev);
    pci_iounmap(pmaddev->pPcidev, pmaddev->pDevBase);
    pci_release_region(pmaddev->pPcidev, 0);
    rc = pci_disable_device(pmaddev->pPcidev);

    return rc;
}

ssize_t maddev_xfer_dma_page(PMADDEVOBJ pmaddev, struct page* pPage,
                             sector_t sector, bool bH2D)
{
    ssize_t lrc = -EADDRNOTAVAIL;
    dma_addr_t HostPA; 
    struct page* page_list[3] = {pPage, NULL, NULL};

    HostPA = maddev_dma_map_page(pmaddev->pdevnode, pPage, 0, PAGE_SIZE, bH2D);
    if (dma_mapping_error(pmaddev->pdevnode, HostPA))
        {
        PERR("maddev_xfer_dma_page... dev#=%d rc=%ld\n",
             (int)pmaddev->devnum, (long int)lrc);
        return lrc;
        }

    PINFO("maddev_xfer_dma_page... dev#=%d PA=x%llX sector=%ld wr=%d\n",
          (int)pmaddev->devnum, (long long int)HostPA, (long int)sector, bH2D);

    lrc = maddev_xfer_sgdma_pages(pmaddev, 1, page_list, sector, bH2D);

    maddev_dma_unmap_page(pmaddev->pdevnode, HostPA, PAGE_SIZE, bH2D);

    return lrc;
}

ssize_t maddev_xfer_sgdma_pages(PMADDEVOBJ pmaddev, 
                                long num_pgs, struct page* page_list[],
                                sector_t sector, bool bH2D)
{    
    __iomem MADREGS *pmadregs = pmaddev->pDevBase;
    phys_addr_t BCDPP = pmadregs->BCDPP; //(U64)virt_to_phys(&pmaddev->SgDmaUnits[0]);
    phys_addr_t CDPP  = BCDPP;
    __iomem MAD_DMA_CHAIN_ELEMENT *pHwSgle = pmadregs->SgDmaUnits;
    U32 DevDataOfst = (sector * MAD_SECTOR_SIZE);
    long   loop_lmt = num_pgs - 1; 
    U32    IntEnable = 
           (bH2D) ? (MAD_INT_STATUS_ALERT_BIT | MAD_INT_DMA_OUTPUT_BIT) :
                    (MAD_INT_STATUS_ALERT_BIT | MAD_INT_DMA_INPUT_BIT);
    enum dma_data_direction dmadir = DMADIR(bH2D);
    //
    U32    DmaCntl = 
           (bH2D) ? (MAD_DMA_CNTL_INIT | MAD_DMA_CNTL_H2D) : MAD_DMA_CNTL_INIT;
    U32    CntlReg = MAD_CONTROL_CHAINED_DMA_BIT;
    long   iostat;
    size_t iocount = 0;

    ssize_t lrc;
    u32 sgl_num = 0;
    u32 sgelen = 0;

    U32 flags1 = 0; 
    U32 j;

    PINFO("maddev_xfer_sgdma_pages... dev#=%d BCDPP=%llx num_pgs=%ld sector=%ld pglist=%px wr=%d\n",
          (int)pmaddev->devnum, BCDPP, num_pgs, 
          (long int)sector, page_list, bH2D);

    if (num_pgs > MAD_SGDMA_MAX_PAGES)
        {
        lrc = -EINVAL;
        PERR("maddev_xfer_sgdma_pages... dev#=%d num_pgs=(%ld) > max(%ld) rc=%ld\n",
             (int)pmaddev->devnum, num_pgs, (long int)MAD_SGDMA_MAX_PAGES, (long int)lrc);
        ASSERT((int)false);
        return lrc;
        }
 
    sgl_num = maddev_dma_map_sglist(pmaddev->pdevnode, page_list, 
                                    pmaddev->sglist, num_pgs, dmadir);
    if (sgl_num < 0)
        {
        PERR("maddev_xfer_sgdma_pages... dev#=%d rc=%d\n",
             (int)pmaddev->devnum, (int)sgl_num);
        return sgl_num;
        }
 
    pmaddev->ioctl_f = eIoPending;

    maddev_acquire_lock_disable_ints(&pmaddev->devlock, &flags1);
    maddev_program_io_regs(pmadregs, CntlReg, IntEnable, (phys_addr_t)0);

    for (j=0; j <= loop_lmt; j++)
        {
        //The set of hardware SG elements is defined as a linked-list even though
        //it is created as an array
        //Assign the phys_addr for the next sg-element in this sg-element
        CDPP = (j == loop_lmt) ? MAD_DMA_CDPP_END : Next_CDPP(CDPP);
                                 //virt_to_phys(&pmadregs->SgDmaUnits[j+1]);

        sgelen = sg_dma_len(&pmaddev->sglist[j]);
        maddev_program_sgdma_regs(pHwSgle, &pmaddev->sglist[j],
                                  sg_dma_address(&pmaddev->sglist[j]),
                                  DevDataOfst, DmaCntl, sgelen, CDPP);
        iocount += sgelen; 
        DevDataOfst += sgelen; 

        //We should use the Chained-Dma-Pkt-Pntr because we treat the list as a
        //linked-list to be proper 
        if (j < loop_lmt)
            {pHwSgle = Next_HWSGLE(pHwSgle, CDPP);} //phys_to_virt(CDPP);}
        }

    //Finally - initiate the i/o
    CntlReg |= MAD_CONTROL_DMA_GO_BIT;
    iowrite32(CntlReg, &pmadregs->Control);
    maddev_enable_ints_release_lock(&pmaddev->devlock, &flags1);

    //Wait and process the results
    iostat = maddev_wait_get_io_status(pmaddev, &pmaddev->ioctl_q,
                                       &pmaddev->ioctl_f,
                                       &pmaddev->devlock);
    iocount = (iostat < 0) ? iostat : pmadregs->DTBC;
    if (iostat == 0)
        {ASSERT((int)(iocount == (num_pgs * PAGE_SIZE)));}

    maddev_dma_unmap_sglist(pmaddev->pdevnode, pmaddev->sglist, sgl_num, dmadir);
    pmaddev->ioctl_f = eIoReset;

    PINFO("maddev_xfer_sgdma_pages... dev#=%d num_pgs=%ld iostat=%ld iocount=%ld\n",
          (int)pmaddev->devnum, num_pgs, (long int)iostat, (long int)iocount);

    return iocount;
}

U32 maddev_dma_map_sglist(struct device* pdev, struct page* page_list[], 
                          struct scatterlist sglist[],
                          U32 num_pgs, enum dma_data_direction dir)
{
    u32 num_sge = num_pgs;  //We don't optimize for physically contiguous pages;

    maddev_sg_set_pages(num_pgs, page_list, sglist);

   //Do a true physical-to-bus-relative mapping
    num_sge = dma_map_sg(pdev, sglist, num_pgs, dir); 
        //rc = dma_mapping_error(pdev, 0)
    if (num_sge < 0)
        {
        PERR("maddev_xfer_sgdma_pages:dma_map_sg() error... pdev=%px rc=%d\n",
             pdev, (int)num_sge);
        }
    
    return num_sge;
}

//This function services both read & write through direct-io
//We mean direct-io between the driver and the user application.
//We will implement 'buffered' io between the host and the hardware device.
//N pages will be copied across the bus - no DMA - working with the index registers
ssize_t maddev_xfer_pages_direct(PMADDEVOBJ pmaddev, int num_pgs, 
                                 struct page* page_list[], U32 offset, bool bH2D)
{
    __iomem MADREGS  *pmadregs = pmaddev->pDevBase; 
    U32          IntEnable = 
                 (bH2D) ? (MAD_INT_STATUS_ALERT_BIT | MAD_INT_BUFRD_OUTPUT_BIT) :
                         (MAD_INT_STATUS_ALERT_BIT | MAD_INT_BUFRD_INPUT_BIT);
    phys_addr_t  HostPA;
    U32     CntlReg;
    U32     CountBits;
    ssize_t iocount = 0;
    long    iostat;

    ASSERT((int)(pmaddev->devnum==pmadregs->Devnum));
    BUG_ON(pmaddev->devnum!=pmadregs->Devnum);

    PINFO("maddev_xfer_pages_direct... dev#=%d num_pgs=%d offset=%u wr=%d\n",
		  (int)pmaddev->devnum, num_pgs,  (uint32_t)offset, bH2D);

    //Declare the specific queue to have a pending io
    if (bH2D) 
        {pmaddev->write_f = eIoPending;}
    else
        {pmaddev->read_f = eIoPending;}

    //If we permitted multiple device i/o's per user i/o we would set up a loop here
    CountBits = maddev_set_count_bits((PAGE_SIZE * num_pgs), MAD_CONTROL_IO_COUNT_MASK,
                                       MAD_CONTROL_IO_COUNT_SHIFT, MAD_SECTOR_SIZE);
    //Do *NOT* set the MAD_CONTROL_IOSIZE_BYTES_BIT
    CntlReg = CountBits;
    //
    HostPA = page_to_phys(page_list[0]);
    BUG_ON(!(virt_addr_valid(phys_to_virt(HostPA))));
    maddevc_program_stream_io(&pmaddev->devlock, pmadregs,
                              CntlReg, IntEnable, HostPA, offset, bH2D);

    //Wait for the io to complete and then process the results
    if (bH2D) 
        {
        iostat = maddev_wait_get_io_status(pmaddev, &pmaddev->write_q,
                                           &pmaddev->write_f,
                                           &pmaddev->devlock);
        if (iostat >= 0)
            {
            iocount = 
            maddev_get_io_count((U32)iostat, MAD_STATUS_WRITE_COUNT_MASK,
                                MAD_STATUS_WRITE_COUNT_SHIFT, MAD_SECTOR_SIZE);
            }
        }
    else
        {
        iostat = maddev_wait_get_io_status(pmaddev, &pmaddev->read_q, 
                                           &pmaddev->read_f,
                                           &pmaddev->devlock);
        if (iostat >= 0)
            {
            iocount = 
            maddev_get_io_count((U32)iostat, MAD_STATUS_READ_COUNT_MASK,
                                MAD_STATUS_READ_COUNT_SHIFT, MAD_SECTOR_SIZE);
            maddev_set_dirty_pages(page_list, num_pgs);
            }
        }

    //If we have an error from the device - that's what we return
    //The specific i/o queue should already be reset in maddev_wait_get_io_status above
    if (iostat < 0)
        {iocount = iostat;}

    //Set the specific i/o queue to ready
    if (bH2D) 
        {pmaddev->write_f = eIoReset;}
    else
        {pmaddev->read_f = eIoReset;}

    PINFO("maddev_xfer_pages_direct... dev#=%d num_pgs=%d iocount=%ld\n",
           (int)pmaddev->devnum, num_pgs, (long int)iocount);

    return iocount;
}

//Determine if we need to build a Scatter-gather list
bool maddev_need_sg(struct page* pPages[], u32 num_pgs)
{
    u32 j = 0;
    phys_addr_t PA[MAD_SGDMA_MAX_PAGES+1];

    if (num_pgs > MAD_DIRECT_XFER_MAX_PAGES) 
        {return true;}

    PA[0]= page_to_phys(pPages[0]);
    if (num_pgs < 2)
        {goto NeedSgXit;}

    //If any pages are not contiguous - we must use scatter-gather
    for (j=1; j < num_pgs; j++)
        {
        PA[j] = page_to_phys(pPages[j]);
        if ((PA[j] - PA[j-1]) != PAGE_SIZE)
            {return true;}
        }

NeedSgXit:;
    #if 1
    //We can DMA a contiguous block - no need for sg-dma
    PINFO("maddev_need_sg... num_pgs=%ld PA0=x%llX PAx=x%llX\n",
           (long int)num_pgs, PA[0], PA[num_pgs-1]);
    #endif

    return false;
}

//This function acquires an array of page structs describing the user buffer
//and locks the buffer pages into RAM. 
//It must first acquire the mmap read-write semaphore for the owning process 
//provided by the macro (current)
long maddev_get_user_pages(U64 usrbufr, U32 nr_pages, struct page *pPages[],
                           struct vm_area_struct *pVMAs[], bool bUpdate)
{
    static long GUP_FLAGS = 0;
    //
    long gup_flags = (GUP_FLAGS | FOLL_WRITE); //either i/o direction
    long num_pgs;

    mmap_read_lock(current->mm);
    num_pgs = get_user_pages((u64)usrbufr, nr_pages, gup_flags, pPages
    #if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 5, 0)
                             ,NULL
    #endif
    );
    mmap_read_unlock(current->mm);

     return num_pgs;
}

void maddev_put_user_pages(struct page** ppPages, u32 num_pgs)
{
    struct page* pPage = *ppPages; 
    u32 j; 
    PINFO("maddev_put_user_pages... num_pgs=%ld pPages=%px pPS0=%px PA0=x%llX\n",
          (long int)num_pgs, ppPages, pPage, virt_to_phys(pPage));
    {
    mmap_read_lock(current->mm);
    for (j=1; j <= num_pgs; j++)
        {
        put_page(pPage);
        pPage++;
        }

    mmap_read_unlock(current->mm);
    }

    return;
}

//This function programs the hardware for a buffered io.
//A chunk of data is going across the bus to the device.
void maddevc_program_stream_io(spinlock_t *splock, __iomem MADREGS *pmadregs,
		                       U32 ControlReg, U32 IntEnableReg, 
                               phys_addr_t HostAddr, U32 offset, bool bH2D)
{
	U32 CntlReg = ControlReg;
   	U32 flags1 = 0;

    PINFO("maddevc_program_stream_io... dev#=%d PA=x%llX CntlReg=x%lX IntEnable=x%lX offset=%ld wr=%d\n",
          (int)pmadregs->Devnum, (long long int)HostAddr, 
          (unsigned long)ControlReg, (unsigned long)IntEnableReg, (long int)offset, bH2D);        

    ASSERT((int)(pmadregs != NULL));

    maddev_acquire_lock_disable_ints(splock, &flags1);
    //__iomem
	maddev_program_io_regs(pmadregs, CntlReg, IntEnableReg, HostAddr);

    if ((CntlReg & MAD_CONTROL_IOSIZE_BYTES_BIT) == 0)
        {
        ASSERT((int)((long)offset >= 0));
        if (bH2D) 
            {pmadregs->ByteIndxWr = offset;}
        else
            {pmadregs->ByteIndxRd = offset;}
        }

    //Write the 'go' bit to the hardware after programming the other registers
	CntlReg |= MAD_CONTROL_BUFRD_GO_BIT;
	iowrite32(CntlReg, &pmadregs->Control);
	//
    maddev_enable_ints_release_lock(splock, &flags1);

    PINFO("maddevc_program_stream_io... dev#=%d exit\n", (int)pmadregs->Devnum);        

	return;
}

//This function resets the hardware device to a standard state after an i/o
void maddev_reset_io_registers(__iomem MADREGS *pmadregs, spinlock_t *splock)
{
	u32 Status;
    U32 IoTag;
   	U32 flags1 = 0;

    if (splock != NULL)
        {maddev_acquire_lock_disable_ints(splock, &flags1);}

    iowrite32(0, &pmadregs->MesgID);
    iowrite32(MAD_CONTROL_RESET_STATE, &pmadregs->Control);

    Status = ioread32(&pmadregs->Status);
    Status &= ~MAD_STATUS_ERROR_MASK;
    iowrite32(Status, &pmadregs->Status);

    iowrite32(0, &pmadregs->IntID);
    iowrite32(0, &pmadregs->IntEnable);

    IoTag = ioread32(&pmadregs->IoTag);
    IoTag++;
    iowrite32(IoTag, &pmadregs->IoTag);

	if (splock != NULL)
        {maddev_enable_ints_release_lock(splock, &flags1);}

    PINFO("maddev_reset_io_registers... dev#=%d ok\n", (int)pmadregs->Devnum);
	return;
}

//This function converts a hardware error to a linux-specific error code
int maddev_status_to_errno(int devnum, PMADREGS pmadregs)
{
int rc = 0;
U32 StatusReg; 

    //BUG_ON(!(virt_addr_valid(pmadregs)));
    StatusReg = (pmadregs->Status & ~MAD_STATUS_CACHE_INIT_MASK);
	if ((pmadregs->IntID & MAD_INT_STATUS_ALERT_BIT) == 0)
        return 0;

    switch(StatusReg)
        {
        case MAD_STATUS_NO_ERROR_MASK: //No Status bits set ?!?
            rc = -EBADE;               //Bad exchange - device disagrees w/ itself ?
            break;

        case MAD_STATUS_GENERAL_ERR_BIT: //The device indicates a general error
           	rc = -EIO;
           	break;

        case MAD_STATUS_OVER_UNDER_ERR_BIT: //The device indicates an overflow/underflow
            //If the cache was empty... Try again after priming the cache
           	if (pmadregs->Control & MAD_CONTROL_CACHE_XFER_BIT) 
           		{rc = -ESTRPIPE;} //Streams pipe error - indicating temporary condition
            else
           	    {rc = ((pmadregs->IntID & MAD_INT_OUTPUT_MASK) != 0) ? 
                       -EOVERFLOW : -ENODATA;}
            break;

        case MAD_STATUS_DEVICE_BUSY_BIT: //The device indicates a busy state
          	rc = -EBUSY;
           	break;

        case MAD_STATUS_DEVICE_FAILURE_BIT: //The device indicates an internal failure
           	rc = -ENOTRECOVERABLE;
           	break;

        case MAD_STATUS_INVALID_IO_BIT:
          	rc = -ENOEXEC;
            break;

        case MAD_STATUS_ERROR_MASK: //All remaining bits on after anding above
            //Assumed power failure (all bits on)  
          	ASSERT((int)(pmadregs->Status == (U32)-1));
            ASSERT((int)(pmadregs->IntID == (U32)-1));
          	rc = -ENODEV; //Well - not anymore
            break;

        default:
          	PWARN("maddev_status_to_errno: undefined device error!... dev#=%d IntID=x%X Control=x%X, Status=x%X\n",
   	              devnum, (unsigned int)pmadregs->IntID, (unsigned int)pmadregs->Control, 
   	              (unsigned int)pmadregs->Status);
   	        rc = -EIO;
        }

    if (rc != 0)
        {PWARN("maddev_status_to_errno... dev#=%d rc=%d\n", devnum, rc);}

	return rc;
}

//This function waits for the DPC to signal/post the event (wakeup the i/o queue)
//and then retrieves the status register 
U32 maddev_wait_get_io_status(PMADDEVOBJ pmdo, wait_queue_head_t* io_q,
                              U32* io_f, spinlock_t* plock)
{
    PMADDEVOBJ pmaddev = pmdo;
    PMADREGS   pIsrState  = (PMADREGS)&pmaddev->IntRegState;
    U32 iostat = 0;
    
    PINFO("maddev_wait_get_io_status() entry... dev#=%u\n", pmaddev->devnum);
    //Wait for the i/o-completion event signalled by the DPC
    wait_event(*io_q, (*io_f != eIoPending));

    //Do we have a completed i/o or an error condition ?
    if (*io_f != eIoCmplt)
        {iostat = *io_f;} //Should be negative
    else
        {
        //Process the i/o completion
        iostat = maddev_status_to_errno(pmaddev->devnum, pIsrState);
        }

    if (iostat != 0)
        PWARN("maddev_wait_get_io_status... dev#=%u iostat=%d\n",
              pmaddev->devnum, (int)iostat);
   
    return iostat;
}

//This function initializes the MAD device object
void maddev_init_io_parms(PMADDEVOBJ pmaddev, uint32_t indx)
{
    ASSERT((int)(pmaddev != NULL));
    //
    pmaddev->devnum = indx;
    pmaddev->read_f = eIoReset;
    pmaddev->write_f = eIoReset;
    pmaddev->ioctl_f = eIoReset;
    void (*dpc_fn)(unsigned long) = NULL;

    #ifdef _CDEV_
    dpc_fn = &maddevc_dpctask;
    #endif

    #ifdef _BLOCKIO_
    dpc_fn = &maddevb_dpctask;
    #endif

    tasklet_init(&pmaddev->dpctask, dpc_fn, indx);
    PINFO("maddev_init_io_parms()... dev#=%u dpctask=%px dpc_fn=%px indx=%u\n",
          pmaddev->devnum, &pmaddev->dpctask, pmaddev->dpctask.func, indx);


    init_waitqueue_head(&pmaddev->read_q);
	init_waitqueue_head(&pmaddev->write_q);
	init_waitqueue_head(&pmaddev->ioctl_q);
	//INIT_WORK(&pmaddev->dpc_work_rd, maddev_dpcwork_rd);
    //INIT_WORK(&pmaddev->dpc_work_wr, maddev_dpcwork_wr);
}

//This function returns a kernel virtual address from an imput phys addr
void* maddev_getkva(phys_addr_t PhysAddr, struct page** ppPgStr)
{
    phys_addr_t pfn = ((PhysAddr >> PAGE_SHIFT) & PFN_AND_MASK); // sign propogates Hi -> Lo  *!?!*
 
    *ppPgStr = pfn_to_page((int)pfn);
    ASSERT((int)(*ppPgStr != NULL));

#ifdef _MAD_SIMULATION_MODE_       //Assume the simulator kmalloc'd our device in RAM
                                   // so a virtual mapping exists 
    return page_address(*ppPgStr); //We compare w/ what the simulator reports for sanity 
#else 
    return kmap(*ppPgStr); //We assume our real hardware is mapped into phys-addr space
#endif
}

//This function unmaps a kernel virtual addr if necessary
void maddev_putkva(struct page* pPgStr)
{
    ASSERT((int)(pPgStr != NULL));
#ifndef _MAD_SIMULATION_MODE_ //We assume that we kmap'd our real hardware into kernel VA
    kunmap(pPgStr);
#endif
}

#ifdef _MAD_SIMULATION_MODE_ /////////////////////////////////////////
//
//This function exchanges necessary parameters w/ the simulator
extern PMAD_SIMULATOR_PARMS madbus_exchange_parms(int num);

int maddev_exchange_sim_parms(struct pci_dev** ppPcidev, PMADDEVOBJ pmaddev)
{
    PMAD_SIMULATOR_PARMS pSimParms;
    int rc = 0;

    ASSERT((int)(ppPcidev != NULL));
    ASSERT((int)(pmaddev != NULL));

    pSimParms = madbus_exchange_parms((int)pmaddev->devnum);
    if (pSimParms == NULL)
        { 
        PWARN("maddev_exchange_sim_parms... dev#=%d rc=-ENXIO\n",
              (int)pmaddev->devnum);
        return -ENXIO;
        }

    pmaddev->pSimParms = pSimParms;

    //The pci-device struct is owned by the simulator bus driver
    *ppPcidev = pSimParms->pPcidev;
    pmaddev->pPcidev = *ppPcidev;
    //
    //And now for the benefirt of the simulator ...
    pSimParms->pmaddev  = pmaddev;
    pSimParms->pmadregs = pmaddev->pDevBase;
    pSimParms->pdevlock = &pmaddev->devlock;

    PINFO("maddev_exchange_sim_parms... dev#=%d pmbobj=%px pPcidev=%px\n",
          (int)pmaddev->devnum, pSimParms->pmadbusobj, *ppPcidev);

    return rc;
}
#endif //_MAD_SIMULATION_MODE_ ///////////////////////////////////

int maddev_kobject_init(struct kobject* pkobj, struct kobject* pPrnt,
                        struct kset* kset, struct kobj_type* ktype, 
                        const char *objname)
{
    int rc = 0;

    ASSERT((int)(pkobj != NULL));
    memset(pkobj, 0x00, sizeof(struct kobject));
    //
    rc = kobject_init_and_add(pkobj, &mad_ktype, pPrnt, objname);
    //pkobj->state_initialized = 0;
    PINFO("maddev_kobject_init(_and_add)... rc=%d\n", rc);
    return rc;

    pkobj->parent = pPrnt;
    pkobj->kset   = kset;
    pkobj->ktype  = ktype;
    if (pPrnt != NULL)
        {
        BUG_ON(pPrnt->sd == NULL);
        ;
        BUG_ON(pPrnt->sd->parent == NULL);
        //pkobj->sd = 
            //kernfs_create_link(pPrnt->sd->parent, objname, pPrnt->sd);
        }
    kobject_set_name(pkobj, objname);
    //kobject_init(pkobj, ktype);
    return rc;
}

int maddev_kobject_register(struct kobject* pkobj, struct kobject* pPrnt, const char *objname)
{
    int rc; 

    memset(pkobj, 0x00, sizeof(struct kobject));
    rc = kobject_init_and_add(pkobj, &mad_ktype, pPrnt, objname);
    if (rc != 0)
        {
        kobject_put(pkobj);
        PERR("mad_kobject_register failed!... rc=%d\n", rc);
        }

    return rc;
}
//
void maddev_kobject_unregister(struct kobject* pkobj)
{
    kobject_del(pkobj);
    kobject_put(pkobj);
}
//
void maddev_kset_unregister(void)
{
    if (bMadKset)
        {maddev_kobject_unregister((struct kobject*)&mad_kset);}
}
//
int maddev_kset_create(void)
{
    int rc;

    rc = maddev_kobject_register((struct kobject*)&mad_kset, NULL, MadKsetNm);
    bMadKset = (rc == 0);

    return rc;
}
