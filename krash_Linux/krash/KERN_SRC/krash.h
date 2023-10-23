/*******************************************************************************/
/*                                                                             */
/*  Exe files   : kcrash.ko                                                    */ 
/*                                                                             */
/*  Module NAME : kcrash.h                                                     */
/*                                                                             */
/*  DESCRIPTION : Properties & Definitions for the kernel-mode crash module    */
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
/* HTF Consulting takes no responsibility for errors or fitness of use         */
/*                                                                             */
/*                                                                             */
/* $Id: kcrash.h, v 1.0 2023/01/01 00:00:00 htf $                              */
/*                                                                             */
/*******************************************************************************/


#define DRIVER_NAME "krash"
#define PDEBUG(fmt,args...) printk(KERN_DEBUG"%s:"fmt,DRIVER_NAME, ##args)
#define PERR(fmt,args...) printk(KERN_ERR"%s:"fmt,DRIVER_NAME,##args)
#define PINFO(fmt,args...) printk(KERN_INFO"%s:"fmt,DRIVER_NAME, ##args)
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/capability.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/jiffies.h>
#include <linux/kdev_t.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>

#define KRASH_N_MINORS 1
#define KRASH_FIRST_MINOR 1
#define KRASH_NODE_NAME "krashdev"
#define KRASH_BUFF_SIZE 1024
#define KRASH_PLATFORM_NAME DRIVER_NAME
#define KRASH_DELAY_MS 10

#define KRASH_UNUSED_LINUX_IOC_MAGIC_NUMBER  0x9A
#define KRASH_DEV_IOC_MAGIC        KRASH_UNUSED_LINUX_IOC_MAGIC_NUMBER
#define KRASH_DEV_IOC_INDEX_BASE   0
//
#define KRASH_DEV_IOC_KRASH_NUM   _IOW(KRASH_DEV_IOC_MAGIC,       \
                                      KRASH_DEV_IOC_INDEX_BASE+1, \
                                      unsigned long)

typedef struct privatedata
{
	int nMinor;

	char buff[KRASH_BUFF_SIZE];

	struct cdev cdev;

	//struct timer_list ktimer1;

	struct tasklet_struct krash_tasklet;

	spinlock_t spinlock1;

	spinlock_t spinlock2;

    int arg;

	struct device *krash_device;
} krash_private;
