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
/*  Module NAME : maddevbmain.c                                                */
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
/* $Id: maddevbmain.c, v 1.0 2021/01/01 00:00:00 htf $                         */
/*                                                                             */
/*******************************************************************************/

#define _DEVICE_DRIVER_MAIN_
#include "maddevb.h"		/* local definitions */
 
//Our parameters which can be set at load time.
int maddev_major = MADDEV_MAJOR; //MADDEVOBJ_MAJOR;
module_param(maddev_major, int, S_IRUGO);
int maddevb_major = MADDEV_MAJOR; //LINUX_MAJOR_DEV_FLOPPY_DISK;
//
int maddev_minor = 0;
module_param(maddev_minor, int, S_IRUGO);
//
int maddev_max_devs = MADBUS_NUMBER_SLOTS;	
module_param(maddev_max_devs, int, S_IRUGO);
//
int maddev_nbr_devs = MADDEV_NBR_DEVS;	/* number of bare maddev devices */
module_param(maddev_nbr_devs, int, S_IRUGO);
MODULE_PARM_DESC(maddev_nbr_devs, "Number of devices to register");

int mad_pci_devid = MAD_PCI_BLOCK_INT_DEVICE_ID;
module_param(mad_pci_devid, int, S_IRUGO);

int g_no_sched;
module_param_named(no_sched, g_no_sched, int, 0444);
MODULE_PARM_DESC(no_sched, "No io scheduler");

int g_submit_queues = MADDEVB_NUM_SUBMIT_QUEUES; //1;
module_param_named(submit_queues, g_submit_queues, int, 0444);
MODULE_PARM_DESC(submit_queues, "Number of submission queues");

int g_home_node = NUMA_NO_NODE;
module_param_named(home_node, g_home_node, int, 0444);
MODULE_PARM_DESC(home_node, "Home node for the device");

//int g_gb = 250
int g_gs = MAD_DEVICE_DATA_SECTORS; 
//module_param_named(gs, g_gs, int, 0444);
//MODULE_PARM_DESC(gb, "Size in GB");
//MODULE_PARM_DESC(gb, "Size in sectore");

int g_bs = MAD_SECTOR_SIZE; 
//module_param_named(bs, g_bs, int, 0444);
//MODULE_PARM_DESC(bs, "Block size (in bytes)");

bool g_blocking = false;
module_param_named(blocking, g_blocking, bool, 0444);
MODULE_PARM_DESC(blocking, "Register as a blocking blk-mq driver device");

bool shared_tags;
module_param(shared_tags, bool, 0444);
MODULE_PARM_DESC(shared_tags, "Share tag set between devices for blk-mq");

int g_irqmode = NULL_IRQ_SOFTIRQ;
//
unsigned long g_completion_nsec = MADDEVB_IO_TIMEOUT_NANO_SECS; //10000;
module_param_named(completion_nsec, g_completion_nsec, ulong, 0444);
MODULE_PARM_DESC(completion_nsec, "Time in ns to complete a request in hardware. Default: 10,000ns");

int g_hw_queue_depth = 1; //64;
//module_param_named(hw_queue_depth, g_hw_queue_depth, int, 0444);
//MODULE_PARM_DESC(hw_queue_depth, "Queue depth for each hardware queue. Default: 64");
int g_queue_mode = MADDEVB_Q_RQ; //MADDEVB_Q_MQ;

bool g_use_per_node_hctx;
module_param_named(use_per_node_hctx, g_use_per_node_hctx, bool, 0444);
MODULE_PARM_DESC(use_per_node_hctx, "Use per-node allocation for hardware context queues. Default: false");

bool g_zoned = false;
//
#ifdef MADDEVB_ZONED
module_param_named(zoned, g_zoned, bool, S_IRUGO);
MODULE_PARM_DESC(zoned, "Make device as a host-managed zoned block device. Default: false");
//
unsigned long g_zone_size = 256;
module_param_named(zone_size, g_zone_size, ulong, S_IRUGO);
MODULE_PARM_DESC(zone_size, "Zone size in MB when block device is zoned. Must be power-of-two: Default: 256");
//
unsigned int g_zone_nr_conv;
module_param_named(zone_nr_conv, g_zone_nr_conv, uint, 0444);
MODULE_PARM_DESC(zone_nr_conv, "Number of conventional zones when block device is zoned. Default: 0");
#endif

MODULE_AUTHOR("H. Thomas Fones");
MODULE_LICENSE("Dual BSD/GPL");

//A set of names for multiple devices
char MadDevNames[10][20] =
     {MADDEVOBJNAME, MADDEVOBJNAME, MADDEVOBJNAME, MADDEVOBJNAME, MADDEVOBJNAME, MADDEVOBJNAME, MADDEVOBJNAME, MADDEVOBJNAME};
char MadDevNumStr[] = DEVNUMSTR;

MADREGS MadRegsRst = {0, MAD_STATUS_CACHE_INIT_MASK, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

struct mad_dev_obj *mad_dev_objects; /* allocated in maddev_init_module */

static struct class *mad_class = NULL;
static char maddrvrname[] = DRIVER_NAME;

//* The structures & entry points general to a pci module
static struct pci_device_id pci_ids[] =
{
	{ PCI_DEVICE(MAD_PCI_VENDOR_ID, MAD_PCI_BLOCK_INT_DEVICE_ID), },
    { PCI_DEVICE(MAD_PCI_VENDOR_ID, MAD_PCI_BLOCK_MSI_DEVICE_ID), },
	{ 0, } 	//{ PCI_DEVICE(0, 0) }, //doesn't work
};
//
MODULE_DEVICE_TABLE(pci, pci_ids);

//static LIST_HEAD(maddevb_list);
struct mutex lock;
DEFINE_IDA(maddevb_indexes);

//These function prototypes are needed here but are defined in maddrvrdefs.h
int maddev_probe(struct pci_dev *pcidev, const struct pci_device_id *ids);
void maddev_shutdown(struct pci_dev *pcidev);
void maddev_remove(struct pci_dev *pcidev);
//
struct mad_dev_obj; // *PMADDEVOBJ;

#include "../../include/maddrvrdefs.c"

#ifdef MADDEV_DEBUG /* use proc only if debugging */
/*
 * The proc filesystem: function to read and entry
 */
int maddev_read_procmem(struct seq_file *s, void *v)
{
        int i, j;
        int limit = s->size - 80; /* Don't print more than this */

        for (i = 0; i < maddev_nbr_devs && s->count <= limit; i++) {
                struct mad_dev *d = &mad_dev_object[i];
                struct mad_qset *qs = d->data;
                if (down_interruptible(&d->sem))
                        return -ERESTARTSYS;
                seq_printf(s,"\nDevice %i: qset %i, q %i, sz %li\n",
                             i, d->qset, d->quantum, d->size);
                for (; qs && s->count <= limit; qs = qs->next) { /* scan the list */
                        seq_printf(s, "  item at %px, qset at %px\n",
                                     qs, qs->data);
                        if (qs->data && !qs->next) /* dump only the last item */
                                for (j = 0; j < d->qset; j++) {
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
		seq_printf(s, "  item at %px, qset at %px\n", d, d->data);
		if (d->data && !d->next) /* dump only the last item */
			for (i = 0; i < dev->qset; i++)
                {
				if (d->data[i])
					seq_printf(s, "    % 4i: %8p\n",
							i, d->data[i]);
			    }
	    }

	up(&dev->sem);
	return 0;
}
	
/*
 * Tie the sequence operators up.
 */
static struct seq_operations maddev_seq_ops = {
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
static int maddevr_open(struct inode *inode, struct file *fp)
{
	struct mad_dev_obj *pmaddevobj = 
                       container_of(inode->i_cdev, struct mad_dev_obj, cdev_str);

	PINFO("maddevr_open... dev#=%d maddev=%p inode=%p fp=%p\n",
		  (int)pmaddevobj->devnum, pmaddevobj, inode, fp);

    mutex_lock(&pmaddevobj->devmutex);
    fp->private_data = pmaddevobj; /* for other methods */
    mutex_unlock(&pmaddevobj->devmutex);

	return 0;          /* success */
}

//This is the release function for one hardware device
static int maddevr_release(struct inode *inode, struct file *fp)
{
	struct mad_dev_obj *pmaddevobj = (struct mad_dev_obj*)fp->private_data;
    int rc = 0;

	PINFO("maddevr_release...dev#=%d inode=%p fp=%p\n\n",
          (int)pmaddevobj->devnum, inode, fp);

    mutex_lock(&pmaddevobj->devmutex);
    if (pmaddevobj->vma != NULL)
        {pmaddevobj->vma = NULL;}
    mutex_unlock(&pmaddevobj->devmutex);

    if (rc != 0)
        {PERR("maddevr_release... dev#=%d rc=%d\n", (int)pmaddevobj->devnum, rc);}

	return rc;
}

/*
 * Data management: read and write
 */
//This is the generic read function
static ssize_t
maddevr_read(struct file *fp, char __user *usrbufr, size_t count, loff_t *f_pos)
{
    struct mad_dev_obj *pmaddevobj = (struct mad_dev_obj*)fp->private_data;
    ssize_t iocount = 0;                                                         

    PINFO("maddevr_read... dev#=%d fp=%px count=%ld offset_arg=%ld fppos=%ld\n",
          (int)pmaddevobj->devnum, (void *)fp,
          (long int)count, (unsigned long int)*f_pos, (long int)fp->f_pos);

    mutex_lock(&pmaddevobj->devmutex);

    //Do a large buffered read using direct-io of the user buffer
    iocount = maddevb_direct_io(fp, usrbufr, count, f_pos, false);

    mutex_unlock(&pmaddevobj->devmutex);

    return iocount;
}

//This is the generic write function
static ssize_t maddevr_write(struct file *fp, const char __user *usrbufr, size_t count,
                            loff_t *f_pos)
{
    PMADDEVOBJ pmaddevobj = (PMADDEVOBJ)fp->private_data;
    ssize_t iocount = 0;                                                         

    PINFO("maddevr_write... dev#=%d fp=%px count=%ld f_pos=%ld fp->pos=%ld\n",
          (int)pmaddevobj->devnum, (void *)fp, (long int)count,
          (long int)*f_pos, (long int)fp->f_pos);

    mutex_lock(&pmaddevobj->devmutex);

    //Do a large buffered write using direct-io of the user buffer
    iocount = maddevb_direct_io(fp, usrbufr, count, f_pos, true);

    mutex_unlock(&pmaddevobj->devmutex);

    return iocount;
}

long maddevr_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	static MADREGS  MadRegs;
	//
	struct mad_dev_obj *pmaddevobj = fp->private_data;
	PMADREGS        pmadregs  = (PMADREGS)pmaddevobj->pDevBase;
	PMADCTLPARMS    pCtlParms = (PMADCTLPARMS)arg;
    //u8 tempbufr[] = "____________\0";
	//
	int err = 0;
	long retval = 0;
	U32  remains = 0;
    u32 flags1 = 0;
    //U32 flags2 = 0;
    u8   cachebufr[MAD_SECTOR_SIZE];
    u8*  devcache = NULL;
    u8*  userbufr = NULL;

	PINFO("maddevr_ioctl... dev#=%d fp=%px cmd=x%X arg=x%X\n",
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
		PDEBUG( "maddevr_ioctl returning -EINVAL\n");
		return -EINVAL;
        }

    //If the ioctl queue is not free we return
    if (pmaddevobj->ioctl_f != eIoReset)
        {return -EAGAIN;}

    mutex_lock(&pmaddevobj->devmutex);

    switch(cmd)
	    {
	    case MADDEVOBJ_IOC_INIT: //Initialize the device in a standard way
	    	PDEBUG( "maddevr_ioctl... dev#=%d MADDEVOBJ_IOC_INIT\n", (int)pmaddevobj->devnum);
            maddev_acquire_lock_disable_ints(&pmaddevobj->devlock, flags1);
            memcpy_toio(pmadregs, &MadRegsRst, sizeof(MADREGS));
            maddev_enable_ints_release_lock(&pmaddevobj->devlock, flags1);
            break;

	    case MADDEVOBJ_IOC_RESET: //Reset all index registers
	    	PDEBUG("maddevr_ioctl... dev#=%d MADDEVOBJ_IOC_RESET\n", (int)pmaddevobj->devnum);
            maddev_acquire_lock_disable_ints(&pmaddevobj->devlock, flags1);
            iowrite32(0, &pmadregs->ByteIndxRd);
	    	iowrite32(0, &pmadregs->ByteIndxWr);
	    	iowrite32(0, &pmadregs->CacheIndxRd);
	    	iowrite32(0, &pmadregs->CacheIndxWr);
	    	iowrite32(MAD_STATUS_CACHE_INIT_MASK, &pmadregs->Status);
            maddev_enable_ints_release_lock(&pmaddevobj->devlock, flags1);
            break;

	    case MADDEVOBJ_IOC_GET_DEVICE:
		    PDEBUG("maddevr_ioctl... dev#=%d; MADDEVOBJ_IOC_GET_DEVICE\n",
                   (int)pmaddevobj->devnum);
		    //
		    memset(&MadRegs, 0xcc, sizeof(MADREGS));
            maddev_acquire_lock_disable_ints(&pmaddevobj->devlock, flags1);
            memcpy_fromio(&MadRegs, pmadregs, sizeof(MADREGS));
            maddev_enable_ints_release_lock(&pmaddevobj->devlock, flags1);

            //possibly paged memory - copy outside of spinlock
            remains =
		    copy_to_user(&pCtlParms->MadRegs, &MadRegs, sizeof(MADREGS)); 
            if (remains > 0)
                {
                PERR("maddevr_ioctl:copy_to_user...  dev#=%d bytes_remaining=%ld rc=-EFAULT\n",
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

        case MADDEVOBJ_IOC_ALIGN_READ_CACHE:
            PDEBUG("maddevr_ioctl... MADDEVOBJ_IOC_ALIGN_READ_CACHE dev#=%d cache_dx=%ld\n",
                   (int)pmaddevobj->devnum, arg);

            if ((arg < 0) || (arg > MAD_DEVICE_MAX_SECTORS-1))
                {
                retval = -EINVAL;
                break;
                }

            MadRegs.CacheIndxRd = arg; //The sector#
            maddev_acquire_lock_disable_ints(&pmaddevobj->devlock, flags1);
            iowrite32(MadRegs.CacheIndxRd, &pmadregs->CacheIndxRd);
            maddev_enable_ints_release_lock(&pmaddevobj->devlock, flags1);
            break;

        case MADDEVOBJ_IOC_ALIGN_WRITE_CACHE:
            PDEBUG("maddevr_ioctl... MADDEVOBJ_IOC_ALIGN_WRITE_CACHE dev#=%d cache_dx=%ld\n",
                   (int)pmaddevobj->devnum, arg);

            if ((arg < 0) || (arg > MAD_DEVICE_MAX_SECTORS-1))
                {
		        retval = -EINVAL;
                break;
                }

            MadRegs.CacheIndxWr = arg; //The sector#
            maddev_acquire_lock_disable_ints(&pmaddevobj->devlock, flags1);
		    iowrite32(MadRegs.CacheIndxWr, &pmadregs->CacheIndxWr);
            maddev_enable_ints_release_lock(&pmaddevobj->devlock, flags1);
            break;

        case MADDEVOBJ_IOC_PULL_READ_CACHE:
            PDEBUG("maddevr_ioctl... MADDEVOBJ_IOC_PULL_READ_CACHE dev#=%d\n",
                   (int)pmaddevobj->devnum);

            //Retrieve the current cache for the user
            devcache = (u8*)((u64)pmadregs + MAD_CACHE_READ_OFFSET);
            maddev_acquire_lock_disable_ints(&pmaddevobj->devlock, flags1);
            memcpy_fromio(cachebufr, devcache, MAD_SECTOR_SIZE);
            maddev_enable_ints_release_lock(&pmaddevobj->devlock, flags1);

            //possibly paged memory - copy outside of spinlock
            userbufr = (u8*)pCtlParms;
            remains = copy_to_user(userbufr, cachebufr, MAD_SECTOR_SIZE); 

            //Program the device for a transfer from the device to the read cache
            maddev_acquire_lock_disable_ints(&pmaddevobj->devlock, flags1);
            iowrite32(0, &pmadregs->IntID);
            iowrite32((MAD_INT_BUFRD_INPUT_BIT|MAD_INT_STATUS_ALERT_BIT),
                       &pmadregs->IntEnable);
            iowrite32((MAD_CONTROL_CACHE_XFER_BIT|MAD_CONTROL_BUFRD_GO_BIT),
                       &pmadregs->Control);
            maddev_enable_ints_release_lock(&pmaddevobj->devlock, flags1);

            //Release our quantum:
            // not processing for an interrupt - sleazy synchronous implementation
            while (ioread32(&pmadregs->IntID) == 0)
                {schedule();}

            //Confirm that we have a good interrupt
            maddev_acquire_lock_disable_ints(&pmaddevobj->devlock, flags1);
            //ASSERT((int)(ioread32(&pmadregs->IntID)==MAD_INT_BUFRD_INPUT_BIT));
            //ASSERT((int)(ioread32(&pmadregs->Status)==MAD_STATUS_NO_ERROR_MASK));
            maddev_reset_io_registers(pmadregs, NULL);
            maddev_enable_ints_release_lock(&pmaddevobj->devlock, flags1);
            break;

        case MADDEVOBJ_IOC_PUSH_WRITE_CACHE:
            PDEBUG("maddevr_ioctl... MADDEVOBJ_IOC_PUSH_WRITE_CACHE dev#=%d\n",
                   (int)pmaddevobj->devnum);

            //Program the device for a transfer from the write cache to the device
            maddev_acquire_lock_disable_ints(&pmaddevobj->devlock, flags1);
            iowrite32(0, &pmadregs->IntID);
            iowrite32((MAD_INT_BUFRD_OUTPUT_BIT|MAD_INT_STATUS_ALERT_BIT),
                       &pmadregs->IntEnable);
            iowrite32((MAD_CONTROL_CACHE_XFER_BIT|MAD_CONTROL_BUFRD_GO_BIT),
                       &pmadregs->Control);
            maddev_enable_ints_release_lock(&pmaddevobj->devlock, flags1);

            //Release our quantum:-
            // not processing for an interrupt - sleazy synchronous implementation
            while (ioread32(&pmadregs->IntID) == 0)
                {schedule();}

            //Confirm that we have a good interrupt
            //ASSERT((int)(ioread32(&pmadregs->IntID)==MAD_INT_BUFRD_OUTPUT_BIT));
            //ASSERT((int)(ioread32(&pmadregs->Status)==MAD_STATUS_NO_ERROR_MASK));

            //possibly paged memory - copy outside of spinlock
            userbufr = (u8*)pCtlParms;
            remains = copy_from_user(cachebufr, userbufr, MAD_SECTOR_SIZE);

            //Update the write cache from the user
            devcache = (u8*)((u64)pmadregs + MAD_CACHE_WRITE_OFFSET);
            maddev_acquire_lock_disable_ints(&pmaddevobj->devlock, flags1);
            memcpy_toio(devcache, cachebufr, MAD_SECTOR_SIZE);
            maddev_reset_io_registers(pmadregs, NULL);
            maddev_enable_ints_release_lock(&pmaddevobj->devlock, flags1);
            break;

        //Not implemented
	    case MADDEVOBJ_IOC_GET_ENABLE:
	    case MADDEVOBJ_IOC_GET_CONTROL:
	        retval = -ENOSYS; 
            break;

	    default:
		    retval = -EINVAL;
	    }

    mutex_unlock(&pmaddevobj->devmutex);

    if (retval != 0)
        {PERR("maddevr_ioctl... devnum=%d rc=%d\n", (int)pmaddevobj->devnum, retval);}

	return retval;
}

//This is the table of entry points for device i/o operations
static struct file_operations maddevr_fops = 
{
	.owner          = THIS_MODULE,
	.llseek         = maddev_llseek,
	.read           = maddevr_read,
	.write          = maddevr_write,
    //.read_iter      = maddev_queued_read,
    //.write_iter     = maddev_queued_write,
    .unlocked_ioctl = maddevr_ioctl,
    .mmap           = maddev_mmap,
    .open           = maddevr_open,
	.release        = maddevr_release,
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
int maddevr_setup_cdev(void* pvoid, int indx)
{
    struct mad_dev_obj* pmaddevobj = (struct mad_dev_obj*)pvoid;
	int devno = MKDEV(maddev_major, maddev_minor + indx);
	int rc = 0;
        
	PDEBUG("maddevb_setup_cdev... dev#=%d maddev_major=%d maddev_minor=%d cdev_no=x%X\n",
		   indx, maddev_major, (maddev_minor+indx), devno);

    //Initialize the cdev structure
	cdev_init(&pmaddevobj->cdev_str, &maddevr_fops);
	pmaddevobj->cdev_str.owner = THIS_MODULE;

    //Introduce the device to the kernel
	rc = cdev_add(&pmaddevobj->cdev_str, devno, 1);
	//if (rc) 	/* Fail gracefully if need be */
    //    {PERR("maddev_setup_cdev:cdev_add... dev#=%d rc=%d", indx, rc);}

    PDEBUG("maddevb_setup_cdev... dev#=%d rc=%d\n", (int)pmaddevobj->devnum, rc);

    return rc;
}

// This is the driver init function
// It allocates memory and initializes all static device objects
//
int maddevb_init_module(void)
{
	int   rc = 0;
	U32    devcount = 0;
    size_t SetSize = ((maddev_max_devs+3) * PAGE_SIZE); //Add padding to paint over a memory error

    int  mjr = 0;
    int len;
    u8 bMSI = (mad_pci_devid == MAD_PCI_BLOCK_MSI_DEVICE_ID);

    maddevb_major = maddev_major;
	PINFO("maddevb_init_module... mjr=%d mnr=%d\n", maddevb_major, maddev_minor);

    /*
	mjr = register_blkdev(maddevb_major, MAD_MAJOR_DEVICE_NAME);
	if (mjr < 0)
	    {
        PERR("maddevb_init_module... register_blkdev rc=%d\n", mjr);
		return -EIO;
	    }

    if (mjr > 0)
        {
        maddevb_major = mjr;
        maddev_major = mjr;
        PINFO("maddevb_init_module... mjr=%d reassigned!\n", maddevb_major);
        } /* */

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
        PERR("maddevb_init_module... kmalloc failed!\n");
        rc = -ENOMEM;
		goto InitFail;  
	    }

    //* Register with the driver core through the bus driver API
    // Register as a pci device driver
    maddev_driver.driver.p->driver = &maddev_driver.driver;
    //rc = maddev_kobject_init(&maddev_driver.driver.p->kobj, NULL, 
    //                         &mad_kset, &mad_ktype, "maddrvrb_kobj");
    rc = pci_register_driver(&maddev_driver);
    if (rc != 0)
        {
        PERR("maddevb_init_module:pci_register_driver... rc=%d\n", rc);
		goto InitFail;  
	    }

    //mad_kset_create();

    /* Create & initialize each device. */
    PINFO("maddevb_init_module... #_static_devices=%d\n", maddev_nbr_devs);

    if (maddev_nbr_devs > 0) //Static devices
        devcount = maddev_setup_devices(maddev_nbr_devs, false, bMSI); 

#ifdef MADDEVOBJ_DEBUG /* only when debugging */
	maddev_create_proc();
#endif

    if (maddev_nbr_devs == 0) //No static devices
        {rc = 0;} 
    else //Did at least one static device init succeed
        {rc = (devcount > 0) ? 0 : -ENODEV;}
 
InitFail:
    PINFO("maddevb_init_module exit... rc=%d\n", rc);
    if (rc != 0)
        {maddev_cleanup_module();}

	return rc;
}

//Declare the driver init & driver exit functions
//
module_init(maddevb_init_module);
module_exit(maddev_cleanup_module);
