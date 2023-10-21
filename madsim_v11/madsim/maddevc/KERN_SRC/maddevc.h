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
#include <linux/dma-direction.h>
#include <linux/highmem.h>
#include <linux/interrupt.h>
#include <linux/uio.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>

#include <asm/io.h>
#include <asm/uaccess.h>	/* copy_*_user */
//
#define _MAD_SIMULATION_MODE_
//
#define  DRIVER_NAME   "maddevc.ko"
#include "../../madbus/KERN_SRC/madbus.h"
#include "../../include/maddevioctls.h"
#include "../../include/maddrvrdefs.h"

#define  MADDEVOBJNAME      "maddevc_objX"
#define  MADDEVOBJNUMDX     11 //.......^

//Macros to help debugging
//
#if 0
#undef PDEBUG             /* undef it, just in case */
#ifdef MADDEV_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
    #define PDEBUG(fmt, args...) printk( KERN_DEBUG "maddevobj: " fmt, ## args)
#  else
     /* This one for user space */
    #define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
    #define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

#undef PDEBUGG
#define PDEBUGG(fmt, args...) /* nothing: it's a placeholder */
#endif

#define MADDEV_NBR_DEVS    MADBUS_NUMBER_SLOTS

#define MAD_MAJOR_OBJECT_NAME     "maddevc_obj"

/*
 * Split minors in two parts
 */
#define TYPE(minor)	(((minor) >> 4) & 0xf)	/* high nibble */
#define NUM(minor)	((minor) & 0xf)		/* low  nibble */

/*
 * The different configurable parameters
 */
extern int maddev_major;     /* main.c */
extern int maddev_nbr_devs;

//
extern ssize_t maddev_direct_io(struct file *filp, const char __user *buf, 
                                size_t count, loff_t *f_pos, bool bWr);

#ifdef _DEVICE_DRIVER_MAIN_
static int maddev_setup_cdev(/*PMADDEVOBJ*/ void* pmaddevobj, int indx);
#endif

static long maddev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
extern irqreturn_t maddevc_isr(int irq, void* dev_id);
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
#include "../../include/sim_aliases.h"
#endif //_MAD_SIMULATION_MODE_
#endif /* _MADDEVC_H_ */
