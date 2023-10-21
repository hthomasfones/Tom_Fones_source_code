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
/*  Module NAME : maddevb.h                                                    */
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
/* $Id: maddevb.h, v 1.0 2021/01/01 00:00:00 htf $                             */
/*                                                                             */
/*******************************************************************************/

#ifndef _MADDEVB_H_
#define _MADDEVB_H_

#define _BLOCKIO_

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
#include <linux/genhd.h>
#include <linux/hdreg.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/configfs.h>
#include <linux/badblocks.h>
#include <linux/buffer_head.h>	/* invalidate_bdev */
#include <linux/bio.h>
#include <linux/uio.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/cdrom.h>

//
#include <asm/io.h>
#include <asm/uaccess.h>	/* copy_*_user */
//
#define _MAD_SIMULATION_MODE_
//
#define  DRIVER_NAME   "maddevb.ko"
#include "../../madbus/KERN_SRC/madbus.h"
#include "../../include/maddevioctls.h"
#include "../../include/maddrvrdefs.h"

#define  MADDEVOBJNAME      "maddevb_objX"
#define  MADDEVOBJNUMDX     11 //.......^

#define MAD_MAJOR_DEVICE_NAME     "maddevb"
#define MAD_MAJOR_OBJECT_NAME     "maddevb_obj"

#define MADDEV_NBR_DEVS         MADBUS_NUMBER_SLOTS
#define MADDEVB_NUM_SUBMIT_QUEUES 1
#define MADDEVB_DISK_SIZE       MAD_DEVICE_DATA_SIZE
//(MAD_DEVICE_MAX_SECTORS * MAD_SECTOR_SIZE)
#define MADDEVB_IO_TIMEOUT_MILLI_SECS 1
#define MADDEVB_IO_TIMEOUT_NANO_SECS \
        (MADDEVB_IO_TIMEOUT_MILLI_SECS * 1000 * 1000)

//Build options
//#define MADDEVB_ZONED
//#define MADDEVB_POWER_MNGT
//#define MADDEVB_BAD_BLOCK

#define LINUX_MAJOR_DEV_FLOPPY_DISK 2
#define MADDEV_MAJOR       LINUX_MAJOR_DEV_FLOPPY_DISK

#define MADDEVB_GENHD_FLAGS \
        (GENHD_FL_EXT_DEVT | GENHD_FL_UP | GENHD_FL_NO_PART_SCAN)

/*
 * Split minors in two parts
 */
#define TYPE(minor)	(((minor) >> 4) & 0xf)	/* high nibble */
#define NUM(minor)	((minor) & 0xf)		/* low  nibble */

#define PAGE_SECTORS_SHIFT	(PAGE_SHIFT - SECTOR_SHIFT)
#define PAGE_SECTORS		(1 << PAGE_SECTORS_SHIFT)
#define SECTOR_MASK		(PAGE_SECTORS - 1)

#define FREE_BATCH		16

#define TICKS_PER_SEC		50ULL
#define TIMER_INTERVAL		(NSEC_PER_SEC / TICKS_PER_SEC)

#define MAP_SZ		((PAGE_SIZE >> SECTOR_SHIFT) + 2)
struct maddevb_page
{
	struct page *page;
	DECLARE_BITMAP(bitmap, MAP_SZ);
};

#define MADDEVB_PAGE_LOCK (MAP_SZ - 1)
#define MADDEVB_PAGE_FREE (MAP_SZ - 2)

extern int  g_queue_mode;
extern int  g_irqmode;
extern int  g_home_node;
extern int  g_no_sched;
extern bool g_blocking;
extern int  g_gs;
extern int  g_bs;
extern int  g_submit_queues;
extern int  g_hw_queue_depth;
extern bool g_use_per_node_hctx;
extern int  g_bs;
extern bool g_zoned;
extern unsigned long g_completion_nsec;
extern bool shared_tags;

enum
{
	NULL_IRQ_NONE		= 0,
	NULL_IRQ_SOFTIRQ	= 1,
	NULL_IRQ_TIMER		= 2,
};

enum
{
	MADDEVB_Q_BIO		= 0,
	MADDEVB_Q_RQ		= 1,
	MADDEVB_Q_MQ		= 2,
};

/*
 * Status flags for maddevb_device.
 *
 * CONFIGURED:	Device has been configured and turned on. Cannot reconfigure.
 * UP:		Device is currently on and visible in userspace.
 * THROTTLED:	Device is being throttled.
 * CACHE:	Device is using a write-back cache.
 */
enum maddevb_device_flags
{
	MADDEVB_DEV_FL_CONFIGURED = 0,
	MADDEVB_DEV_FL_UP		  = 1,
	MADDEVB_DEV_FL_THROTTLED  = 2,
	MADDEVB_DEV_FL_CACHE	  = 3,
};

#define MAP_SZ		((PAGE_SIZE >> SECTOR_SHIFT) + 2)
/*
 * The different configurable parameters
 */
extern int maddev_major;     /* main.c */
extern int maddev_nbr_devs;

static struct maddevb_cmd
{
	struct list_head list;
	struct llist_node ll_list;
	struct __call_single_data csd;
	struct request *req;
	struct bio *bio;
	unsigned int tag;
	blk_status_t error;
	struct maddevb_queue *nq;
	struct hrtimer timer;
};

static struct maddevb_queue
{
	unsigned long *tag_map;
	wait_queue_head_t wait;
	unsigned int queue_depth;
	struct maddevblk_device *dev;
	unsigned int requeue_selection;

	struct maddevb_cmd *cmds;
};

static struct maddevb 
{
	struct maddevblk_device *mbdev;
	struct list_head list;
	unsigned int index;
	struct request_queue *reqQ;
	struct gendisk *disk;
	struct blk_mq_tag_set *tag_set;
	struct blk_mq_tag_set __tag_set;
	unsigned int queue_depth;
	atomic_long_t cur_bytes;
	struct hrtimer bw_timer;
	unsigned long cache_flush_pos;
	spinlock_t lock;
	struct maddevb_queue *queues;
	unsigned int nr_queues;
	char disk_name[DISK_NAME_LEN];
};

static struct maddevblk_device
{
	struct maddevb *pmaddevb;
	struct config_item item;
	struct radix_tree_root data; /* data stored in the disk */
	struct radix_tree_root cache; /* disk cache data */
	unsigned long flags; /* device flags */
	unsigned int curr_cache;
	struct badblocks badblocks;

	unsigned int nr_zones;
	struct blk_zone *zones;
	sector_t zone_size_sects;

	unsigned long size; /* device size in MB */
	unsigned long completion_nsec; /* time in ns to complete a request */
	unsigned long cache_size; /* disk cache size in MB */
	unsigned long zone_size; /* zone size in MB if device is zoned */
	unsigned int zone_nr_conv; /* number of conventional zones */
	unsigned int submit_queues; /* number of submission queues */
	unsigned int home_node; /* home node for the device */
	unsigned int queue_mode; /* block interface */
	unsigned int blocksize; /* block size */
	unsigned int irqmode; /* IRQ completion handler */
	unsigned int hw_queue_depth; /* queue depth */
	unsigned int index; /* index of the disk, only valid with a disk */
	unsigned int mbps; /* Bandwidth throttle cap (in MB/s) */
	bool blocking; /* blocking blk-mq device */
	bool use_per_node_hctx; /* use per-node allocation for hardware context */
	bool power; /* power on/off the device */
	bool memory_backed; /* if data is stored in memory */
	bool discard; /* if support discard */
	bool zoned; /* if device is zoned */
    void*     pmaddevobj;
};

static inline u64 mb_per_tick(int mbps)
{
	return (1 << 20) / TICKS_PER_SEC * ((u64) mbps);
}

extern int maddevb_create_device(/*PMADDEVOBJ pmaddev*/void* pv);
static int maddevb_gendisk_register(struct maddevb *maddevb);
static blk_qc_t maddevb_queue_bio(struct request_queue *q, struct bio *bio);
void maddevb_restart_queue_async(struct maddevb *maddevb);
static void maddevb_end_cmd(struct maddevb_cmd *cmd);
static bool maddevb_should_requeue_request(struct request *rq);
static bool maddevb_should_timeout_request(struct request *rq);
static blk_status_t maddevb_handle_cmd(struct maddevb_cmd *cmd, sector_t sector,
				                       sector_t nr_sectors, enum req_opf op);
static int maddevb_init_tag_set(struct maddevb *maddevb, struct blk_mq_tag_set *set);
static int maddevb_init_driver_queues(struct maddevb *maddevb);
static void maddevb_init_queues(struct maddevb *maddevb);
static void maddevb_config_discard(struct maddevb *maddevb);
static int maddevb_setup_queues(struct maddevb *maddevb);
static void maddevb_validate_conf(struct maddevblk_device *dev);
bool maddevb_setup_fault(void);
static void maddevb_setup_bwtimer(struct maddevb *maddevb);
void maddevb_cleanup_queues(struct maddevb *maddevb);
extern void maddevb_delete_device(struct maddevb *maddevb);
static void maddevb_free_device(struct maddevblk_device *dev);
int maddevb_ioctl(struct block_device* bdev, fmode_t mode, 
                  unsigned int cmd, unsigned long arg);
int maddevb_collect_biovecs(struct mad_dev_obj *pmaddevobj, struct bio* pbio,
                            u32 nr_sectors, struct bio_vec* pbiovecs);
extern ssize_t 
maddevb_xfer_sgdma_bvecs(PMADDEVOBJ pmaddevobj, struct bio_vec* biovecs,
                        long nr_bvecs, sector_t sector, u32 nr_sectors, bool bWr);

#ifdef CONFIG_BLK_DEV_ZONED
int maddevb_zone_init(struct maddevblk_device *dev);
void maddevb_zone_exit(struct maddevblk_device *dev);
//int maddevb_zone_report(struct gendisk *disk, sector_t sector,
//		                struct blk_zone *zones, unsigned int *nr_zones);
//static blk_status_t 
//maddevb_handle_zoned(struct maddevblk_cmd *cmd,
//				       enum req_opf op, sector_t sector,
//				       sector_t nr_sectors);
#else
static inline int maddevb_zone_init(struct maddevblk_device *dev)
{
	PERR("CONFIG_BLK_DEV_ZONED not enabled\n");
	return -EINVAL;
}
#endif

static inline void* 
maddevb_get_parent_from_bdev(struct block_device *bdev)
{
    struct maddevb *maddevb = bdev->bd_disk->private_data;

    ASSERT((int)(maddevb != NULL));
    ASSERT((int)(maddevb->mbdev != NULL));
    ASSERT((int)(maddevb->mbdev->pmaddevobj != NULL));

    return maddevb->mbdev->pmaddevobj;
}

#if 0
//static inline void maddevb_zone_exit(struct maddevblk_device *dev) {}
static inline int 
maddevb_zone_report(struct gendisk *disk, sector_t sector,
                    struct blk_zone *zones, unsigned int *nr_zones)
{
	return -EOPNOTSUPP;
}

static inline blk_status_t 
maddevb_handle_zoned(struct maddevb_cmd *cmd,
				     enum req_opf op, sector_t sector,
					 sector_t nr_sectors)
{
	return BLK_STS_NOTSUPP;
}
#endif
blk_status_t maddevb_handle_zoned(struct maddevb_cmd *cmd, enum req_opf op,
	                             sector_t sector, sector_t nr_sectors);

#if 0
ssize_t
maddev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t
maddev_read_direct(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
ssize_t
maddev_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
ssize_t
maddev_write_direct(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);

//loff_t  maddev_llseek(struct file *filp, loff_t off, int whence);
#endif

static int maddevb_rw_page(struct block_device *bdev, sector_t sector,
		                   struct page *pPages, unsigned int op);
static int maddevb_open(struct block_device *bdev, fmode_t mode);
static void maddevb_release(struct gendisk *disk, fmode_t mode);
static int maddevb_revalidate_disk(struct gendisk *disk);
static int maddevb_getgeo(struct block_device* bdev, struct hd_geometry* geo);
static void maddevb_swap_slot_free_notify(struct block_device* bdev, 
                                          unsigned long offset);
//static int maddevb_zone_report(struct gendisk *disk, sector_t sector,
//		                       struct blk_zone *zones, unsigned int *nr_zones);
static int maddevb_zone_report(struct gendisk *disk, sector_t sector,
		                       /*struct blk_zone *zones,*/ 
							   unsigned int nr_zones, report_zones_cb cb, void* data);

//int maddevb_ioctl(struct block_device* bdev, fmode_t mode, unsigned int cmd, unsigned long arg);
extern irqreturn_t maddevb_isr(int irq, void* dev_id);
//void maddevb_dpc(struct work_struct *work);
extern void maddevb_dpctask(ulong indx);
//void maddevb_dpctask(ulong indx);
void maddev_complete_io(struct work_struct *work);
ssize_t maddev_queued_read(struct kiocb *pkiocb, struct iov_iter *piov);
ssize_t maddev_queued_write(struct kiocb *pkiocb, struct iov_iter *piov);
void maddev_dpcwork_rd(struct work_struct *work);
void maddev_dpcwork_wr(struct work_struct *work);
//
#ifdef _MAD_SIMULATION_MODE_
//Alias the pci functions to the simulator replacements
#include "../../include/sim_aliases.h"
#endif //_MAD_SIMULATION_MODE_
//
#endif /* _MADDEVB_H_ */
