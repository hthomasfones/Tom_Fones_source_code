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
/*  Module NAME : maddevcmain.c                                                */
/*                                                                             */
/*  DESCRIPTION : Main module for the MAD character-mode driver                */
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
/* $Id: maddevcmain.c, v 1.0 2021/01/01 00:00:00 htf $                         */
/*                                                                             */
/*******************************************************************************/

#define _DEVICE_DRIVER_MAIN_
#include "maddevc.h"		/* local definitions */

//Our parameters which can be set at load time.
int maddev_major = MADDEVOBJ_MAJOR;
module_param(maddev_major, int, S_IRUGO);
//
int maddev_minor = 0;
module_param(maddev_minor, int, S_IRUGO);
//
int   maddev_max_devs = MADBUS_NUMBER_SLOTS;	
module_param(maddev_max_devs, int, S_IRUGO);
//
int maddev_nbr_devs = MADDEV_NBR_DEVS;	/* number of bare maddev devices */
module_param(maddev_nbr_devs, int, S_IRUGO);

int mad_pci_devid = MAD_PCI_CHAR_INT_DEVICE_ID;
module_param(mad_pci_devid, int, S_IRUGO);

MODULE_AUTHOR("H. Thomas Fones");
MODULE_LICENSE("Dual BSD/GPL");

//A set of names for multiple devices
char MadDevNames[10][20] =
     {MADDEVOBJNAME, MADDEVOBJNAME, MADDEVOBJNAME, MADDEVOBJNAME, MADDEVOBJNAME, MADDEVOBJNAME, MADDEVOBJNAME, MADDEVOBJNAME};
char MadDevNumStr[] = DEVNUMSTR;

MADREGS MadRegsRst = {0, MAD_STATUS_CACHE_INIT_MASK, 0, 0x080B0B, 0, 0, 0, 0, 0, 0, 0, 0};

struct mad_dev_obj *mad_dev_objects; /* allocated in maddev_init_module */

static struct class *mad_class = NULL;
static char maddrvrname[] = DRIVER_NAME;

// The structures & entry points general to a pci module
static struct pci_device_id pci_ids[] =
{
	{ PCI_DEVICE(MAD_PCI_VENDOR_ID, MAD_PCI_CHAR_INT_DEVICE_ID), },
	{ PCI_DEVICE(MAD_PCI_VENDOR_ID, MAD_PCI_CHAR_MSI_DEVICE_ID), },
	{ 0, } 	//{ PCI_DEVICE(0, 0) }, //doesn't work
};
//
MODULE_DEVICE_TABLE(pci, pci_ids);
//
//These function prototypes are needed here but are defined in maddrvrdefs.h
int maddev_probe(struct pci_dev *pcidev, const struct pci_device_id *ids);
void maddev_shutdown(struct pci_dev *pcidev);
 void maddev_remove(struct pci_dev *pcidev);

#include "../../include/maddrvrdefs.c"

#ifdef MADDEV_DEBUG /* use proc only if debugging */
/*
 * The proc filesystem: function to read and entry
 */
int maddev_read_procmem(struct seq_file *s, void *v)
{
        int i, j;
        int limit = s->size - 80; /* Don't print more than this */

        for (i = 0; i < maddev_nbr_devs && s->count <= limit; i++)
            {
            struct mad_dev *d = &mad_dev_object[i];
            struct mad_qset *qs = d->data;
            if (down_interruptible(&d->sem))
                return -ERESTARTSYS;

            seq_printf(s,"\nDevice %i: qset %i, q %i, sz %li\n",
                      i, d->qset, d->quantum, d->size);

            for (; qs && s->count <= limit; qs = qs->next)
                { /* scan the list */
                seq_printf(s, "  item at %p, qset at %p\n",
                           qs, qs->data);
                if (qs->data && !qs->next) /* dump only the last item */
                    for (j = 0; j < d->qset; j++)
                       {
                       if (qs->data[j])
                           seq_printf(s, "    % 4i: %8p\n",
                                     j, qs->data[j]);
                       }
                }
                up(&mad_dev_object[i].sem);
        }
        return 0;
}

/*
 * Here are our sequence iteration methods.  Our "position" is
 * simply the device number.
 */
static void *maddev_seq_start(struct seq_file *s, loff_t *pos)
{
	if (*pos >= maddev_nbr_devs)
		return NULL;   /* No more to read */

	return mad_dev_object + *pos;
}

static void *maddev_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	(*pos)++;
	if (*pos >= maddev_nbr_devs)
		return NULL;

	return mad_dev_object + *pos;
}

static void maddev_seq_stop(struct seq_file *s, void *v)
{
	/* Actually, there's nothing to do here */
}

static int maddev_seq_show(struct seq_file *s, void *v)
{
	struct mad_dev *dev = (struct mad_dev *) v;
	struct mad_qset *d;
	int i;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	seq_printf(s, "\nDevice %i: qset %i, q %i, sz %li\n",
			(int) (dev - mad_dev_object), dev->qset,
			dev->quantum, dev->size);
	for (d = dev->data; d; d = d->next)
        { /* scan the list */
		seq_printf(s, "  item at %p, qset at %p\n", d, d->data);
		if (d->data && !d->next) /* dump only the last item */
			for (i = 0; i < dev->qset; i++)
                {
				if (d->data[i])
					seq_printf(s, "    % 4i: %8p\n", i, d->data[i]);
			    }
	}
	up(&dev->sem);
	return 0;
}
	
/*
 * Tie the sequence operators up.
 */
static struct seq_operations maddev_seq_ops =
{
	.start = maddev_seq_start,
	.next  = maddev_seq_next,
	.stop  = maddev_seq_stop,
	.show  = maddev_seq_show
};

/*
 * Now to implement the /proc files we need only make an open
 * method which sets up the sequence operators.
 */
static int maddevmem_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, maddev_read_procmem, NULL);
}

static int maddevseq_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &maddev_seq_ops);
}

/*
 * Create a set of file operations for our proc files.
 */
static struct file_operations maddevmem_proc_ops = {
	.owner   = THIS_MODULE,
	.open    = maddevmem_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

static struct file_operations maddevseq_proc_ops = {
	.owner   = THIS_MODULE,
	.open    = maddevseq_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release
};
	
/*
 * Actually create (and remove) the /proc file(s).
 */
static void maddev_create_proc(void)
{
	proc_create_data("maddevmem", 0 /* default mode */,
			         NULL /* parent dir */, &maddevmem_proc_ops,
			         NULL /* client data */);
	proc_create("maddevseq", 0, NULL, &maddevseq_proc_ops);
}

static void maddev_remove_proc(void)
{
	/* no problem if it was not registered */
	remove_proc_entry("maddevmem", NULL /* parent dir */);
	remove_proc_entry("maddevseq", NULL);
}
//
#endif /* MADDEV_DEBUG */

/*
 * Open and close
 */
//This is the open function for one hardware device
static int maddevc_open(struct inode *inode, struct file *fp)
{
	struct mad_dev_obj *pmaddevobj = 
                       container_of(inode->i_cdev, struct mad_dev_obj, cdev_str);

	PINFO("maddev_open... dev#=%d maddev=%p inode=%p fp=%p\n",
		  (int)pmaddevobj->devnum, pmaddevobj, inode, fp);

    mutex_lock(&pmaddevobj->devmutex);
    fp->private_data = pmaddevobj; /* for other methods */
    mutex_unlock(&pmaddevobj->devmutex);

	return 0;          /* success */
}

//This is the release function for one hardware device
static int maddevc_release(struct inode *inode, struct file *fp)
{
	struct mad_dev_obj *pmaddevobj = (struct mad_dev_obj*)fp->private_data;

	PINFO("maddev_release...dev#=%d inode=%p fp=%p\n",
          (int)pmaddevobj->devnum, inode, fp);

    mutex_lock(&pmaddevobj->devmutex);
//
    mutex_unlock(&pmaddevobj->devmutex);

	return 0;
}

/*
 * Data management: read and write
 */
//This is the generic read function
static ssize_t
maddevc_read(struct file *fp, char __user *usrbufr, size_t count, loff_t *f_pos)
{
    struct mad_dev_obj *pmaddevobj = (struct mad_dev_obj*)fp->private_data;
    ssize_t iocount = 0;                                                         

    PINFO("maddevc_read... dev#=%d fp=%px count=%ld offset_arg=%ld fppos=%ld\n",
          (int)pmaddevobj->devnum, (void *)fp,
          (long int)count, (unsigned long int)*f_pos, (long int)fp->f_pos);

    mutex_lock(&pmaddevobj->devmutex);
  
    if (count > MAD_BUFRD_IO_MAX_SIZE)
        {iocount = -EOVERFLOW;}
    else
        //Do a normal buffered read
        {iocount = maddev_read_bufrd(fp, usrbufr, count, f_pos);}

    mutex_unlock(&pmaddevobj->devmutex);

    return iocount;
}

//This is the generic write function
static ssize_t maddevc_write(struct file *fp, const char __user *usrbufr, size_t count,
                            loff_t *f_pos)
{
    PMADDEVOBJ pmaddevobj = (PMADDEVOBJ)fp->private_data;
    ssize_t iocount = 0;                                                         

    PINFO("maddev_write... dev#=%d fp=%px count=%ld f_pos=%ld fp->pos=%ld\n",
          (int)pmaddevobj->devnum, (void *)fp, (long int)count, (long int)*f_pos, (long int)fp->f_pos);

    mutex_lock(&pmaddevobj->devmutex);

    if (count > MAD_BUFRD_IO_MAX_SIZE)
        {iocount = -EOVERFLOW;}
    else
        //Do a normal buffered write 
        {iocount = maddev_write_bufrd(fp, usrbufr, count, f_pos);}

    mutex_unlock(&pmaddevobj->devmutex);

    return iocount;
}

/*
 * The ioctl() implementation
 */
long maddevc_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	static MADREGS  MadRegs;
	//
	struct mad_dev_obj *pmaddevobj = fp->private_data;
	PMADREGS        pmadregs  = (PMADREGS)pmaddevobj->pDevBase;
	PMADCTLPARMS    pCtlParms = (PMADCTLPARMS)arg;
	//
	int err = 0;
	long retval = 0;
	U32  remains = 0;
    u32 flags1 = 0;
    //U32 flags2 = 0;

	PINFO("maddevc_ioctl... dev#=%d fp=%px cmd=x%X arg=x%X\n",
		  (int)pmaddevobj->devnum, (void *)fp, cmd, (int)arg);

    /* Extract the type and number bitfields, and don't decode
	 * wrong cmds: return (inappropriate ioctl) before access_ok() */
	if ((_IOC_TYPE(cmd) != MADDEVOBJ_IOC_MAGIC)  ||
        (_IOC_NR(cmd) > MADDEVOBJ_IOCTL_MAX_NBR))
        {
		PDEBUG( "maddev_ioctl returning -EACCES\n");
		return -EACCES;
        }

	/* The direction is a bitmask, and VERIFY_WRITE catches R/W
	 * transfers. `Type' is user-oriented, while
	 * access_ok is kernel-oriented, so the concept of "read" and
	 * "write" is reversed */
    err = !access_ok(/*VERIFY_WRITE,*/ (void __user *)arg, _IOC_SIZE(cmd));
	if (err)
        {
		PDEBUG( "maddev_ioctl returning -EINVAL\n");
		return -EINVAL;
        }

    //If the ioctl queue is not free we return
    if (pmaddevobj->ioctl_f != eIoReset)
        {return -EAGAIN;}

    mutex_lock(&pmaddevobj->devmutex);

    switch(cmd)
	    {
	    case MADDEVOBJ_IOC_INIT: //Initialize the device in a standard way
	    	PDEBUG( "maddev_ioctl MADDEVOBJ_IOC_INIT\n");
            maddev_acquire_lock_disable_ints(&pmaddevobj->devlock, flags1);
            memcpy_toio(&MadRegsRst, pmadregs, sizeof(MADREGS));
            maddev_enable_ints_release_lock(&pmaddevobj->devlock, flags1);
            break;

	    case MADDEVOBJ_IOC_RESET: //Reset all index registers
	    	PDEBUG("maddev_ioctl MADDEVOBJ_IOC_RESET\n");
            maddev_acquire_lock_disable_ints(&pmaddevobj->devlock, flags1);
            iowrite32(0, &pmadregs->ByteIndxRd);
	    	iowrite32(0, &pmadregs->ByteIndxWr);
	    	iowrite32(0, &pmadregs->CacheIndxRd);
	    	iowrite32(0, &pmadregs->CacheIndxWr);
	    	iowrite32(MAD_STATUS_CACHE_INIT_MASK, &pmadregs->Status);
            maddev_enable_ints_release_lock(&pmaddevobj->devlock, flags1);
            break;

	    case MADDEVOBJ_IOC_GET_DEVICE:
		    PDEBUG("maddev_ioctl... dev#=%d; MADDEVOBJ_IOC_GET_DEVICE\n",
                   (int)pmaddevobj->devnum);
		    //
		    memset(&MadRegs, 0xcc, sizeof(MADREGS));
            maddev_acquire_lock_disable_ints(&pmaddevobj->devlock, flags1);
            memcpy_fromio(&MadRegs, pmadregs, sizeof(MADREGS));
            maddev_enable_ints_release_lock(&pmaddevobj->devlock, flags1);

            remains =
		    copy_to_user(&pCtlParms->MadRegs, &MadRegs, sizeof(MADREGS)); //possibly paged memory - copy outside of spinlock
		    if (remains > 0)
                {
                PERR("maddev_ioctl:copy_to_user...  dev#=%d bytes_remaining=%ld rc=-EFAULT\n",
                     (int)pmaddevobj->devnum, remains);
	    		retval = -EFAULT;
                }
		    break;

	    case MADDEVOBJ_IOC_SET_ENABLE:
		    MadRegs.IntEnable = arg;
            maddev_acquire_lock_disable_ints(&pmaddevobj->devlock, flags1);
            iowrite32(MadRegs.IntEnable, &pmadregs->IntEnable);
            maddev_enable_ints_release_lock(&pmaddevobj->devlock, flags1);
            break;

	    case MADDEVOBJ_IOC_SET_CONTROL:
		    MadRegs.Control = arg;
            maddev_acquire_lock_disable_ints(&pmaddevobj->devlock, flags1);
            iowrite32(MadRegs.Control, &pmadregs->Control);
            maddev_enable_ints_release_lock(&pmaddevobj->devlock, flags1);
	        break;

        case MADDEVOBJ_IOC_SET_READ_INDX:
            MadRegs.ByteIndxRd = MAD_MODULO_ADJUST(arg, MAD_UNITIO_SIZE_BYTES);
            PDEBUG("maddev_ioctl... dev#=%d read_dx=%ld\n",
                   (int)pmaddevobj->devnum, pmadregs->ByteIndxRd);
            maddev_acquire_lock_disable_ints(&pmaddevobj->devlock, flags1);
		    iowrite32(MadRegs.ByteIndxRd, &pmadregs->ByteIndxRd);
            maddev_enable_ints_release_lock(&pmaddevobj->devlock, flags1);
            PDEBUG("maddev_ioctl... dev#=%d set_rd_indx=%ld\n",
                   (int)pmaddevobj->devnum, pmadregs->ByteIndxRd);
		    break;

        case MADDEVOBJ_IOC_SET_WRITE_INDX:
		    MadRegs.ByteIndxWr = MAD_MODULO_ADJUST(arg, MAD_UNITIO_SIZE_BYTES);
            PDEBUG("maddev_ioctl... dev#=%d write_dx=%ld\n",
                   (int)pmaddevobj->devnum, pmadregs->ByteIndxWr);
            maddev_acquire_lock_disable_ints(&pmaddevobj->devlock, flags1);
		    iowrite32(MadRegs.ByteIndxWr, &pmadregs->ByteIndxWr);
            maddev_enable_ints_release_lock(&pmaddevobj->devlock, flags1);
            PDEBUG("maddev_ioctl... dev#=%d set_wr_indx=%ld\n",
                   (int)pmaddevobj->devnum, pmadregs->ByteIndxWr);
		    break;

        //Not implemented
	    case MADDEVOBJ_IOC_GET_ENABLE:
	    case MADDEVOBJ_IOC_GET_CONTROL:
        //case MADDEVOBJ_IOC_ALIGN_READ_CACHE:
        //case MADDEVOBJ_IOC_ALIGN_WRITE_CACHE:
	        retval = -ENOSYS; 
            break;

	    default:
		    PERR("maddev_ioctl... dev#=%d rc=-EINVAL\n",
                   (int)pmaddevobj->devnum);
		    retval = -EINVAL;
	    }

    mutex_unlock(&pmaddevobj->devmutex);

	return retval;
}

#if 0
static int maddev_vma_fault(struct vm_fault *vmf)
{
	struct vm_area_struct *vma = vmf->vma;
	struct mad_dev_obj *pmaddevobj = (struct mad_dev_obj *)vma->vm_private_data;
	//
	struct page *page_str = NULL;
	unsigned char *pageptr = NULL; /* default to "missing" */
	unsigned long ofset = 0;
	int retval = 0;

	PDEBUG( "maddev_vma_fault... dev#=%d, vma=%p, vmf=%p\n", (int)pmaddevobj->devnum, vma, vmf);

	down(&pmaddevobj->devsem);
	//
	ofset = (unsigned long)(vma->vm_pgoff << PAGE_SHIFT) + (vmf->address - vma->vm_start);
	pageptr = (void *)pmaddevobj->pDevBase;
	pageptr += ofset;
	page_str = virt_to_page(pageptr);
	//
	PDEBUG( "maddev_vma_fault... dev#=%d, pageptr=%p, PhysAddr=0x%0X\n",
		   (int)pmaddevobj->devnum, pageptr, (unsigned int)__pa(pageptr));

	/* got it, now increment the count */
	get_page(page_str);
	vmf->page = page_str;
	//
	up(&pmaddevobj->devsem);

	return retval;
} 
#endif
  
//This is the table of entry points for device i/o operations
static struct file_operations maddevc_fops = 
{
	.owner          = THIS_MODULE,
	.llseek         = maddev_llseek,
	.read           = maddevc_read,
	.write          = maddevc_write,
    .read_iter      = maddev_queued_read,
    .write_iter     = maddev_queued_write,
    .unlocked_ioctl = maddevc_ioctl,
    .mmap           = maddev_mmap,
    .open           = maddevc_open,
	.release        = maddevc_release,
};

#if 0
static void madcdev_default_release(struct kobject* kobj)
{
    PDEBUG("madcdev_release... \n");
}
//
static struct kobj_type ktype_madcdev_default = 
{
    .release = madcdev_default_release,
};
#endif

/*
 * Set up the char_dev structure for this device.
 */
int maddevc_setup_cdev(void* pvoid, int indx)
{
    struct mad_dev_obj* pmaddevobj = (struct mad_dev_obj*)pvoid;
	int devno = MKDEV(maddev_major, maddev_minor + indx);
	int rc = 0;
        
	PDEBUG("maddevc_setup_cdev... dev#=%d maddev_major=%d maddev_minor=%d cdev_no=x%X\n",
		   indx, maddev_major, (maddev_minor+indx), devno);

    //Initialize the cdev structure
	cdev_init(&pmaddevobj->cdev_str, &maddevc_fops);
	pmaddevobj->cdev_str.owner = THIS_MODULE;

    //Introduce the device to the kernel
	rc = cdev_add(&pmaddevobj->cdev_str, devno, 1);
	//if (rc) 	/* Fail gracefully if need be */
    //    {PERR("maddev_setup_cdev:cdev_add... dev#=%d rc=%d", indx, rc);}

    PDEBUG("maddev_setup_cdev... dev#=%d rc=%d\n", (int)pmaddevobj->devnum, rc);

    return rc;
}

// This is the driver init function
// It allocates memory and initializes all static device objects
static int maddevc_init_module(void)
{
	int    rc = 0;
    dev_t  dev = 0;
	U32    devcount = 0;
    U32    len;
    size_t SetSize = (maddev_max_devs+3) * PAGE_SIZE; //Add padding to paint over a memory error
    u8 bMSI = (mad_pci_devid == MAD_PCI_CHAR_MSI_DEVICE_ID);
 
	PINFO("maddev_init_module... mjr=%d mnr=%d\n", maddev_major, maddev_minor);

// Get a range of minor numbers to work with, asking for a dynamic
//  major number unless directed otherwise at load time. 
	if (maddev_major == 0)
        {// Get a range of minor numbers, asking for a dynamic major number 
        rc = 
        alloc_chrdev_region(&dev, maddev_minor, maddev_nbr_devs, MAD_MAJOR_OBJECT_NAME);
        if (rc == 0)
            {
            maddev_major = MAJOR(dev);
	        PINFO("maddev_init_module... allocated major number (%d)\n", maddev_major);
            }
        }
    else
	    {// Get a range of minor numbers - using the assigned major #
	    dev = MKDEV(maddev_major, maddev_minor);
		rc = 
        register_chrdev_region(maddev_major, maddev_nbr_devs, MAD_MAJOR_OBJECT_NAME);
	    }
    //
	if (rc < 0)
	    {
		PERR("maddev_init_module: can't register/alloc chrdev region... rc=%d\n", rc);
		goto InitFail;
	    }

    //Create a class for creating device nodes for hotplug devices
    //Lop off the trailing X to make a generic class name in /sys/class/<name>
    len = strlen(MadDevNames[0]);
    MadDevNames[0][len-1] = 0x00;
    mad_class = class_create(THIS_MODULE, MadDevNames[0]);
    if (IS_ERR(mad_class))
        {
        rc = PTR_ERR(mad_class);
        PERR("maddev_init_module:class_create returned %d\n", rc);
        rc = 0; //Let's continue
        }

    // Allocate memory for all device contexts - the number specified at load time
	mad_dev_objects = kzalloc(SetSize, MAD_KMALLOC_FLAGS);
	if (!mad_dev_objects)
	    {
		rc = -ENOMEM;
        PERR("maddev_init_module kalloc failed... rc=-ENOMEM\n");
		goto InitFail;  
	    }

    //* Register with the driver core through the bus driver API
    // Register as a pci device driver
    maddev_driver.driver.p->driver = &maddev_driver.driver;
    //rc = maddev_kobject_init(&maddev_driver.driver.p->kobj, NULL, 
    //                         &mad_kset, &mad_ktype, "maddrvrc_kobj");
    rc = pci_register_driver(&maddev_driver);
    if (rc != 0)
        {
        PERR("maddev_init_module:pci_register_driver error!... rc=%d\n", rc);
		goto InitFail;  
	    }

    //mad_kset_create();

    /* Create & initialize each static device. */
    PINFO("maddevc_init_module... #_static_devices=%d\n", maddev_nbr_devs);

    if (maddev_nbr_devs > 0) //Static devices
        devcount = maddev_setup_devices(maddev_nbr_devs, false, bMSI); 

#ifdef MADDEVOBJ_DEBUG /* only when debugging */
	maddev_create_proc();
#endif

    if (maddev_nbr_devs == 0) //No static devices
        {rc = 0;} 
    else //Did at least one static device init succeed
        {rc = (devcount > 0) ? 0 : -ENODEV;}
 
    return rc;

InitFail:
	maddev_cleanup_module();
	PERR( "maddev_init_module failure - returning (%d)\n", rc);

	return rc;
}

//Declare the driver init & driver exit functions
module_init(maddevc_init_module);
module_exit(maddev_cleanup_module);
