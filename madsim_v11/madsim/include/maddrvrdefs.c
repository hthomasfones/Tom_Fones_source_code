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
	.remove   = maddev_remove,
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
    PMADDEVOBJ pmaddevobj = NULL;
    u8 bMSI = 0;

    ASSERT((int)(pcidev != NULL));
    devnum = (U32)pcidev->slot;
    pmaddevobj = (PMADDEVOBJ)((U8*)mad_dev_objects + (PAGE_SIZE * devnum)); 

	PINFO("maddev_probe... slotnum=%d pmaddevobj=%px pcidev=%px vendor=x%X pci_devid=x%X\n",
          (int)devnum, pmaddevobj, pcidev, pcidev->vendor, pcidev->device);

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

    pmaddevobj->devnum = devnum;
    pmaddevobj->pPcidev = pcidev;

    // Build the device object... 
	return maddev_setup_device(pmaddevobj, &pmaddevobj->pPcidev, true, bMSI);
}
//
void maddev_shutdown(struct pci_dev *pcidev)
{
    ASSERT((int)(pcidev != NULL));

   	PINFO("maddev_shutdown... pcidev=%px\n", pcidev);

    //What else to do other than remove ?
    maddev_remove(pcidev);

    return;
}

void maddev_remove(struct pci_dev *pcidev)
{
    U32 devnum; 
    struct mad_dev_obj *pmaddevobj; 
    ASSERT((int)(pcidev != NULL));

    devnum = (U32)pcidev->slot;
    pmaddevobj = (PMADDEVOBJ)((u8*)mad_dev_objects + (PAGE_SIZE * devnum));

	PINFO("maddev_remove... devnum=%d pmaddevobj=%px pcidev=%px\n",
          (int)devnum, pmaddevobj, pcidev);

    if (pmaddevobj != NULL)
        {maddev_remove_device(pmaddevobj);}
}

static struct kset mad_kset;
//
static char MadKsetNm[] = "madkset";
//
static U8  bMadKset = 0;
//
static struct kobj_type mad_ktype = 
{
    .release = NULL,
    .sysfs_ops = NULL,
    .default_attrs = NULL,
};

//This is the open function for the virtual memory area struct
void maddev_vma_open(struct vm_area_struct* vma)
{
	struct mad_dev_obj *pmaddevobj = vma->vm_private_data;
	PDEBUG( "maddev_vma_open... dev#=%d\n", (int)pmaddevobj->devnum);
}

//This is the close function for the virtual memory area struct
void maddev_vma_close(struct vm_area_struct* vma)
{
	struct mad_dev_obj *pmaddevobj = vma->vm_private_data;
	PDEBUG( "maddev_vma_close... dev#=%d\n", (int)pmaddevobj->devnum);
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
	struct mad_dev_obj *pmaddevobj = fp->private_data;
    PMADREGS pmadregs = pmaddevobj->pDevBase; 
	//
	loff_t newpos = 0;
   	u32 flags1 = 0;
    //U32 flags2 = 0;
    int rc = 0;

	PINFO("maddev_llseek... dev#=%d lseek=%ld\n",
          (int)pmaddevobj->devnum, (ulong)off);

    mutex_lock(&pmaddevobj->devmutex);
    switch(whence)
        {
	    case 0: /* SEEK_SET */
		    newpos = off;
		    break;

	    case 1: /* SEEK_CUR */
		    newpos = fp->f_pos + off;
		    break;

	    case 2: /* SEEK_END */
		    //newpos = pmaddevobj->size + off;
		    break;

	    default: /* can't happen */
		    rc = -EINVAL;
	    }

	if (newpos < 0) 
        {rc = -EINVAL;}

    if (rc == -EINVAL)
        {
        mutex_unlock(&pmaddevobj->devmutex);
        PWARN("maddev_llseek... dev#=%d rc=-EINVAL\n",
              (int)pmaddevobj->devnum);
        return rc;
        }

    //Align to unit-io size
    newpos = (newpos / MAD_UNITIO_SIZE_BYTES);
    newpos = (newpos * MAD_UNITIO_SIZE_BYTES);

    //Update the device registers
    maddev_acquire_lock_disable_ints(&pmaddevobj->devlock, flags1);
    iowrite32(newpos, &pmadregs->ByteIndxRd);
    iowrite32(newpos, &pmadregs->ByteIndxWr);
    maddev_enable_ints_release_lock(&pmaddevobj->devlock, flags1);

    //Sets the file_pos for the next read/write 
    fp->f_pos = newpos;
    mutex_unlock(&pmaddevobj->devmutex);

    PINFO("maddev_llseek... dev#=%d location=%ld fpos=%ld\n",
          (int)pmaddevobj->devnum, (ulong)newpos, (ulong)fp->f_pos);

	return newpos;
}

//This is the function invoked when an application calls the mmap function
//on the device. It returns a virtual mode address for the memory-mapped device
int maddev_mmap(struct file *fp, struct vm_area_struct* vma)
{
	struct mad_dev_obj *pmaddevobj = fp->private_data;
	//struct inode* inode_str = fp->f_inode;
    U32    pfn              = phys_to_pfn(pmaddevobj->MadDevPA);
    size_t MapSize          = vma->vm_end - vma->vm_start;
	//
	int rc = 0;

	PINFO("maddev_mmap... dev#=%d fp=%px pfn=x%llX PA=x%llX MapSize=%ld\n",
          (int)pmaddevobj->devnum, (void *)fp, (unsigned long long)pfn, 
          pmaddevobj->MadDevPA, (long int)MapSize);

    mutex_lock(&pmaddevobj->devmutex);

    //Map/remap the Page Frame Number of the phys addr of the device into
    //user mode virtual addr. space
    rc = remap_pfn_range(vma, vma->vm_start, pfn, MapSize, vma->vm_page_prot);
    if (rc != 0)
        {
        mutex_unlock(&pmaddevobj->devmutex);
        PERR("maddev_mmap:remap_pfn_range... dev#=%d rc=%d\n",
             (int)pmaddevobj->devnum, rc);
        return rc;
        }

	vma->vm_ops = &maddev_remap_vm_ops;
	vma->vm_flags |= VM_IO; //RESERVED;
	vma->vm_private_data = fp->private_data;
    pmaddevobj->vma = vma;

    //Increment the reference count on first use
	maddev_vma_open(vma);
    mutex_unlock(&pmaddevobj->devmutex);

    PDEBUG("maddev_mmap()... dev#=%d start=%px rc=%d\n",
           (int)pmaddevobj->devnum, (void *)vma->vm_start, rc);

    return rc;
}

//This function sets up one pci device object
int maddev_setup_device(PMADDEVOBJ pmaddevobj, struct pci_dev** ppPcidevTmp, U8 bHPL, u8 bMSI)
{
    U32 i = pmaddevobj->devnum;
    dev_t devno = MKDEV(maddev_major, i);
    //
    struct pci_dev* pPcidevTmp;
    struct pci_dev* pPcidev;
    int rc = 0;
    u16 num_irqs = 0;
    U32 flags1 = 0;

	PINFO("maddev_setup_device... dev#=%d pmaddevobj=%px bHPL=%d bMSI=%d\n",
		  (int)i, pmaddevobj, bHPL, bMSI);

    mutex_init(&pmaddevobj->devmutex);
    mutex_lock(&pmaddevobj->devmutex);

    spin_lock_init(&pmaddevobj->devlock);
    //Sanity check
    maddev_acquire_lock_disable_ints(&pmaddevobj->devlock, flags1);
    maddev_enable_ints_release_lock(&pmaddevobj->devlock, flags1);

    #ifdef _MAD_SIMULATION_MODE_
    //Exchange parameters w/ the simulator
    rc = maddev_exchange_sim_parms(&pPcidevTmp, pmaddevobj);
    if (rc != 0)
		{
        mutex_unlock(&pmaddevobj->devmutex);
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
            mutex_unlock(&pmaddevobj->devmutex);
            PERR("maddev_setup_device... dev#=%d pci_get_device rc=%d\n",
                 (int)i, (int)PTR_ERR(*ppPcidevTmp));
            return (int)PTR_ERR(*ppPcidevTmp);
            }
        }
    //
    pPcidevTmp = *ppPcidevTmp;
    pPcidevTmp->msi_cap = bMSI;
    //Assign the device name for this device before acquiring resources
	MadDevNames[i][MADDEVOBJNUMDX] = MadDevNumStr[i];

    num_irqs = (bMSI) ? MAD_NUM_MSI_IRQS : 0;
    num_irqs++;
    rc = maddev_claim_pci_resrcs(pPcidevTmp, pmaddevobj,
                                 MadDevNames[i], num_irqs);
    if (rc != 0)
		{
        mutex_unlock(&pmaddevobj->devmutex);
        PERR("maddev_setup_device:maddev_claim_pci_resrcs... dev#=%d rc=%d\n",
		     (int)i, rc);
		return rc;
		}

    //If we discovered the device at startup - not hotplugged
    if (!bHPL) 
        {pmaddevobj->pPcidev = pPcidevTmp;}

    pPcidev = pmaddevobj->pPcidev;
    pPcidev->driver = &maddev_driver;
    pPcidev->msi_cap = bMSI;
	maddev_init_io_parms(pmaddevobj, i);

    //Create a device node - the equivalent to mknod in BASH
    pmaddevobj->pdevnode = device_create(mad_class, NULL, /* no parent device */ 
		                                 devno, NULL, /* no additional data */
                                         MadDevNames[i]);
    if (IS_ERR(pmaddevobj->pdevnode)) 
        {
        rc = PTR_ERR(pmaddevobj->pdevnode);
        pmaddevobj->pdevnode = NULL;
		PWARN("maddev_setup_device:device_create... dev#=%d rc=%d\n",
              (int)i, rc);
        //device_create failure may not be fatal
        }

    //Configure & register the generic device w/in pci_dev for the sysfs tree
    pPcidev->dev.init_name = MadDevNames[i];
    //pPcidev->dev.driver    = (struct device_driver*)&maddev_driver;
    pPcidev->dev.driver_data = (void *)pmaddevobj;
    rc = register_device(&pPcidev->dev
                         #ifdef _MAD_SIMULATION_MODE_ 
                         ,i  // The simulator needs the device #
                         #endif
                         );
	if (rc != 0) //ToDO: Determine if fatal or not
		{
        PWARN("maddev_setup_device:register_device... dev#=%d rc=%d\n", (int)i, (int)rc);
        }

    pmaddevobj->bDevRegstrd = (rc == 0) ? true : false;

    #ifndef _MAD_SIMULATION_MODE_
    rc = dma_set_mask(ppcidev, DMA_BIT_MASK(64));
    if (rc != 0)
        {
        maddev_release_pci_resrcs(pmaddevobj);
        mutex_unlock(&pmaddevobj->devmutex);
        PERR("maddev_setup_device:dma_set_mask... dev#=%d rc=%d\n", (int)i, rc);
        return -ENOTSUP;
        }
    #endif
    pmaddevobj->pdevnode = &pmaddevobj->pPcidev->dev;

    #ifdef _CDEV_
	    rc = maddevc_setup_cdev(pmaddevobj, i);
    #endif

    #ifdef _BLOCKIO_
        //Set up a target device for fs-open, mmap, ioctls
    	rc = maddevr_setup_cdev(pmaddevobj, i);
        rc = maddevb_create_device(pmaddevobj);
    #endif

    if (rc != 0)
		{
        maddev_release_pci_resrcs(pmaddevobj);
        mutex_unlock(&pmaddevobj->devmutex);
        PERR("maddev_setup_device:maddev_create_device... dev#=%d rc=%d\n",
			 (int)i, rc);
        return rc;
		}

    pmaddevobj->bReady = true;
    mutex_unlock(&pmaddevobj->devmutex);

    PINFO("maddev_setup_device... dev=%d pdevobj=%px devVA=%px devPA=x%llX exit :)\n",
          (int)i, (void *)pmaddevobj, 
          pmaddevobj->pDevBase, pmaddevobj->MadDevPA);

    //Release our quantum - let waiting threads run
  	schedule(); 

    return rc;
}

int maddev_setup_devices(int num_devs, U8 bHPL, u8 bMSI) 
{
    int i;
    int devcount = 0;
    int rc;
    PMADDEVOBJ  pmaddevobj;
    struct pci_dev* pPcidevTmp = NULL;

	for (i = 1; i <= num_devs; i++)
	    {
        pmaddevobj = 
        (PMADDEVOBJ)((u8*)mad_dev_objects + (PAGE_SIZE * i));
        pmaddevobj->devnum = i;
        rc = maddev_setup_device(pmaddevobj, &pPcidevTmp, false, bMSI); 
        if (rc != 0)
            {
            PWARN("maddev_init_module:maddev_setup_device... dev#=%d rc=%d\n",
                  (int)pmaddevobj->devnum, rc);
            continue;
            }

        devcount++;
	    }

    PINFO("maddev_setup_devices()... %d\n", devcount);

    return devcount;
}

//This function tears down one device object
void maddev_remove_device(PMADDEVOBJ pmaddevobj)
{
    int rc;
    int devnum = (int)pmaddevobj->devnum;

	PINFO("maddev_remove_device()... dev#=%d pmaddevobj=%px\n",
		  devnum, (void *)pmaddevobj);

    if ((devnum < 1) || (devnum > maddev_max_devs)) 
        {
        PERR("maddev_remove_device()... invalid dev#=%d\n", devnum);
        return;
        }

    pmaddevobj->bReady = false;

    #ifdef _CDEV_
	cdev_del(&pmaddevobj->cdev_str);
    #endif

    #ifdef _BLOCKIO_
    maddevb_delete_device(pmaddevobj->maddevblk_dev->pmaddevb);
    #endif

    if (pmaddevobj->bDevRegstrd)
        {
        unregister_device(&pmaddevobj->pPcidev->dev);
        pmaddevobj->bDevRegstrd = false;
        }
    else
        {PWARN("maddev_remove_device... dev#=%d not registered! (%px)\n",
              (int)pmaddevobj->devnum, (void *)pmaddevobj);}

    #if 0
    maddev_kobject_unregister(&pmaddevobj->pPcidev->dev.kobj);
    #endif
    if (pmaddevobj->pdevnode != NULL)
        {device_destroy(mad_class, MKDEV(maddev_major, pmaddevobj->devnum));}

    rc = maddev_release_pci_resrcs(pmaddevobj);

    PDEBUG("maddev_remove_device... dev#=%d exit :)\n",(int)pmaddevobj->devnum);
}

void maddev_remove_devices(int num_devices)
{
	U32 i;
    PMADDEVOBJ pmaddevobj = NULL;

	PINFO("maddev_remove_devices()... num devices=%d\n", num_devices);
    for (i = 1; i <= num_devices; i++)
        {
        pmaddevobj = (PMADDEVOBJ)((u8*)mad_dev_objects + (PAGE_SIZE * i));
        if (pmaddevobj == NULL)
            {continue;}

        if (pmaddevobj->devnum > 0) //If maddev_setup_device assigned a device#
            {
            ASSERT((int)(i == pmaddevobj->devnum));
            if (pmaddevobj->bReady == true) //if not unplugged
                {maddev_remove_device(pmaddevobj);}
            else
                {PWARN("maddev_cleanup... i=%d dev#=%d pmadobj=%px device not active!\n",
                       (int)i, (int)pmaddevobj->devnum, (void *)pmaddevobj);}
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

	pci_unregister_driver(&maddev_driver);

	/* cleanup_module is never called if registering failed */
    #ifdef _CDEV_
	unregister_chrdev_region(devno, maddev_nbr_devs);
    #endif

    #ifdef _BLOCKIO_
    unregister_blkdev(maddev_major, MAD_MAJOR_DEVICE_NAME);
    #endif

   	PINFO("maddev_cleanup_module... exit :)\n");
}   
 
   
irq_handler_t select_isr_function(int msinum)
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
             PERR("maddev_claim_pci_resrcs:select_isr_function() invalid msi#(%d)\n", msinum);
        }

    return isr_function;

}

int maddev_claim_pci_resrcs(struct pci_dev* pPcidev, PMADDEVOBJ pmaddevobj,
                            char* DevName, u16 NumIrqs)
{
    static int irqflags = IRQF_TRIGGER_HIGH | IRQF_TRIGGER_LOW;
    //
    register U32 j = 0;
    //
    U8 bMSI = pPcidev->msi_cap;
    irq_handler_t isr_function = NULL;
    int rc = 0;
    phys_addr_t BaseAddr;
    U64 Bar0end;
    U32 MapLen;
    U32 ResFlags;
    U64 PciCnfgU64 = 0;
    U32 PciCnfgLo = 0;
    U32 PciCnfgHi = 0;
    U8  PciCnfgU8;

    // Determine the device phys addr from Pci-Core *AND* PCI config space
    BaseAddr = pci_resource_start(pPcidev, 0);
    Bar0end  = pci_resource_end(pPcidev, 0);
    MapLen   = (U32)Bar0end - BaseAddr + 1;
    ResFlags = pci_resource_flags(pPcidev, 0);

    //Optional integrity checks - verfiying pci_resource... functions
    ASSERT((int)(MapLen >= MAD_SIZEOF_REGISTER_BLOCK));
    ASSERT((int)(MapLen == pci_resource_len(pPcidev, 0)));
    ASSERT((int)((ResFlags & IORESOURCE_MEM) != 0));

    rc = pci_read_config_dword(pPcidev, PCI_BASE_ADDRESS_0, &PciCnfgLo);
    rc = pci_read_config_dword(pPcidev, (PCI_BASE_ADDRESS_0+4), &PciCnfgHi);
    if (rc != 0)
        {
	    PERR("maddev_claim_pci_resrcs:pci_read_config_dword... dev#=%d rc=%d\n",
             (int)pmaddevobj->devnum, rc);
        return rc;
        }

    PciCnfgU64 = (U64)(PciCnfgHi << 32);
    PciCnfgU64 += PciCnfgLo;
    PDEBUG("pci_read_config_dword... BaseAddr=x%llX PciCnfg_Hi:Lo=x%lX:%lX x%llX\n",
           BaseAddr, PciCnfgHi, PciCnfgLo, PciCnfgU64);

    rc = pci_request_region(pPcidev, 0, DevName);
    if (rc != 0)
        {
	    PERR("maddev_claim_pci_resrcs:pci_request_region... dev=%d rc=%d\n",
             (int)pmaddevobj->devnum, rc);
        return rc;
        }

    //Get a kernel virt addr for the device
    pmaddevobj->MadDevPA = BaseAddr;
    pmaddevobj->pDevBase = phys_to_virt(pmaddevobj->MadDevPA);
	if (pmaddevobj->pDevBase == NULL)
        {
        pci_release_region(pPcidev, 0);
        return -ENOMEM;
        }

    PINFO("maddev_claim_pci_resrcs... dev#=%d PA=x%llX kva=%px\n",
		  (int)pmaddevobj->devnum, pmaddevobj->MadDevPA, pmaddevobj->pDevBase);
    pmaddevobj->pDevBase->Devnum = pmaddevobj->devnum;

    rc = pci_read_config_byte(pPcidev, PCI_INTERRUPT_LINE, &PciCnfgU8);
    if (rc != 0)
        {
	    PERR("maddev_claim_pci_resrcs:pci_read_config_dword... dev=%d rc=%d\n",
             (int)pmaddevobj->devnum, rc);
        return rc;
        }

    //Optional integrity check - verfiying one retrieve of Irq vs another
    //ASSERT((int)((int)PciCnfgU8 == pPcidev->irq));

    if (bMSI)
        {
        rc = pci_enable_msi_block(pPcidev, NumIrqs);
        if (rc != 0)
	        {
	        PERR("maddev_claim_pci_resrcs:pci_enable_msi_block... dev=%d rc=%d\n",
		         (int)pmaddevobj->devnum, rc);

            pci_release_region(pPcidev, 0);
	        return rc;
	        }
        }

    pmaddevobj->base_irq = pPcidev->irq;
    for (j=0; j < NumIrqs; j++)
        {
        isr_function = select_isr_function((int)j);
        if (isr_function != NULL)
            {rc = request_irq((pmaddevobj->base_irq + j), isr_function,
                               irqflags, DevName, pmaddevobj);}
        if (rc != 0)
            {break;} //Any request_irq failure is fatal
        }

    if (rc != 0)
        {
        PERR("maddev_claim_pci_resrcs:request_irq(%d)... dev=%d j=%d rc=%d\n",
             (int)(pPcidev->irq+j), (int)pmaddevobj->devnum, (int)j, rc);

        pci_release_region(pPcidev, 0);
        return rc;
        }

    rc = pci_enable_device(pPcidev);
	if (rc != 0)
		{
		PERR("maddev_claim_pci_resrcs:pci_enable_device... dev=%d rc=%d\n",
             (int)pmaddevobj->devnum, rc);
        maddev_release_pci_resrcs(pmaddevobj);
        }

    PDEBUG("maddev_claim_pci_resrcs... dev#=%d rc=%d\n", (int)pmaddevobj->devnum, rc);

    return rc;
}

// This function releases PCI-related resources
int maddev_release_pci_resrcs(PMADDEVOBJ pmaddevobj)
{
    register U32 j = 0;
    int rc = 0;

    PINFO("maddev_release_pci_resrcs... dev#=%d\n", (int)pmaddevobj->devnum);

    rc = free_irq(pmaddevobj->base_irq, pmaddevobj);
    if (pmaddevobj->pPcidev->msi_cap != 0)
        {
        for (j=1; j <= (MAD_NUM_MSI_IRQS); j++)
            {rc = free_irq((pmaddevobj->base_irq+j), pmaddevobj);}

        pci_disable_msi(pmaddevobj->pPcidev);
        }

    rc = pci_disable_device(pmaddevobj->pPcidev);
    pci_release_region(pmaddevobj->pPcidev, 0);

    return rc;
}

ssize_t maddev_xfer_dma_page(PMADDEVOBJ pmaddevobj, struct page* pPage,
                             sector_t sector, bool bWr)
{
    static U32 DXBC = PAGE_SIZE;
    //
    U32    DevLoclAddr = (sector * MAD_SECTOR_SIZE);
    U32    IntEnable = 
           (bWr) ? (MAD_INT_STATUS_ALERT_BIT | MAD_INT_DMA_OUTPUT_BIT) :
                   (MAD_INT_STATUS_ALERT_BIT | MAD_INT_DMA_INPUT_BIT);
    U32    DmaCntl   = MAD_DMA_CNTL_INIT;
    U32    CntlReg   = 0; 
    long   iostat;
    size_t iocount = 0;
    ssize_t lrc = -EADDRNOTAVAIL;
    u32 flags1 = 0; 
    //U32 flags2 = 0;
    dma_addr_t HostPA; 

    if (bWr)
        {DmaCntl |= MAD_DMA_CNTL_H2D;} //Write == Host2Disk

    HostPA = maddev_dma_map_page(pmaddevobj->pdevnode, pPage, PAGE_SIZE, 0, bWr);
    if (dma_mapping_error(pmaddevobj->pdevnode, HostPA))
        {
        PERR("maddev_xfer_dma_page... dev#=%d rc=%ld\n",
             (int)pmaddevobj->devnum, (long int)lrc);
        ASSERT((int)false);
        return lrc;
        }

    PINFO("maddev_xfer_dma_page... dev#=%d PA=x%llX sector=%ld wr=%d\n",
          (int)pmaddevobj->devnum, (long long int)HostPA, (long int)sector, bWr);

    pmaddevobj->ioctl_f = eIoPending;

    maddev_acquire_lock_disable_ints(&pmaddevobj->devlock, flags1);
    //
    maddev_program_io_regs(pmaddevobj->pDevBase, CntlReg, IntEnable, HostPA);

    iowrite32(DevLoclAddr, &pmaddevobj->pDevBase->DevLoclAddr);
    iowrite32(DmaCntl, &pmaddevobj->pDevBase->DmaCntl);
    iowrite32(DXBC, &pmaddevobj->pDevBase->DTBC);
    writeq(MAD_DMA_CDPP_END, &pmaddevobj->pDevBase->BCDPP);

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
        {ASSERT((int)(iocount == PAGE_SIZE));}

    maddev_dma_unmap_page(pmaddevobj->pdevnode, HostPA, PAGE_SIZE, bWr);

    PDEBUG("maddev_xfer_dma_page... dev#=%d iostat=%ld iocount=%ld\n",
           (int)pmaddevobj->devnum, iostat, iocount);
    pmaddevobj->ioctl_f = eIoReset;

    return iocount;
}

ssize_t maddev_xfer_sgdma_pages(PMADDEVOBJ pmaddevobj, 
                                long num_pgs, struct page* page_list[],
                                sector_t sector, bool bWr)
{
    //static U32 DXBC = PAGE_SIZE;
    //
    U32 DevLoclAddr = (sector * MAD_SECTOR_SIZE);
    PMAD_DMA_CHAIN_ELEMENT pSgDmaElement = &pmaddevobj->SgDmaElements[0];
    long   loop_lmt = num_pgs - 1; 
    U32    IntEnable = 
           (bWr) ? (MAD_INT_STATUS_ALERT_BIT | MAD_INT_DMA_OUTPUT_BIT) :
                   (MAD_INT_STATUS_ALERT_BIT | MAD_INT_DMA_INPUT_BIT);
    enum dma_data_direction dmadir = DMADIR(bWr);
    //
    U32    DmaCntl   = MAD_DMA_CNTL_INIT;
    U32    CntlReg   = MAD_CONTROL_CHAINED_DMA_BIT;
    U64    CDPP;
    long   iostat;
    size_t iocount = 0;

    ssize_t lrc;
    u32 sgl_num = 0;
    u32 sgelen = 0;

    u32 flags1 = 0; 
    U32 j;

    if (bWr)
        {DmaCntl |= MAD_DMA_CNTL_H2D;} //Write == Host2Disk

    PDEBUG("maddev_xfer_sgdma_pages... dev#=%d num_pgs=%ld sector=%ld pglist=%px wr=%d\n",
           (int)pmaddevobj->devnum, num_pgs, (long int)sector, page_list, bWr);

    if (num_pgs > MAD_SGDMA_MAX_PAGES)
        {
        lrc = -EINVAL;
        PERR("maddev_xfer_sgdma_pages... dev#=%d num_pgs=(%ld) > max(%ld) rc=%ld\n",
             (int)pmaddevobj->devnum, num_pgs, (long int)MAD_SGDMA_MAX_PAGES, (long int)lrc);
        ASSERT((int)false);
        return lrc;
        }
 
    sgl_num = maddev_dma_map_sgl(pmaddevobj->pdevnode, page_list, pmaddevobj->sgl, num_pgs, dmadir);
    if (sgl_num==0)
        {
        lrc = -EADDRNOTAVAIL;
        PERR("maddev_xfer_sgdma_pages... dev#=%d rc=%ld\n",
             (int)pmaddevobj->devnum, (long int)lrc);
        return lrc;
        }
 
    pmaddevobj->ioctl_f = eIoPending;

    maddev_acquire_lock_disable_ints(&pmaddevobj->devlock, flags1);
    //
    maddev_program_io_regs(pmaddevobj->pDevBase, CntlReg, IntEnable, (phys_addr_t)0);

    //Set the base of the Chained-DMA-Pkt-Pntr list
    writeq(virt_to_phys(pSgDmaElement), &pmaddevobj->pDevBase->BCDPP);

    for (j=0; j <= loop_lmt; j++)
        {
        //The set of hardware SG elements is defined as a linked-list even though
        //it is created as an array
        //Assign the phys_addr for the next sg-element in this sg-element
        CDPP = (j == loop_lmt) ? MAD_DMA_CDPP_END : 
                                 virt_to_phys(&pmaddevobj->SgDmaElements[j+1]);

        sgelen = sg_dma_len(&pmaddevobj->sgl[j]);
        maddev_program_sgdma_regs(pSgDmaElement, 
                                  sg_dma_address(&pmaddevobj->sgl[j]),
                                  DevLoclAddr, DmaCntl, sgelen, CDPP);
        iocount += sgelen; 
        DevLoclAddr += sgelen; 

        //We could use the next array element but instead we use the 
        //Chained-Dma-Pkt-Pntr because we treat the chained list as a
        //linked-list to be proper 
        //pSgDmaElement = &pmaddevobj->SgDmaElements[j];
        if (j < loop_lmt)
            {pSgDmaElement = phys_to_virt(CDPP);}
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
        {ASSERT((int)(iocount == (num_pgs * PAGE_SIZE)));}

    maddev_dma_unmap_sgl(pmaddevobj->pdevnode, pmaddevobj->sgl, num_pgs, dmadir);
    pmaddevobj->ioctl_f = eIoReset;

    PINFO("maddev_xfer_sgdma_pages... dev#=%d num_pgs=%ld iostat=%ld iocount=%ld\n",
          (int)pmaddevobj->devnum, num_pgs, (long int)iostat, (long int)iocount);

    return iocount;
}

U32 maddev_dma_map_sgl(struct device* pdev, struct page* page_list[], 
                       struct scatterlist sgl[],
                       U32 num_pgs, enum dma_data_direction dir)
{
    u32 num_sge = num_pgs;  //We don't optimize for physically contiguous pages;
    u32 j;

    maddev_sg_set_pages(num_pgs, page_list, sgl);

    #ifdef _MAD_SIMULATION_MODE_ 
        //Set the dma address to the physical ram address
        for (j=0; j < num_pgs; j++)
            {
            sgl[j].dma_address = (dma_addr_t)page_to_phys(page_list[j]); 
            BUG_ON(!(virt_addr_valid(phys_to_virt(sgl[j].dma_address))));
            }
    #else 
        //Do a true physical-to-bus-relative mapping
        num_sge = dma_map_sg(pdev, sgl, num_pgs, dir); 
        if (dma_mapping_error(pmaddevobj->pdevnode, 0))
            {
            PERR("maddev_xfer_sgdma_pages... dma_mapping_error pdev#=%px\n",
                 (int)pmaddevobj->devnum);
            ASSERT((int)num_sge==0);
            }
    #endif

    return num_sge;
}

//This function services both read & write through direct-io
//We mean direct-io between the driver and the user application.
//We will implement 'buffered' io between the host and the hardware device.
//N pages will be copied across the bus - no DMA - working with the index registers
ssize_t maddev_xfer_pages_direct(PMADDEVOBJ pmaddevobj, int num_pgs, 
                                 struct page* page_list[], U32 offset, bool bWr)
{
    PMADREGS     pmadregs = pmaddevobj->pDevBase; 
    U32          IntEnable = 
                 (bWr) ? (MAD_INT_STATUS_ALERT_BIT | MAD_INT_BUFRD_OUTPUT_BIT) :
                         (MAD_INT_STATUS_ALERT_BIT | MAD_INT_BUFRD_INPUT_BIT);
    phys_addr_t  HostPA;
    U32     CntlReg;
    U32     CountBits;
    ssize_t iocount = 0;
    long    iostat;

    ASSERT((int)(pmaddevobj->devnum==pmadregs->Devnum));
    BUG_ON(pmaddevobj->devnum!=pmadregs->Devnum);

    PINFO("maddev_xfer_pages_direct... dev#=%d num_pgs=%d offset=%ld wr=%d\n",
		  (int)pmaddevobj->devnum, num_pgs, (long int)offset, bWr);

    //Declare the specific queue to have a pending io
    if (bWr) 
        {pmaddevobj->write_f = eIoPending;}
    else
        {pmaddevobj->read_f = eIoPending;}

    //If we permitted multiple device i/o's per user i/o we would set up a loop here
    CountBits = maddev_set_count_bits((PAGE_SIZE * num_pgs), MAD_CONTROL_IO_COUNT_MASK,
                                       MAD_CONTROL_IO_COUNT_SHIFT, MAD_SECTOR_SIZE);
    //Do *NOT* set the MAD_CONTROL_IOSIZE_BYTES_BIT
    CntlReg = CountBits;
    //
    HostPA = page_to_phys(page_list[0]);
    BUG_ON(!(virt_addr_valid(phys_to_virt(HostPA))));
    maddevc_program_stream_io(&pmaddevobj->devlock, pmadregs,
                             CntlReg, IntEnable, HostPA, offset, bWr);

    //Wait for the io to complete and then process the results
    if (bWr) 
        {
        iostat = maddev_get_io_status(pmaddevobj, &pmaddevobj->write_q,
                                      &pmaddevobj->write_f,
                                      &pmaddevobj->devlock);
        if (iostat >= 0)
            {
            iocount = 
            maddev_get_io_count((U32)iostat, MAD_STATUS_WRITE_COUNT_MASK,
                                MAD_STATUS_WRITE_COUNT_SHIFT, MAD_SECTOR_SIZE);
            }
        }
    else
        {
        iostat = maddev_get_io_status(pmaddevobj, &pmaddevobj->read_q, 
                                      &pmaddevobj->read_f,
                                      &pmaddevobj->devlock);
        if (iostat >= 0)
            {
            iocount = 
            maddev_get_io_count((U32)iostat, MAD_STATUS_READ_COUNT_MASK,
                                MAD_STATUS_READ_COUNT_SHIFT, MAD_SECTOR_SIZE);
            maddev_set_dirty_pages(page_list, num_pgs);
            }
        }

    //If we have an error from the device - that's what we return
    //The specific i/o queue should already be reset in maddev_get_io_status above
    if (iostat < 0)
        {iocount = iostat;}

    //Set the specific i/o queue to ready
    if (bWr) 
        {pmaddevobj->write_f = eIoReset;}
    else
        {pmaddevobj->read_f = eIoReset;}

    PDEBUG("maddev_xfer_pages_direct... dev#=%d num_pgs=%d iocount=%ld\n",
           (int)pmaddevobj->devnum, num_pgs, (long int)iocount);

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
    PDEBUG("maddev_need_sg... num_pgs=%ld PA0=x%llX PAx=x%llX\n",
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
    static long GUP_FLAGS = 
           (FOLL_TOUCH|FOLL_SPLIT|FOLL_GET|FOLL_FORCE|FOLL_POPULATE|FOLL_MLOCK);
    //
    //long gup_flags = (bUpdate) ? (GUP_FLAGS | FOLL_WRITE) : GUP_FLAGS;
    long gup_flags = (GUP_FLAGS | FOLL_WRITE); //either i/o direction
    long num_pgs;
    //int x=0; 

    down_read(&current->mm->mmap_sem);
    num_pgs = get_user_pages((u64)usrbufr, nr_pages, gup_flags, pPages, NULL);
    up_read(&current->mm->mmap_sem);

    #if 0
    PDEBUG("maddev_get_user_pages... num_pgs:x=%d r=%d pPgs=%px pPS0=%px pPS1=%px pPS2=%px\n",
           nr_pages, num_pgs, pPages, pPages[0], pPages[1], pPages[2]);
    while (pPages[x] != NULL)
        {
        PDEBUG("maddev_get_user_pages... pPSx=%px PAx=x%llX flagsx=x%X\n",
               pPages[x], page_to_phys(pPages[x]), pPages[x]->flags);
        x++;
        }
    #endif

    return num_pgs;
}

void maddev_put_user_pages(struct page** ppPages, u32 num_pgs)
{
    //if 1
    struct page* pPage = *ppPages; 
    u32 j; 
    PDEBUG("maddev_put_user_pages... num_pgs=%ld pPages=%px pPS0=%px PA0=x%llX\n",
           (long int)num_pgs, ppPages, pPage, virt_to_phys(pPage));
    //#endif

    {
    down_read(&current->mm->mmap_sem);
    //put_user_pages(ppPages, num_pgs);
    //struct page* pPage = *ppPages;
    for (j=1; j <= num_pgs; j++)
        {
        put_page(pPage);
        pPage++;
        }

    up_read(&current->mm->mmap_sem);
    }

    return;
}

//This function programs the hardware for a buffered io.
//A chunk of data is going across the bus to the device.
void maddevc_program_stream_io(spinlock_t *splock, PMADREGS pmadregs,
		                      U32 ControlReg, U32 IntEnableReg, 
                              phys_addr_t HostAddr, U32 offset, bool bWr)
{
	U32 CntlReg = ControlReg;
   	u32 flags1 = 0;
    //U32 flags2 = 0;

    PINFO("maddevc_program_stream_io... dev#=%d PA=x%llX CntlReg=x%lX IntEnable=x%lX offset=%ld wr=%d\n",
          (int)pmadregs->Devnum, (long long int)HostAddr, 
          (unsigned long)ControlReg, (unsigned long)IntEnableReg, (long int)offset, bWr);        

    ASSERT((int)(pmadregs != NULL));

    maddev_acquire_lock_disable_ints(splock, flags1);
    //
	maddev_program_io_regs(pmadregs, CntlReg, IntEnableReg, HostAddr);

    if ((CntlReg & MAD_CONTROL_IOSIZE_BYTES_BIT) == 0)
        {
        ASSERT((int)((long)offset >= 0));
        if (bWr) 
            {pmadregs->ByteIndxWr = offset;}
        else
            {pmadregs->ByteIndxRd = offset;}
        }

    //Write the 'go' bit to the hardware after programming the other registers
	CntlReg |= MAD_CONTROL_BUFRD_GO_BIT;
	iowrite32(CntlReg, &pmadregs->Control);
	//
    maddev_enable_ints_release_lock(splock, flags1);

    PINFO("maddevc_program_stream_io... dev#=%d exit\n", (int)pmadregs->Devnum);        

	return;
}

//This function resets the hardware device to a standard state after an i/o
void maddev_reset_io_registers(PMADREGS pmadregs, spinlock_t *splock)
{
	u32 Status;
   	u32 flags1 = 0;
    //U32 flags2 = 0;
    U32 IoTag;

    BUG_ON(!(virt_addr_valid(pmadregs)));

    if (splock != NULL)
        {maddev_acquire_lock_disable_ints(splock, flags1);}

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
        {maddev_enable_ints_release_lock(splock, flags1);}

    PDEBUG("maddev_reset_io_registers... dev#=%d ok\n", (int)pmadregs->Devnum);
	return;
}

//This function converts a hardware error to a linux-specific error code
int maddev_status_to_errno(int devnum, PMADREGS pmadregs)
{
int rc = 0;
U32 StatusReg; 

    BUG_ON(!(virt_addr_valid(pmadregs)));

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
           	rc = -ENODATA;
           	if (pmadregs->IntID & MAD_INT_OUTPUT_MASK)
           		rc = -EOVERFLOW;

            //If the cache was empty... Try again after priming the cache
           	if (pmadregs->Control & MAD_CONTROL_CACHE_XFER_BIT) 
           		rc = -EAGAIN;                                   
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
        PWARN("maddev_status_to_errno... dev#=%d rc=%d\n", devnum, rc);

	return rc;
}

//This function waits for the DPC to signal/post the event (wakeup the i/o queue)
//and then retrieves the status register 
long maddev_get_io_status(PMADDEVOBJ pmdo, wait_queue_head_t* io_q,
                          long* io_f, spinlock_t* plock)
{
    PMADDEVOBJ pmaddevobj = pmdo;
    PMADREGS   pIsrState  = &pmaddevobj->IntRegState;
    long iostat = 0;

    //Wait for the i/o-completion event signalled by the DPC
    wait_event(*io_q, (*io_f != eIoPending));

    //Do we have a completed i/o or an error condition ?
    if (*io_f != eIoCmplt)
        {iostat = *io_f;} //Should be negative
    else
        {
        //Process the i/o completion
        iostat = maddev_status_to_errno(pmaddevobj->devnum, pIsrState);

        //If we are working with user pages we must do the simulation
        //buffer copying in the user thread context ...
        //even though it worked fine inside the simulator for the 
        //simple case of buffered i/o (copyfromuser, copytouser)
        //These function(s) are found in the simulator mbdevthread.C 
        #ifdef _MAD_SIMULATION_MODE_
        pmaddevobj->pSimParms->pcomplete_simulated_io(pmaddevobj->pSimParms->pmadbusobj,
                                                      pIsrState);
        #endif
        }

    PDEBUG("maddev_get_io_status... dev#=%ld iostat=x%lX\n",
           pmaddevobj->devnum, iostat);
    return iostat;
}

//This function initializes the MAD device object
void maddev_init_io_parms(PMADDEVOBJ pmaddevobj, U32 indx)
{
    ASSERT((int)(pmaddevobj != NULL));
    //
    pmaddevobj->devnum = indx;
    pmaddevobj->read_f = eIoReset;
    pmaddevobj->write_f = eIoReset;
    pmaddevobj->ioctl_f = eIoReset;

    #ifdef _BLOCKIO_
	tasklet_init(&pmaddevobj->dpctask, maddevb_dpctask, indx);
    #endif

    #ifdef _CDEV_
	tasklet_init(&pmaddevobj->dpctask, maddevc_dpctask, indx);
    #endif

    init_waitqueue_head(&pmaddevobj->read_q);
	init_waitqueue_head(&pmaddevobj->write_q);
	init_waitqueue_head(&pmaddevobj->ioctl_q);
	//INIT_WORK(&pmaddevobj->dpc_work_rd, maddev_dpcwork_rd);
    //INIT_WORK(&pmaddevobj->dpc_work_wr, maddev_dpcwork_wr);
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

int maddev_exchange_sim_parms(struct pci_dev** ppPcidev, PMADDEVOBJ pmaddevobj)
{
    PMAD_SIMULATOR_PARMS pSimParms;
    int rc = 0;

    ASSERT((int)(ppPcidev != NULL));
    ASSERT((int)(pmaddevobj != NULL));

    pSimParms = madbus_exchange_parms((int)pmaddevobj->devnum);
    if (pSimParms == NULL)
        { 
        PWARN("maddev_exchange_sim_parms... dev#=%d rc=-ENXIO\n",
              (int)pmaddevobj->devnum);
        return -ENXIO;
        }

    pmaddevobj->pSimParms = pSimParms;

    //The pci-device struct is owned by the simulator bus driver
    *ppPcidev = pSimParms->pPcidev;
    pmaddevobj->pPcidev = *ppPcidev;
    //
    pSimParms->pmaddevobj = pmaddevobj;
    pSimParms->pdevlock   = &pmaddevobj->devlock;

    PINFO("maddev_exchange_sim_parms... dev#=%d pmbobj=%px pPcidev=%px\n",
          (int)pmaddevobj->devnum, pSimParms->pmadbusobj, *ppPcidev);

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
    PDEBUG("maddev_kobject_init(_and_add)... rc=%d\n", rc);
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

    //rc = kobject_set_name(pkobj, objname);
    //if (rc == 0)
    //{rc = kobject_register(pkobj);}
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
    //kobject_unregister(pkobj);
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
int maddev_kset_create()
{
    int rc;

    rc = maddev_kobject_register((struct kobject*)&mad_kset, NULL, MadKsetNm);
    bMadKset = (rc == 0);

    return rc;
}

