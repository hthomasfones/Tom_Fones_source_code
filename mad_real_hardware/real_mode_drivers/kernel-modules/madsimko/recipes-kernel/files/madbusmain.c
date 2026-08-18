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
/*  Module NAME : madbusmain.c                                                 */
/*                                                                             */
/*  DESCRIPTION : Main module for the MAD bus driver                           */
/*                                                                             */
/*  MODULE_AUTHOR("HTF Consulting");                                           */
/*  MODULE_LICENSE("Dual/GPL");                                                */
/*                                                                             */
/* The source code in this file can be freely used, adapted, and redistributed */
/* in source or binary form, so long as an acknowledgment appears in derived   */
/* source files.  The citation should state that the source code comes from    */
/* a set of source files developed by HTF Consulting                           */
/* http://www.htfconsulting.com                                                */
/*                                                                             */
/* No warranty is attached.                                                    */
/* HTF Consulting assumes no responsibility for errors or fitness of use       */
/*                                                                             */
/*                                                                             */
/* This source file was derived from source code developed by                  */
/* ALESSANDRO RUBINI and JONATHAN CORBET.                                      */
/* Copyright (C) 2001 O'REILLY & ASSOCIATES -- appearing in the book:          */
/* "LINUX DEVICE DRIVERS" by Rubini and Corbet,                                */
/* published by O'Reilly & Associates.                                         */
/* No warranty is attached to the original source code.                        */ 
/*                                                                             */
/*                                                                             */
/* $Id: madbusmain.c, v 1.0 2021/01/01 00:00:00 htf $                          */
/*                                                                             */
/*******************************************************************************/
//
#define _DEVICE_DRIVER_MAIN_
#define _SIM_DRIVER_
//
#include "madbus.h"
#include <linux/dma-map-ops.h>

//Our parameters which can be set at load time.
int   madbus_major    = MADDEV_MAJOR_BUS;
module_param(madbus_major, int, S_IRUGO);

int   madbus_minor    = 0;
//module_param(madbus_minor, int, S_IRUGO);

int madbus_nbr_slots = MADBUS_NUMBER_SLOTS;	
module_param(madbus_nbr_slots, int, S_IRUGO);
//
int   madbus_base_irq = MADBUS_BASE_IRQ;
module_param(madbus_base_irq, int, S_IRUGO);
//
MADBUS_ALLOC_ORDER geAllocOrder = MAD_PAGE_ORDER_XALLOC;

char madbus_id[] = "madbus0";
MODULE_AUTHOR("H. Thomas Fones"); 
MODULE_LICENSE("Dual BSD/GPL");
char *Version = "$Revision: 1.1 $";
MODULE_IMPORT_NS(CONFIGFS);

int gNumDevsActv = 0;

static char MadBusObjNames[10][20] = 
    {MADBUSOBJNAME, MADBUSOBJNAME, MADBUSOBJNAME, MADBUSOBJNAME,
	 MADBUSOBJNAME, MADBUSOBJNAME, MADBUSOBJNAME, MADBUSOBJNAME};
static char MadBusNumStr[] = DEVNUMSTR;

PMADBUSOBJ madbus_objects = NULL;   

MADREGS MadRegsRst = {0, 0, 0, 0x00, 0, 0, 0, 0, 0, 0, 0, 0};

U8 bBusRootAdded = 0;

/*
 * Export a simple attribute.
 */
/*static ssize_t show_bus_version(struct bus_type *bus, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", Version);
}*/
//
//static BUS_ATTR(version, S_IRUGO, show_bus_version, NULL);

void madbus_root_release(struct device *pbusroot)
{
    PINFO("madbus_root_release: pbusroot=%px of_node=%px\n",
          pbusroot, pbusroot->of_node);

    if (pbusroot->of_node != NULL)
        {
        of_reserved_mem_device_release(pbusroot);
        arch_teardown_dma_ops(pbusroot); //Just a stub if no iommu
        }
}
/*
 * MAD devices.
 */
/*
 * Open and close
 */
//This is the open function for one bus child device
//
static int madbus_fdev_open(struct inode *inode, struct file *fp)
{
	struct madbus_object *mbobj = 
           container_of(inode->i_cdev, struct madbus_object, cdev_str);

	PINFO("madbus_fdev_open: dev#=%d, mbobj=%px inode=%px fp=%px\n",
		  (int)mbobj->devnum, mbobj, inode, fp);

	fp->private_data = mbobj; /* for other methods */

	return 0;          /* success */
}

//static int madbus_fdev_close(struct inode *inode, struct file *fp)
//{
//    struct madbus_object *mbobj = fp->private_data;
//
//    return 0;
//}

//This is the release function for one bus child device
static int madbus_fdev_release(struct inode *inode, struct file *fp)
{
	struct madbus_object *mbobj = fp->private_data;

	PINFO("madbus_fdev_release... dev#=%d inode=%px fp=%px\n",
          (int)mbobj->devnum, inode, fp);
    
    fp->private_data = NULL;

	return 0;
}

//This is the ioctl function for a bus child device
static long madbus_fdev_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	struct madbus_object *mbobj = fp->private_data;
 	PMADREGS        pMadRegs = mbobj->pmadregs;
    U32             indx = mbobj->devnum;
	PMADBUSCTLPARMS pCtlParms = (PMADBUSCTLPARMS)arg;
  
	MADBUSCTLPARMS CtlParms;
    U32  remains = 0;
    int err = 0;
	int retval = 0;

	PINFO("madbus_fdev_ioctl: dev#=%d fp=%px cmd=x%X arg=x%lX\n",
		  (int)mbobj->devnum, fp, cmd, arg);

	// Extract the type and number bitfields, and don't decode
	// wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	if ((_IOC_TYPE(cmd) != MADBUS_IOCTL_MAGIC) || (_IOC_NR(cmd) > MADBUS_IOCTL_MAX_NBR))
	    {
		PWARN("madbusobj_ioctl: returning -EACCES\n");
		return -EACCES;
	    }

	//The direction is a bitmask, and VERIFY_WRITE catches R/W transfers.
    // `Type' is user-oriented, while access_ok is kernel-oriented,
    //  so the concept of "read" and "write" is reversed
	err = !access_ok(/*VERIFY_WRITE,*/ (void __user *)arg, _IOC_SIZE(cmd));
	if (err)
	    {
		PWARN("madbusobj_ioctl: returning -EFAULT\n");
		return -EINVAL;
        }

    if (indx == 0)
        {
        if ((cmd != MADBUS_IOCTL_HOT_PLUG) && (cmd != MADBUS_IOCTL_HOT_UNPLUG))
	        {
            PERR("madbusobj_ioctl: non-hotplug command issued to device 0\n");
            return -EINVAL;
            }
        }
    else
        {
        if ((cmd == MADBUS_IOCTL_HOT_PLUG) || (cmd == MADBUS_IOCTL_HOT_UNPLUG))
	        {
            PERR("madbusobj_ioctl: hotplug command issued to device != 0\n");
            return -EINVAL;
            }
        }

	switch(cmd)
	    {
	    case MADBUS_IOCTL_RESET:
		    PINFO( "madbusobj_ioctl: MADBUS_IOCTL_RESET\n");
		    memset(pMadRegs, 0x00, sizeof(MADREGS));
            pMadRegs->Devnum = mbobj->devnum;
            pMadRegs->Status = MAD_STATUS_CACHE_INIT_MASK;
            break;

	    case MADBUS_IOCTL_EXPIRE:
		    PINFO( "madbusobj_ioctl: MADBUS_IOCTL_EXPIRE\n");
		    memset(pMadRegs, 0xFF, sizeof(MADREGS));
		    break;

	    case MADBUS_IOCTL_GET_DEVICE:
		    PINFO( "madbusobj_ioctl: MADBUS_IOCTL_GET_DEVICE\n");
            remains =
            copy_to_user(&pCtlParms->MadRegs, pMadRegs, sizeof(MADREGS)); 
            if (remains > 0)
                {
                PERR("maddev_ioctl:copy_to_user... dev#=%d bytes_remaining=%d rc=-EFAULT\n",
                     (int)mbobj->devnum, (int)remains);
                retval = -EFAULT;
                }
		    break;

	    case MADBUS_IOCTL_SET_MSI:
		    pMadRegs->MesgID = arg;
		    break;

	    case MADBUS_IOCTL_SET_STATUS:
		    pMadRegs->Status = arg;
		    break;

	    case MADBUS_IOCTL_SET_INTID:
		    pMadRegs->IntID = arg;
		    break;

        case MADBUS_IOCTL_HOT_PLUG:
            PINFO( "madbusobj_ioctl: MADBUS_IOCTL_HOTPLUG\n");
            {
            remains = copy_from_user(&CtlParms, pCtlParms, sizeof(MADBUSCTLPARMS)); 
            ASSERT((int)(remains == 0)); //No remainder
            retval = madbus_hotplug(CtlParms.Parm, //devnum (slotnum) 
                                    CtlParms.Val); //pci_devid
            }
            break;

        case MADBUS_IOCTL_HOT_UNPLUG:
            PINFO( "madbusobj_ioctl: MADBUS_IOCTL_HOT_UNPLUG\n");
            retval = madbus_hotunplug(arg);
            break;

	    default:  /* redundant, as cmd was checked against MAXNR */
            LINUX_SWITCH_DEFAULTCASE_ASSERT;
		    retval = -EINVAL;
	    }

	if (retval != 0)
        {PWARN("madbus_ioctl: dev#=%d returning %d\n", (int)indx, retval);}

	return retval;
}

//This is the open function for the virtual memory area struct
static void madbus_vma_open(struct vm_area_struct* vma)
{
	struct madbus_object *mbobj = (struct madbus_object *)vma->vm_private_data;

	PINFO( "madbus_vma_open... dev#=%d vma=%px\n", (int)mbobj->devnum, vma);
}

//This is the close function for the virtual memory area struct
static void madbus_vma_close(struct vm_area_struct* vma)
{
	struct madbus_object *mbobj = (struct madbus_object *)vma->vm_private_data;

	PINFO( "madbus_vma_close... dev#=%d vma=%px\n", (int)mbobj->devnum, vma);
}
//
#if 0
static int madbus_vma_fault(struct vm_fault *vmf)
{
	struct vm_area_struct *vma = vmf->vma;
	struct madbus_object *mbobj = (struct madbus_object *)vma->vm_private_data;
	//
	struct page *page_str = NULL;
	unsigned char *pageptr = NULL; /* default to "missing" */
	unsigned long ofset =
			      (unsigned long)(vma->vm_pgoff << PAGE_SHIFT) + (vmf->address - vma->vm_start);
	int retval = 0;

	PINFO(
		   "madbus_vma_fault... devno=%d, pmaddev=%px, pgoff=%d, vmfaddr=%px, vmstart=%px, ofset=%d\n",
		   (int)mbobj->devnum, mbobj->pmadregs, (int)vma->vm_pgoff, vmf->address, vma->vm_start, (int)ofset);

   	if (mbobj->pmadregs == NULL)
   		return -ENXIO;

	spin_lock(&mbobj->devlock);
	//
	pageptr = (void *)mbobj->pmadregs;
	pageptr += ofset;
	page_str = virt_to_page(pageptr);

	/* got it, now increment the count */
	get_page(page_str);
	vmf->page = page_str;
	//
	spin_unlock(&mbobj->devlock);

	return retval;
}
#endif 

//This table specifies the entry points for VM mapping operations
static struct vm_operations_struct madbus_remap_vm_ops =
{
		.open  = madbus_vma_open,
		.close = madbus_vma_close,
		//.fault = madbus_vma_fault,
};

//This is the function invoked when an application calls the mmap function
//on the device. It returns a virtual mode address for the memory-mapped device
//The simulator-ui uses this function to get wired to the device
static int madbus_fdev_mmap(struct file *fp, struct vm_area_struct* vma)
{
	struct madbus_object *mbobj = fp->private_data;
	//struct inode* inode_str     = fp->f_inode;
    phys_addr_t    pfn          = phys_to_pfn(mbobj->MadDevPA);
    size_t MapSize              = vma->vm_end - vma->vm_start;
    //
	int rc = 0;

	PINFO("madbus_fdev_mmap... dev#=%d fp=%px pfn=x%llX PA=x%llX MapSize=%lu\n",
          (int)mbobj->devnum, (void *)fp, (unsigned long long)pfn, mbobj->MadDevPA, 
          MapSize);

    rc = remap_pfn_range(vma, vma->vm_start, pfn, MapSize, vma->vm_page_prot);
    if (rc != 0)
        {
	    PERR("madbus_fdev_mmap:remap_pfn_range... dev#=%d rc=%d\n",
             (int)mbobj->devnum, rc);
        return rc;
        }

	vma->vm_ops = &madbus_remap_vm_ops;
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(6,3,0)
        vm_flags_set(vma, VM_IO);
    #else
        vma->vm_flags |= VM_IO;
    #endif
	vma->vm_private_data = fp->private_data;

    //Increment the reference count on first use
	madbus_vma_open(vma);
    PINFO("madbus_fdev_mmap:remap_pfn_range... dev#=%d start=%px rc=%d\n",
           (int)mbobj->devnum, (void *)vma->vm_start, rc);

    return rc;
}

/*
 * Create a set of file operations for our devices for the simulator-ui.
 */
//This is the table of entry points for device i/o operations
static struct file_operations madbus_dev_fileops = {
	.owner   = THIS_MODULE,
	//.read    = seq_read,
	//.llseek  = madbus_fdev_lseek,
	.open    = madbus_fdev_open,
	.release = madbus_fdev_release,
	.unlocked_ioctl = madbus_fdev_ioctl,
	.mmap    = madbus_fdev_mmap,
};

//The bus device definition 
void madbus_obj_release(struct device *pdev)
{
    PMADBUSOBJ pmadbusobj = container_of(pdev, MADBUSOBJ, sysfs_dev);
    //ASSERT((int)(pdev != NULL));
    int devnum = (int)pmadbusobj->devnum;

	printk(KERN_DEBUG "madbus_obj_release... dev#=%d pdev=%px busobj=%px devVA=%px\n", 
           devnum, pdev, pmadbusobj, pmadbusobj->pmadregs);

   if (devnum > 0)
       {madbus_free_device_memory(pmadbusobj);}
}

//Free device memory consistent with how it was Alloc'd
int madbus_free_device_memory(PMADBUSOBJ pmadbusobj) 
{
int devnum = (int)pmadbusobj->devnum;
size_t alloc_size = pmadbusobj->alloc_size;
//size_t numpages = (alloc_size / PAGE_SIZE);
int rc = 0;

	PINFO("madbus_free_device_memory... dev#=%d order=%d size=%lu\n",
          devnum, geAllocOrder, alloc_size);

    if (pmadbusobj->SimParms.pInBufr != NULL)
        {kfree(pmadbusobj->SimParms.pInBufr);}

    if (pmadbusobj->SimParms.pOutBufr != NULL)
        {kfree(pmadbusobj->SimParms.pOutBufr);}

    switch (geAllocOrder) 
        {
        case eKmallocBytes: //MAD_PAGE_ORDER_KMALLOC_BYTES:
            if (pmadbusobj->pmadregs != NULL)
                {kfree(pmadbusobj->pmadregs);}
            break;

        case eAllocPages: //MAD_PAGE_ORDER_ALLOC_PAGES:  
            if (pmadbusobj->pPage != NULL)
                {__free_pages(pmadbusobj->pPage, geAllocOrder);}
            break;

        //We should have export_symbol(dma_free_contiguous) in the kernel
        case eCmaAlloc : //MAD_PAGE_ORDER_CMA_ALLOC:
            fallthrough;

        case eDtbSharedMem: //MAD_PAGE_ORDER_DTB_ALLOC_SHARED:
            //if (pmadbusobj->pPage != NULL)
            //    {dma_free_contiguous(&madbus_root, 
            //                         pmadbusobj->pPage, alloc_size);}
            break;

        case eDtbPrivMem: // MAD_PAGE_ORDER_DTB_ALLOC_PRIV:
            //if (pmadbusobj->pmadregs != NULL)
            //    {dma_free_coherent(&madbus_root, alloc_size,
            //                       pmadbusobj->pmadregs, pmadbusobj->hDMA);}
            break;                                      

        default:
            rc = -ENOSYS;
            PWARN("madbus.ko... dev#=%d malloc not implemented! rc=%d\n",
            (int)pmadbusobj->devnum, rc);
        }

    if (rc != 0)
        {PWARN("madbus_free_device_memory... dev#=%d rc=%d\n", devnum, rc);}

    return rc;
}

static void madbus_delete_object(PMADBUSOBJ pmadbusobj)
{
    int rc = 0;

    PINFO("madbus_delete_object... dev#=%d pmadbusobj=%px refcount=%d\n",
          (int)pmadbusobj->devnum, pmadbusobj, 
          (int)kref_read(&pmadbusobj->sysfs_dev.kobj.kref));

 	if ((pmadbusobj->pThread != NULL) && (!(IS_ERR(pmadbusobj->pThread))))
	    {
	    rc = kthread_stop(pmadbusobj->pThread);
	    if (rc != 0)
            {PWARN("kthread_stop returned: (%d) dev#=%d pThread=%px\n",
                   rc, (int)pmadbusobj->devnum, pmadbusobj->pThread);}
        }

    if (pmadbusobj->bRegstrd)
        {
        PINFO("madbus_delete_object... cdev_del(&pmadbusobj->cdev_str);\n"); 
        cdev_del(&pmadbusobj->cdev_str);
        PINFO("madbus_delete_object... device_unregister(&pmadbusobj->sysfs_dev);\n");
        device_unregister(&pmadbusobj->sysfs_dev);
        }
}

// This cleanup (exit) function is used to handle initialization failures as well.
// Therefore, it must be careful to work correctly even if some of the items
// have not been initialized
static void madbus_cleanup(void)
{
    dev_t devno = MKDEV(madbus_major, 0);

	PMADBUSOBJ pmadbusobj;
    ULONG i;
	//int rc = 0;

	PINFO("madbus_cleanup()... #objects=%d\n", gNumDevsActv);

	if (madbus_objects != NULL)
	    {
    	for (i = 0; i < gNumDevsActv; i++)
	        {
	    	pmadbusobj = &madbus_objects[i];
            ASSERT((int)(pmadbusobj->devnum == i));   
            madbus_delete_object(pmadbusobj);
            }
	    }

    if (bBusRootAdded)
        {
        device_del(&madbus_root);
        put_device(&madbus_root);
        }
	
    PINFO( "madbus.ko: driver_unregister(&madbus_drvr);\n");
    driver_unregister(&madbus_drvr);
    
    PINFO( "madbus.ko: bus_remove_file(&madbus_type, &madbus_attr_ver);\n");
    bus_remove_file(&madbus_type, &madbus_attr_ver);
    
    PINFO( "madbus.ko: bus_unregister(&madbus_type);\n");
	bus_unregister(&madbus_type);
	
	unregister_chrdev_region(devno, madbus_nbr_slots);

	PINFO( "madbus_cleanup()...  :)\n");
}

static void madbus_exit(void)
{
	dev_t devno = MKDEV(madbus_major, madbus_minor);
    
	PINFO("madbus_exit()... devno=x%X\n", devno);
    madbus_cleanup();
    PINFO("madbus_exit()... :)\n\n");
}

/*
 * Set up the char_dev structure for this device.
 */
static int madbus_dev_setup_cdev(struct madbus_object *mbobj, int indx)
{
	int rc = 0;
	int devno = MKDEV(madbus_major, madbus_minor + indx);

	PINFO("madbus_dev_setup_cdev... devnum=%d madbus_major=%d madbus_minor=%d mkdev=x%X\n",
		  (int)indx, madbus_major, (madbus_minor+indx), devno);

    //Initialize the cdev structure
	cdev_init(&mbobj->cdev_str, &madbus_dev_fileops);
	mbobj->cdev_str.owner = THIS_MODULE;

    //Introduce the device to the kernel
	rc = cdev_add(&mbobj->cdev_str, devno, 1);
	if (rc != 0) /* Fail gracefully if need be */
        {PERR("Error %d adding madbus_dev%d", rc, indx);}

    return rc;
}

static int madbus_alloc_device_memory(PMADBUSOBJ pmadbusobj)
{
static U32 gfpflags = (__GFP_DIRECT_RECLAIM);
//
size_t alloc_size = PAGE_ALIGN(MAD_DEVICE_MAP_MEM_SIZE + PAGE_SIZE);
//size_t numpages = (alloc_size / PAGE_SIZE);
int rc = 0;

    pmadbusobj->SimParms.pInBufr = kzalloc(PAGE_SIZE, MAD_KMALLOC_FLAGS);
    pmadbusobj->SimParms.pOutBufr = kzalloc(PAGE_SIZE, MAD_KMALLOC_FLAGS);
    if (pmadbusobj->SimParms.pOutBufr == NULL)
        {
        if (pmadbusobj->SimParms.pInBufr != NULL)
            {kfree(pmadbusobj->SimParms.pInBufr);}
        PERR("madbus.ko... dev#=%d kzalloc failed! rc=%d\n",
             (int)pmadbusobj->devnum, rc);
        return -ENOMEM;
        }

    pmadbusobj->MadDevPA = 0LL;
    switch (geAllocOrder) 
        {
        case eKmallocBytes: //MAD_PAGE_ORDER_KMALLOC_BYTES:
            pmadbusobj->pmadregs = 
            (PMADREGS)kmalloc(alloc_size, gfpflags);
            if (pmadbusobj->pmadregs != NULL)
                {pmadbusobj->MadDevPA = virt_to_phys(pmadbusobj->pmadregs);}
            else
                {
                rc = -ENOMEM;
                PERR("madbus.ko... dev#=%d kmalloc failed! rc=%d\n",
                     (int)pmadbusobj->devnum, rc);
                }
            break;

        case eAllocPages: //MAD_PAGE_ORDER_ALLOC_PAGES:
            pmadbusobj->pPage = alloc_pages(__GFP_HIGHMEM, eAllocPages);
            if (pmadbusobj->pPage != NULL)
                {
                pmadbusobj->pmadregs = page_to_virt(pmadbusobj->pPage);
                pmadbusobj->MadDevPA = page_to_phys(pmadbusobj->pPage); 
                ASSERT((int)(pmadbusobj->pmadregs == 
                             phys_to_virt(pmadbusobj->MadDevPA)));
                }

            if (pmadbusobj->pmadregs == NULL)
                {
                rc = -ENOMEM;
                PERR("madbus.ko... dev#=%d alloc_pages failed! rc=%d\n",
                     (int)pmadbusobj->devnum, rc);
                }
            break;

        //We need export_symbol(dma_alloc_contiguous) in the kernel
        case eCmaAlloc: //MAD_PAGE_ORDER_CMA_ALLOC:
            fallthrough;

        //We need several CONFIG_OF_... settings in kernel-menuconfig
        //and a shared region defined in DTS    
        case eDtbSharedMem: //MAD_PAGE_ORDER_DTB_ALLOC_SHARED:
            pmadbusobj->pPage = dma_alloc_contiguous(&madbus_root, 
                                                     alloc_size, gfpflags);
            if (pmadbusobj->pPage != NULL)
                {
                pmadbusobj->pmadregs = page_to_virt(pmadbusobj->pPage);
                pmadbusobj->MadDevPA = page_to_phys(pmadbusobj->pPage); 
                ASSERT((int)(pmadbusobj->pmadregs == 
                             phys_to_virt(pmadbusobj->MadDevPA)));
                }          
 
            if (pmadbusobj->pmadregs == NULL)
                {
                rc = -ENOMEM;
                PERR("madbus.ko:dma_alloc_contiguous failed!... dev#=%d rc=%d\n",
                     (int)pmadbusobj->devnum, rc);
                }
            break;
 
        //See comments above + we need export_symbol(dma_alloc_from_contiguous) in the kernel    
        case eDtbPrivMem: //MAD_PAGE_ORDER_DTB_ALLOC_PRIV:
            pmadbusobj->pmadregs = dma_alloc_coherent(&madbus_root, 
                                                      alloc_size, 
                                                      &pmadbusobj->hDMA, 
                                                      gfpflags);
            if (pmadbusobj->pmadregs == NULL)
                {
                rc = -ENOMEM;
                PERR("madbus.ko:dma_alloc_coherent failed!... dev#=%d rc=%d\n",
                     (int)pmadbusobj->devnum, rc);
                }
            else
                {
                pmadbusobj->MadDevPA = dma_to_phys(&madbus_root,
                                                   pmadbusobj->hDMA);
                ASSERT((int)(pmadbusobj->hDMA == pmadbusobj->MadDevPA));
                }
            break;

        default:
            rc = -ENOSYS;
            PERR("madbus.ko... unknown memory allocation type! rc=%d\n",rc);
        }

    if (rc != 0)
        {
        kfree(pmadbusobj->SimParms.pInBufr);
        kfree(pmadbusobj->SimParms.pOutBufr);
        PERR("madbus.ko... dev#=%d rc=%d\n", (int)pmadbusobj->devnum, rc);
        return rc;
        }

    memset(pmadbusobj->pmadregs, 0x77, MAD_DEVICE_MAP_MEM_SIZE); //The whole device
    memset(pmadbusobj->pmadregs, 0x00, MAD_DEVICE_MEM_SIZE_NODATA); //Just the registers
    pmadbusobj->alloc_size = alloc_size;
 
    PINFO("madbus_alloc_device_memory... dev#=%d order=%d \npPage=%px PA=x%llX kva=%px #pages=%ld alloc_size=%llu:x%llX\n",
          (int)pmadbusobj->devnum, geAllocOrder, pmadbusobj->pPage, 
          pmadbusobj->MadDevPA, pmadbusobj->pmadregs,
          (long)MAD_DEVICE_MAX_PAGES, (u64)alloc_size, (u64)alloc_size);

    return 0;
}

int madbus_setup_device(PMADBUSOBJ pmadbusobj, u32 indx, u8 bHP)
{
    int i= (int)indx;
    int rc = 0;
    pmadbusobj->devnum = indx;
    
    PINFO("madbus_setup_device... dev#=%d pmadbusobj=%px Hp=%d\n",
          i, pmadbusobj, bHP);

    //Register the bus-discovered device with sysfs
    MadBusObjNames[i][MBDEVNUMDX] = MadBusNumStr[i];
    //pmadbusobj->sysfs_dev.init_name = MadBusObjNames[i];
    dev_set_name(&pmadbusobj->sysfs_dev, MadBusObjNames[i]);

    // madbus_dev becomes the root of the bus object device tree
    pmadbusobj->sysfs_dev.parent = &madbus_root;
    pmadbusobj->sysfs_dev.driver = &madbus_drvr;
    pmadbusobj->sysfs_dev.release = &madbus_obj_release;
    rc = device_register(&pmadbusobj->sysfs_dev);
    PINFO("madbus_setup_device:device_register... dev#=%d rc=%d\n", i, rc);

    if (rc == 0)
        {
        rc = madbus_dev_setup_cdev(pmadbusobj, i);
        if (rc == 0)
            {pmadbusobj->bRegstrd = true;}
        else
            {
            device_unregister(&pmadbusobj->sysfs_dev);
            return rc;
            } 
        }

    if (i == 0) //Only malloc a virtual device in ram for devices 1..N
        {return 0;}

    rc = madbus_alloc_device_memory(pmadbusobj);
    if (rc == 0)
        {rc = (-1 * madbus_create_thread(pmadbusobj));}
           
   if (rc != 0)
        {
        device_unregister(&pmadbusobj->sysfs_dev);
        cdev_del(&pmadbusobj->cdev_str);
        PERR("madbus_setup_device... dev#=%d rc=%d\n", i, rc);
        }

    PINFO("madbus_setup_device... dev#=%d refcount=%d\n", 
          i, kref_read(&pmadbusobj->sysfs_dev.kobj.kref));

    return rc;
}

//Set up all statically allocated devices
int madbus_setup_devices(PMADBUSOBJ madbusobjs, int num_devices)
{
PMADBUSOBJ pmadbusobj = madbusobjs; 
int i;
int rc = 0;
int numdevsactv = 0;

    for (i = 0; i <= num_devices; i++)
        {
        rc = madbus_setup_device(pmadbusobj, i, false);
       	if (rc != 0)
        	{
       		if (i > 1)
        		{
                rc = 0;  
                break;
                } //We assume a failure condition will persist

            //If we failed the 1st device setup
            PERR("madbus_init_module:madbus_setup_devices() rc=%d\n", rc);
            return rc;
       		}

        ASSERT((int)(pmadbusobj->bRegstrd == true));    
        numdevsactv++;
        pmadbusobj++; 
        }

    PINFO("madbus_init_module:madbus_setup_devices() num_devices=%d\n",
          numdevsactv);

    return numdevsactv;
}

static int madbus_init_register_bus_device(struct device *pbusroot)
{
    int rc = 0;
    struct device_node *of_node = NULL; 

    device_initialize(pbusroot);
    pbusroot->parent    = NULL;
    pbusroot->bus       = &madbus_type,
    //pbusroot->init_name = madbus_id, 
    pbusroot->driver    = NULL;
    dev_set_name(pbusroot, madbus_id);
	pbusroot->release   = madbus_root_release, 
    pbusroot->dma_mask = &pbusroot->coherent_dma_mask;
    pbusroot->coherent_dma_mask = DMA_BIT_MASK(64);

    if (geAllocOrder <= eCmaAlloc)
        {goto dev_add;}

    // Here we set up claiming memeory reserved in the device-tree (open-firmware)    
    if (geAllocOrder == MAD_PAGE_ORDER_DTB_ALLOC_PRIV) 
        {of_node = of_find_node_by_name(NULL, "madbus-node-priv");}

    if (geAllocOrder == MAD_PAGE_ORDER_DTB_ALLOC_SHARED)
        {of_node = of_find_node_by_name(NULL, "madbus-node-shared");}

    if (of_node == NULL)
        {
        rc = -ENXIO;
        goto err_put_dev;
        }
    else        
        {
        pbusroot->of_node = of_node;
        rc = of_dma_configure(pbusroot, of_node, true);
        if (rc == 0)
            {rc = (-1 * of_reserved_mem_device_init(pbusroot));}

        if (rc != 0)
            {
            PERR("madbus:of_dma-configure-reserve-init... of_node=%px rc=%d\n", 
                 of_node, rc);
            pbusroot->of_node = NULL;
            of_node_put(of_node);
            rc = -EOPNOTSUPP;
            goto err_put_dev;
            }
        }

dev_add:;            
    //return device_register(pbusroot);
    rc = device_add(pbusroot);
	if (rc != 0)
        {
        PERR("madbus_init_register_bus_device:device_add... rc=%d\n", rc);
        if (pbusroot->of_node != NULL)
            {
            of_reserved_mem_device_release(pbusroot);
            of_node_put(pbusroot->of_node);
            }
        }

err_put_dev:;
    if (rc != 0)
        {put_device(pbusroot);} //Decrement the refcount after device_initialize(pbusroot);

    return rc;
}

static int madbus_init(void)
{
	//register ulong i = 0;
	int ret = 0;
    //int rc = 0;
    int numdevsactv = 0;
    dev_t dev = 0;
	PMADBUSOBJ pmadbusobj;
	//
	dev = MKDEV(madbus_major, madbus_minor);
	PINFO("madbus_init()... dev=x%X\n", dev);

/* Get a range of minor numbers to work with. */
    // If we need a major number allocated dynamically we use alloc_chrdev_region */
	//ret = alloc_chrdev_region(&dev, madbus_minor, madbus_nbr_devs,
	//		                  MADBUS_MAJOR_OBJECT_NAME);
	//madbus_major = MAJOR(dev);

    //But we trust our statically assigned major number for bus slot-devices
    //therefore we can use register_chrdev_region
    ret = register_chrdev_region(madbus_major, madbus_nbr_slots, 
                                 MADBUS_MAJOR_OBJECT_NAME);
	if (ret < 0)
	    {
		PERR("madbus_init_module: can't register region... mjr=%d mnr=%d rc=%d\n",
			 madbus_major, madbus_minor, ret);
		return ret;
	    }

    //Register the bus w/ the system before adding devices
	ret = bus_register(&madbus_type);
	if (ret)
	    {
		PERR("madbus_init:bus_register... rc=%d\n", ret);
		return ret;
	    }

    //Set the attribute(s) for this bus
	ret = bus_create_file(&madbus_type, &madbus_attr_ver);
	if (ret)
		{PERR("Unable to create bus attribute(s): rc=%d continuing...\n", ret);}

    //Register this driver as the base of the device tree
	ret = driver_register(&madbus_drvr);
    if (ret != 0)
        {
        PERR("Unable to register the madbus driver; rc=%d\n", ret);
        return ret;
        }

    //Register our one of bus device 
    ret = madbus_init_register_bus_device(&madbus_root);
    if (ret != 0)
	    {
        PERR("Unable to register the madbus root device; rc=%d\n", ret);
        return ret; 
        }
    //
    bBusRootAdded = (ret == 0);

// allocate the devices -- we can't have them static, as the number
// can be specified at load time */
//
	SimInitPciConfig(DfltPciConfig);
    madbus_objects = 
    kzalloc(((MADBUS_NUMBER_SLOTS+1) * sizeof(MADBUSOBJ)), MAD_KMALLOC_FLAGS);
    if (madbus_objects == NULL)
        {
	    ret = -ENOMEM;
	    PERR("madbus_init_module: can't get memory... exiting\n");
		//madbus_exit();
        goto InitExit;
        }

    pmadbusobj = madbus_objects;
    numdevsactv = madbus_setup_devices(pmadbusobj, madbus_nbr_slots);
   	if (numdevsactv >= 0)
        {gNumDevsActv = numdevsactv;}
    else    
        {
        ret = numdevsactv;
        PERR("madbus_init_module:madbus_setup_devices() rc=%d\n", ret);
 		}

InitExit:;
    if (ret != 0)
        {madbus_cleanup();}      

    PINFO("madbus_init_module... rc=%d madbus_objects=%px #slots=%d #devices=%d\n\n",
          ret, madbus_objects, madbus_nbr_slots, numdevsactv);

 	return ret;
}
//
module_init(madbus_init);
module_exit(madbus_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tom Fones");
MODULE_DESCRIPTION("Driver for supporting a PCI device simulator");