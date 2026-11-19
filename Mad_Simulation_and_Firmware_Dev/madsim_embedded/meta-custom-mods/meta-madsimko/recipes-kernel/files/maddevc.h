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
/*  Module NAME : maddevc.h                                                    */
/*                                                                             */
/*  DESCRIPTION : Properties & Definitions for the MAD character mode driver   */
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
/* $Id: maddevc.h, v 1.0 2021/01/01 00:00:00 htf $                             */
/*                                                                             */
/*******************************************************************************/

#ifndef _MADDEVC_H_
#define _MADDEVC_H_

#define _CDEV_
#define _KERNEL_MODULE_

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */
#include <linux/device.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/pci.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/dma-direction.h>
#include <linux/highmem.h>
#include <linux/interrupt.h>
#include <linux/uio.h>
#include <linux/uaccess.h> 
#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <asm/io.h>
//#include <asm/uaccess.h>	/* copy_*_user */
//
#define _MAD_SIMULATION_MODE_
//
#define  DRIVER_NAME   "maddevc.ko"
#include "madbus.h"
#include "maddevioctls.h"
#include "maddrvrdefs.h"

#define  MADDEVOBJNAME      "maddevc_objX"
#define  MADDEVOBJNUMDX     11 //.......^

//Macros to help debugging
//
#if 0
#undef PINFO             /* undef it, just in case */
#ifdef MADDEV_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
    #define PINFO(fmt, args...) printk( KERN_DEBUG "maddevobj: " fmt, ## args)
#  else
     /* This one for user space */
    #define PINFO(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
    #define PINFO(fmt, args...) /* not debugging: nothing */
#endif

#undef PINFOG
#define PINFOG(fmt, args...) /* nothing: it's a placeholder */
#endif

#define MADDEV_NBR_DEVS    MADBUS_NUMBER_SLOTS

#define MAD_MAJOR_OBJECT_NAME     "maddevc_obj"

/*
 * Split minors in two parts
 */
#define TYPE(minor)	(((minor) >> 4) & 0xf)	/* high nibble */
#define NUM(minor)	((minor) & 0xf)		/* low  nibble */

//typedef irq_handler_t isr_function(int irq, void* dev_id);
/*
 * The different configurable parameters
 */
extern int maddev_major;     /* main.c */
extern int maddev_nbr_devs;

int maddevc_setup_cdev(void* pvoid, int indx);

//#ifdef _DEVICE_DRIVER_MAIN_
//static int maddev_setup_cdev(/*PMADDEVOBJ*/ void* pmaddev, int indx);
//#endif

irqreturn_t maddevc_msi_one_isr(int irq, void* dev_id);
irqreturn_t maddevc_msi_two_isr(int irq, void* dev_id);
irqreturn_t maddevc_msi_three_isr(int irq, void* dev_id);
irqreturn_t maddevc_msi_four_isr(int irq, void* dev_id);
irqreturn_t maddevc_msi_five_isr(int irq, void* dev_id);
irqreturn_t maddevc_msi_six_isr(int irq, void* dev_id);
irqreturn_t maddevc_msi_seven_isr(int irq, void* dev_id);
irqreturn_t maddevc_msi_eight_isr(int irq, void* dev_id);

//This is the common worker function for all (msi & legacy interrupt functions
//void *dev_get_drvdata(const struct device *dev); void dev_set_drvdata(struct device *dev, void *data)
static inline irqreturn_t maddevc_isr_worker_fn(int irq, void* dev_id, int msinum)
{
    PMADDEVOBJ pmaddev = dev_id;
	__iomem MADREGS *pmadregs;
	U32 flags1 = 0;
	u32 IntID = 0;
    u32 Control = 0;
    int devnum = 0;

    ASSERT((int)(pmaddev != NULL));
    pmadregs = pmaddev->pDevBase;
    devnum = pmaddev->devnum;
 
    maddev_acquire_lock_disable_ints(&pmaddev->devlock, &flags1);
    IntID = ioread32(&pmadregs->IntID);
    Control = ioread32(&pmadregs->Control);

    PINFO("maddevc_isr... dev#=%d pmaddev=%px irq=%d msinum=%d IntID=x%X Control=x%X\n",
          (int)devnum, pmaddev, irq, msinum, 
          (uint32_t)IntID, (uint32_t)Control);
 
    //Save the device register state for the DPC
	memcpy_fromio((PMADREGS)&pmaddev->IntRegState, pmadregs, MADREGS_BASE_SIZE);

    //Disable interrupts on this device
    iowrite32(MAD_ALL_INTS_DISABLED, &pmadregs->IntEnable);
    iowrite32(0, &pmadregs->MesgID);

    if (IntID == (u32)MAD_ALL_INTS_CLEARED)
        {   //This is not our IRQ
        maddev_enable_ints_release_lock(&pmaddev->devlock, &flags1);

        PERR("maddevc_isr... invalid int recv'd: dev#=%d IntID=x%X rc=%d\n",
             (int)pmaddev->devnum, IntID, IRQ_NONE);
    	return IRQ_NONE;
        }

    if (IntID == (u32)MAD_INT_INVALID_BYTEMODE_MASK)//Any / all undefined int conditions
        {
        maddev_enable_ints_release_lock(&pmaddev->devlock, &flags1);

        PERR("maddevc_isr... undefined int recv'd: dev#=%d IntID=x%X rc=%d\n",
             (int)pmaddev->devnum, IntID, IRQ_HANDLED);
    	return IRQ_HANDLED;
        }

    #ifdef _MAD_SIMULATION_MODE_ 
    //Release the spinlock *NOT* at device-irql BEFORE enqueueing the DPC
    maddev_enable_ints_release_lock(&pmaddev->devlock, &flags1);
    #endif

    /* Schedule the DPC handler */
    pmaddev->dpctask.data = devnum;
    BUG_ON(pmaddev->dpctask.func == NULL);
    tasklet_hi_schedule(&pmaddev->dpctask);

    #ifndef _MAD_SIMULATION_MODE_
    //With real hardware release the spinlock at device-irql AFTER enqueueing the DPC
    maddev_enable_ints_release_lock(&pmaddev->devlock, flags1);
    #endif

    PINFO("maddevc_isr... normal exit: dev#=%d IntID=x%X rc=%d\n",
           (int)pmaddev->devnum, IntID, IRQ_HANDLED);

	return IRQ_HANDLED;
}

long maddevc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
extern irqreturn_t maddevc_legacy_isr(int irq, void* dev_id);
irqreturn_t maddevc_legacy_isr(int irq, void* dev_id);
//void maddevc_dpc(struct work_struct *work);
extern void maddevc_dpctask(ulong indx);
void maddev_complete_io(struct work_struct *work);
extern ssize_t maddev_read_bufrd(struct file *fp, char __user *usrbufr,
                                 size_t count, loff_t *f_pos);
ssize_t maddev_write_bufrd(struct file *filp, const char __user *buf,
                           size_t count, loff_t *f_pos);

extern ssize_t maddev_queued_read(struct kiocb *pkiocb, struct iov_iter *piov);
extern ssize_t maddev_queued_write(struct kiocb *pkiocb, struct iov_iter *piov);
void maddev_dpcwork_rd(struct work_struct *work);
void maddev_dpcwork_wr(struct work_struct *work);

#ifdef _MAD_SIMULATION_MODE_
//Alias the pci functions to the simulator replacements
#include "sim_aliases.h"
#endif //_MAD_SIMULATION_MODE_
#endif /* _MADDEVC_H_ */
