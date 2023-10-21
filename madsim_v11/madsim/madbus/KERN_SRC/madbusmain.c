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

//Our parameters which can be set at load time.
//
int   madbus_major    = MADBUSOBJ_MAJOR;
module_param(madbus_major, int, S_IRUGO);
//
int   madbus_minor    = 0;
//module_param(madbus_minor, int, S_IRUGO);
//
int madbus_nbr_slots = MADBUS_NUMBER_SLOTS;	
module_param(madbus_nbr_slots, int, S_IRUGO);
//
int   madbus_base_irq = 30;
module_param(madbus_base_irq, int, S_IRUGO);
//
//ulong maddev_size = MAD_DEVICE_MAP_MEM_SIZE;
//module_param(maddev_size, ulong, S_IRUGO);
//
char madbus_id[] = "madbus_0";

MODULE_AUTHOR("H. Thomas Fones");
MODULE_LICENSE("Dual BSD/GPL");
char *Version = "$Revision: 1.1 $";

static char MadBusObjNames[10][20] = 
    {MADBUSOBJNAME, MADBUSOBJNAME, MADBUSOBJNAME, MADBUSOBJNAME,
	 MADBUSOBJNAME, MADBUSOBJNAME, MADBUSOBJNAME, MADBUSOBJNAME};
static char MadBusNumStr[] = DEVNUMSTR;

PMADBUSOBJ madbus_objects = NULL;   
//PMADREGS MadDevs = NULL;

MADREGS MadRegsRst = {0, 0, 0, 0x00, 0, 0, 0, 0, 0, 0, 0, 0};


/*
 * Export a simple attribute.
 */
/*static ssize_t show_bus_version(struct bus_type *bus, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", Version);
}*/
//
//static BUS_ATTR(version, S_IRUGO, show_bus_version, NULL);

/*
 * MAD devices.
 */
/*
 * Open and close
 */
//This is the open function for one bus child device
//
static int madbus_dev_open(struct inode *inode, struct file *fp)
{
	struct madbus_object *mbobj = 
           container_of(inode->i_cdev, struct madbus_object, cdev_str);

	PINFO("madbus_dev_open: dev#=%d, mbobj=%px inode=%px fp=%px\n",
		  (int)mbobj->devnum, mbobj, inode, fp);

	fp->private_data = mbobj; /* for other methods */

	return 0;          /* success */
}

//This is the release function for one bus child device
//
static int madbus_dev_release(struct inode *inode, struct file *fp)
{
	struct madbus_object *mbobj = fp->private_data;

	PINFO("madbus_dev_release: dev#=%d inode=%px fp=%px\n",
          (int)mbobj->devnum, inode, fp);

	return 0;
}

//This is the ioctl function for a bus child device
//
static long madbus_dev_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	struct madbus_object *mbobj = fp->private_data;
 	PMADREGS        pMadRegs = (PMADREGS)mbobj->pmaddevice;
	PMADBUSCTLPARMS pCtlParms = (PMADBUSCTLPARMS)arg;
    U32             indx = mbobj->devnum;
	//
    U32  remains = 0;
    int err = 0;
	int retval = 0;

	PINFO("madbusobj_ioctl: dev#=%d fp=%px cmd=x%X arg=x%X\n",
		  (int)mbobj->devnum, fp, cmd, arg);

	// Extract the type and number bitfields, and don't decode
	// wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	//
	if ((_IOC_TYPE(cmd) != MADBUS_IOC_MAGIC) || (_IOC_NR(cmd) > MADBUS_IOC_MAX_NBR))
	    {
		PWARN("madbusobj_ioctl: returning -EACCES\n");
		return -EACCES;
	    }

	//The direction is a bitmask, and VERIFY_WRITE catches R/W transfers.
    // `Type' is user-oriented, while access_ok is kernel-oriented,
    //  so the concept of "read" and "write" is reversed
	//
	err = !access_ok(/*VERIFY_WRITE,*/ (void __user *)arg, _IOC_SIZE(cmd));
	if (err)
	    {
		PWARN("madbusobj_ioctl: returning -EFAULT\n");
		return -EINVAL;
        }

    if (indx == 0)
        {
        if ((cmd != MADBUS_IOC_HOT_PLUG) && (cmd != MADBUS_IOC_HOT_UNPLUG))
	        {
            PERR("madbusobj_ioctl: non-hotplug command issued to device 0\n");
            return -EINVAL;
            }
        }
    else
        {
        if ((cmd == MADBUS_IOC_HOT_PLUG) || (cmd == MADBUS_IOC_HOT_UNPLUG))
	        {
            PERR("madbusobj_ioctl: hotplug command issued to device != 0\n");
            return -EINVAL;
            }
        }

	switch(cmd)
	    {
	    case MADBUS_IOC_RESET:
		    PDEBUG( "madbusobj_ioctl: MADBUS_IOC_RESET\n");
		    memset(pMadRegs, 0x00, sizeof(MADREGS));
            pMadRegs->Devnum = mbobj->devnum;
            break;

	    case MADBUS_IOC_EXPIRE:
		    PDEBUG( "madbusobj_ioctl: MADBUS_IOC_EXPIRE\n");
		    memset(pMadRegs, 0xFF, sizeof(MADREGS));
		    break;

	    case MADBUS_IOC_GET_DEVICE:
		    PDEBUG( "madbusobj_ioctl: MADBUS_IOC_GET_DEVICE\n");
            remains =
            copy_to_user(&pCtlParms->MadRegs, pMadRegs, sizeof(MADREGS)); 
            if (remains > 0)
                {
                PERR("maddev_ioctl:copy_to_user...  dev#=%d bytes_remaining=%ld rc=-EFAULT\n",
                     (int)mbobj->devnum, remains);
                retval = -EFAULT;
                }
		    break;

	    case MADBUS_IOC_SET_MSI:
		    pMadRegs->MesgID = arg;
		    break;

	    case MADBUS_IOC_SET_STATUS:
		    pMadRegs->Status = arg;
		    break;

	    case MADBUS_IOC_SET_INTID:
		    pMadRegs->IntID = arg;
		    break;

        case MADBUS_IOC_HOT_PLUG:
            retval = madbus_hotplug(pCtlParms->Parm, //devnum (slotnum) 
                                    pCtlParms->Val); //pci_devid
            break;

        case MADBUS_IOC_HOT_UNPLUG:
            retval = madbus_hotunplug(arg);
            break;

	    default:  /* redundant, as cmd was checked against MAXNR */
            LINUX_SWITCH_DEFAULTCASE_ASSERT;
		    retval = -EINVAL;
	    }

	if (retval != 0)
        {PWARN("madbus_ioctl: returning %d\n", retval);}

	return retval;
}

//This is the open function for the virtual memory area struct
static void madbus_vma_open(struct vm_area_struct* vma)
{
	struct madbus_object *mbobj = (struct madbus_object *)vma->vm_private_data;

	PDEBUG( "madbus_vma_open... dev#=%d vma=%px\n", (int)mbobj->devnum, vma);
}

//This is the close function for the virtual memory area struct
static void madbus_vma_close(struct vm_area_struct* vma)
{
	struct madbus_object *mbobj = (struct madbus_object *)vma->vm_private_data;

	PDEBUG( "madbus_vma_close... dev#=%d vma=%px\n",  (int)mbobj->devnum, vma);
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

	PDEBUG(
		   "madbus_vma_fault... devno=%d, pmaddev=%px, pgoff=%d, vmfaddr=%px, vmstart=%px, ofset=%d\n",
		   (int)mbobj->devnum, mbobj->pmaddevice, (int)vma->vm_pgoff, vmf->address, vma->vm_start, (int)ofset);

   	if (mbobj->pmaddevice == NULL)
   		return -ENXIO;

	spin_lock(&mbobj->devlock);
	//
	pageptr = (void *)mbobj->pmaddevice;
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
//
//This table specifies the entry points for VM mapping operations
//
static struct vm_operations_struct madbus_remap_vm_ops =
{
		.open  = madbus_vma_open,
		.close = madbus_vma_close,
		//.fault = madbus_vma_fault,
};
//

//This is the function invoked when an application calls the mmap function
//on the device. It returns a virtual mode address for the memory-mapped device
//The simulator-ui uses this function to get wired to the device
//
static int madbus_dev_mmap(struct file *fp, struct vm_area_struct* vma)
{
	struct madbus_object *mbobj = fp->private_data;
	struct inode* inode_str     = fp->f_inode;
    phys_addr_t    pfn          = phys_to_pfn(mbobj->MadDevPA);
    size_t MapSize              = vma->vm_end - vma->vm_start;
    //
	int rc = 0;

	PINFO("madbus_dev_mmap... dev#=%d fp=%px pfn=x%llX PA=x%llX MapSize=%d prot=x%X\n",
          (int)mbobj->devnum, fp, pfn, mbobj->MadDevPA, 
          (int)MapSize, vma->vm_page_prot);

    rc = remap_pfn_range(vma, vma->vm_start, pfn, MapSize, vma->vm_page_prot);
    if (rc != 0)
        {
	    PERR("madbus_dev_mmap:remap_pfn_range... dev#=%d rc=%d\n",
             (int)mbobj->devnum, rc);
        return rc;
        }

	vma->vm_ops = &madbus_remap_vm_ops;
	vma->vm_flags |= VM_IO; //RESERVED;
	vma->vm_private_data = fp->private_data;

    //Increment the reference count on first use
	madbus_vma_open(vma);
    PDEBUG("madbus_dev_mmap:remap_pfn_range... dev#=%d start=%px rc=%d\n",
           (int)mbobj->devnum, vma->vm_start, rc);

    return rc;
}

/*
 * Create a set of file operations for our devices for the simulator-ui.
 */
//This is the table of entry points for device i/o operations
//
static struct file_operations madbus_dev_fileops = {
	.owner   = THIS_MODULE,
	//.read    = seq_read,
	//.llseek  = madbus_dev_lseek,
	.open    = madbus_dev_open,
	.release = madbus_dev_release,
	.unlocked_ioctl = madbus_dev_ioctl,
	.mmap    = madbus_dev_mmap,
};

U8 bBusDevRegstrd = 0;

//Free device memory   consistend with how it was malloced
static int madbus_free_device_memory(PMADBUSOBJ pmadbusobj)
{
size_t size;
int rc = 0;

	PINFO("madbus_free_device_memory... dev#=%d\n", pmadbusobj->devnum);

    if (pmadbusobj->SimParms.pInBufr != NULL)
        kfree(pmadbusobj->SimParms.pInBufr);

    if (pmadbusobj->SimParms.pOutBufr != NULL)
        kfree(pmadbusobj->SimParms.pOutBufr);

    if (pmadbusobj->pmaddevice == NULL)
        return 0;

    switch (MAD_XALLOC_PAGES_ORDER) 
        {
        case MAD_KMALLOC_BYTES_PAGE_ORDER:
            kfree(pmadbusobj->pmaddevice);
            break;

        case MAD_ALLOC_PAGES_ORDER:
            __free_pages(pmadbusobj->pPage, MAD_ALLOC_PAGES_ORDER);
            break;

        #ifdef MAD_DMA_CMA_ALLOC_PAGES_ORDER
        //We should have export_symbol(dma_free_contiguous) in the kernel
        case MAD_DMA_CMA_ALLOC_PAGES_ORDER:
            size = ((1 << MAD_DMA_CMA_ALLOC_PAGES_ORDER) * PAGE_SIZE);
            dma_free_contiguous(NULL, pmadbusobj->pPage, size);
            break;
        #endif

        default:
            rc = -ENOSYS;
            PWARN("madbus_malloc_device_mem... dev#=%d malloc not implemented! rc=%d\n",
                  (int)pmadbusobj->devnum, rc);
        }

    if (rc != 0)
        {PWARN("madbus_free_device_memory... dev#=%d rc=%d\n", 
               pmadbusobj->devnum, rc);}

    return rc;
}

// This cleanup (exit) function is used to handle initialization failures as well.
// Therefore, it must be careful to work correctly even if some of the items
// have not been initialized
//
static void madbus_exit(void)
{
	register ULONG i;
	dev_t devno = MKDEV(madbus_major, madbus_minor);
	PMADBUSOBJ pmadbusobj;
	int rc = 0;

	PINFO("madbus_exit()... devno=x%X\n", devno);

	if (madbus_objects != NULL)
	    {
    	for (i = 0; i <= madbus_nbr_slots; i++)
	        {
	    	pmadbusobj = &madbus_objects[i];
            if (pmadbusobj == NULL)
                {continue;}

            if (pmadbusobj->bRegstrd)
                {device_unregister(&pmadbusobj->sysfs_dev);}

	        if ((pmadbusobj->pThread != NULL) && 
                (!(IS_ERR(pmadbusobj->pThread))))
	    	    {
	    		rc = kthread_stop(pmadbusobj->pThread);
	    		if (rc != 0)
                    {PWARN("kthread_stop returned: (%d), dev#=%d, pThread=%px\n",
                           rc, (int)i, pmadbusobj->pThread);}
                }

		    if (pmadbusobj->pmaddevice != NULL)
                {madbus_free_device_memory(pmadbusobj);}
            }

	    kfree((void*)madbus_objects);
	    }

    if (bBusDevRegstrd)
        {device_unregister(&madbus_dev);}
	//
    driver_unregister(&madbus_drvr);
    //
    bus_remove_file(&madbus_type, &madbus_attr_ver);
    //
	bus_unregister(&madbus_type);
	//
	unregister_chrdev_region(devno, madbus_nbr_slots);

	PINFO( "madbus_exit() :)\n");
}

/*
 * Set up the char_dev structure for this device.
 */
static int madbus_dev_setup_cdev(struct madbus_object *mbobj, int indx)
{
	int rc = 0;
	int devno = MKDEV(madbus_major, madbus_minor + indx);

	PINFO("madbus_dev_setup_cdev... madbus_major=%d madbus_minor=%d devno=x%X\n",
		  madbus_major, (madbus_minor+indx), devno);

    //Initialize the cdev structure
	cdev_init(&mbobj->cdev_str, &madbus_dev_fileops);
	mbobj->cdev_str.owner = THIS_MODULE;

    //Introduce the device to the kernel
	rc = cdev_add(&mbobj->cdev_str, devno, 1);
	if (rc) /* Fail gracefully if need be */
        {PERR("Error %d adding madbus_dev%d", rc, indx);}

    return rc;
}

static int madbus_malloc_device_memory(PMADBUSOBJ pmadbusobj)
{
static U32 gfpflags = (MAD_DEVICE_KMALLOC_FLAGS | __GFP_DIRECT_RECLAIM);
//
size_t size;
int rc = 0;

    pmadbusobj->SimParms.pInBufr = kzalloc(PAGE_SIZE, MAD_KMALLOC_FLAGS);
    pmadbusobj->SimParms.pOutBufr = kzalloc(PAGE_SIZE, MAD_KMALLOC_FLAGS);
    if (pmadbusobj->SimParms.pOutBufr == NULL)
        {return -ENOMEM;}

    switch (MAD_XALLOC_PAGES_ORDER) 
        {
        case MAD_KMALLOC_BYTES_PAGE_ORDER:
            pmadbusobj->pmaddevice = 
            (PMADREGS)kmalloc(MAD_DEVICE_MAP_MEM_SIZE, MAD_DEVICE_KMALLOC_FLAGS);
            if (pmadbusobj->pmaddevice == NULL)
                {
                rc = -ENOMEM;
                PWARN("madbus_malloc_device_mem... dev#=%d kmalloc failed! rc=%d\n",
                      (int)pmadbusobj->devnum, rc);
                }
            break;

        case MAD_ALLOC_PAGES_ORDER:
            pmadbusobj->pPage = alloc_pages(__GFP_HIGHMEM, MAD_ALLOC_PAGES_ORDER);
            if (pmadbusobj->pPage != NULL)
                {pmadbusobj->pmaddevice = page_to_virt(pmadbusobj->pPage);}

            if (pmadbusobj->pmaddevice == NULL)
                {
                rc = -ENOMEM;
                PWARN("madbus_malloc_device_mem... dev#=%d alloc_pages failed! rc=%d\n",
                      (int)pmadbusobj->devnum, rc);
                }
            break;

        #ifdef MAD_DMA_CMA_ALLOC_PAGES_ORDER
        //We should have export_symbol(dma_alloc_contiguous) in the kernel
        case MAD_DMA_CMA_ALLOC_PAGES_ORDER:
            size = ((1 << MAD_DMA_CMA_ALLOC_PAGES_ORDER) * PAGE_SIZE);
            ASSERT((int)(size == MAD_DEVICE_MAP_MEM_SIZE));

            pmadbusobj->pPage = dma_alloc_contiguous(NULL, //no device necessary
                                                     size, gfpflags);
            if (pmadbusobj->pPage != NULL)
                {pmadbusobj->pmaddevice = page_to_virt(pmadbusobj->pPage);}

            if (pmadbusobj->pmaddevice == NULL)
                {
                rc = -ENOMEM;
                PWARN("madbus_malloc_device_mem... dev#=%d dma_alloc_contiguous failed! rc=%d\n",
                      (int)pmadbusobj->devnum, rc);
                }
            break;
        #endif

        default:
            rc = -ENOSYS;
            PWARN("madbus_malloc_device_mem... unknown memory allocation type! rc=%d\n",
                  rc);
        }

    if (rc == 0)
        {
        ASSERT((int)(pmadbusobj->pmaddevice != NULL));
        pmadbusobj->MadDevPA = virt_to_phys(pmadbusobj->pmaddevice);
        memset(pmadbusobj->pmaddevice, 0x00, MAD_DEVICE_MEM_SIZE_NODATA);
        memset(((u8*)pmadbusobj->pmaddevice + MAD_DEVICE_MEM_SIZE_NODATA),
               0xFF, MAD_DEVICE_DATA_SIZE);

        PINFO("madbus_setup_device... dev#=%d order=%d pPage=%px PA=x%llX kva=%px #pages=%ld size=%ld\n",
              (int)pmadbusobj->devnum, MAD_XALLOC_PAGES_ORDER, pmadbusobj->pPage, 
              pmadbusobj->MadDevPA, pmadbusobj->pmaddevice,
              MAD_DEVICE_MAX_PAGES, (MAD_DEVICE_MAX_PAGES * PAGE_SIZE));
              //{PINFO("madbus_setup_device... dev#=%d  VA=%px PA=x%llX\n",
              //       (int)i, pmadbusobj->pmaddevice, pmadbusobj->MadDevPA);}
        }

    return rc;
}

static int madbus_setup_device(PMADBUSOBJ pmadbusobj)
{
    U32 i = pmadbusobj->devnum;
    int rc  = 0;

    rc = madbus_dev_setup_cdev(pmadbusobj, i);
    if (rc != 0)
        {return rc;} 

    //Register the sysfs device
    MadBusObjNames[i][MBDEVNUMDX] = MadBusNumStr[i];
    pmadbusobj->sysfs_dev.init_name = MadBusObjNames[i];
    if (i > 0)
        {pmadbusobj->sysfs_dev.parent = &madbus_dev;}

    pmadbusobj->sysfs_dev.driver = &madbus_drvr;
    pmadbusobj->sysfs_dev.release = madbus_release;
    rc = device_register(&pmadbusobj->sysfs_dev);
    if (rc != 0)
        {PWARN("madbus_setup_device:device_register... dev#=%d rc=%d\n",
               (int)i, rc);}
    //
    pmadbusobj->bRegstrd = (rc == 0);
    //Sysfs device register failure is not fatal

    if (i > 0) //Only malloc a virtual device in ram for devnums 1..N
        {
        rc = madbus_malloc_device_memory(pmadbusobj);     
        if (rc != 0)
            {return rc;} 
        
        rc = madbus_create_thread(pmadbusobj);
        if (rc != 0)
            {return rc;} 
        }

    if (rc != 0)
        {PWARN("madbus_setup_device... dev#=%d rc=%d\n", (int)i, rc);}

    return rc;
}

static int madbus_init(void)
{
	register ulong i = 0;
	int ret = 0;
    int rc = 0;
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
    ret = 
    register_chrdev_region(madbus_major, madbus_nbr_slots,
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

    //Register our one-per-bus device - defined statically
    ret = device_register(&madbus_dev);
    if (ret != 0)
	    {
        PERR("Unable to register the madbus root device; rc=%d\n", ret);
        return ret; 
        }
    //
    bBusDevRegstrd = (ret == 0);

// allocate the devices -- we can't have them static, as the number
// can be specified at load time */
//
	SimInitPciConfig(DfltPciConfig);
    madbus_objects = 
    kzalloc(((madbus_nbr_slots+1) * sizeof(MADBUSOBJ)), MAD_KMALLOC_FLAGS);
    if (madbus_objects == NULL)
        {
	    ret = -ENOMEM;
	    PERR("madbus_init_module: can't get memory... exiting\n");
		madbus_exit();
        goto InitExit;
        }

    //memset(madbus_objects, 0x00, (madbus_nbr_slots+1) * sizeof(MADBUSOBJ));
    //
    for (i = 0; i <= madbus_nbr_slots; i++)
        {
       	pmadbusobj = &madbus_objects[i];
       	pmadbusobj->devnum = i;
        rc = madbus_setup_device(pmadbusobj);
       	if (rc != 0)
        	{
       		if (i > 1)
        		{break;} //We assume a failure condition will persist

            //If we failed the 1st device setup
            PERR("madbus_init_module: Xmalloc failed-0... exiting\n");
       		madbus_exit();
            ret = rc;
            goto InitExit;
       		}
        }

InitExit:;
	PINFO("madbus_init_module... rc=%d madbus_objects=%px #slots=%d #devices=%d\n",
          ret, madbus_objects, madbus_nbr_slots, (int)(i-1));

 	return ret;
}
//
module_init(madbus_init);
module_exit(madbus_exit);
