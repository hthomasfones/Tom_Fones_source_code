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
#define _KERNEL_MODULE_
#define _MAD_SIMULATION_MODE_

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */
#include <linux/device.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/pci.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
//#include <linux/mm.h>
#include <linux/dma-direction.h>
#include <linux/version.h>
#include <linux/uaccess.h> 
#include <linux/vmalloc.h> 
#include <linux/highmem.h>
#include <linux/interrupt.h>
#include <linux/fs.h>    
#include <linux/blk_types.h> 

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,1,0)
    #error "Unsupported kernel version"
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,15,0) && LINUX_VERSION_CODE <= KERNEL_VERSION(5,15,999)
    #include <linux/genhd.h>
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,6,0)
    #include <linux/blkdev.h>   /* gendisk, add_disk, del_gendisk, etc. */
#endif

//#include <linux/backing-dev.h>
#include <linux/hdreg.h>
#include <linux/blk-mq.h>
#include <linux/scatterlist.h>
#include <linux/mm.h>
//#include <linux/folio.h>
#include <linux/pagemap.h>
#include <linux/page-flags.h>
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

#ifndef blk_opf_t
    typedef __u32 blk_opf_t;
#endif

/* GET_OPF_FROM_REQ(req) should return op+flags in a compatible type */
#ifndef GET_OPF_FROM_REQ
    #define GET_OPF_FROM_REQ(req)   (req)->cmd_flags
#endif

#if 0
#if defined(REQ_OP_MASK)
    #define req_opf req_op  
	#define GET_OPF_FROM_REQ(req) \
	        (enum req_opf)((blk_opf_t)req->cmd_flags & REQ_OP_MASK)
#else
	typedef enum req_opf blk_opf_t;
	#define GET_OPF_FROM_REQ(req)  (enum req_opf)req_op(req); 
#endif

/* GET_OPF_FROM_REQ(req) should return op+flags in a compatible type */
#ifndef GET_OPF_FROM_REQ
    #define GET_OPF_FROM_REQ(req)   (req)->cmd_flags
#endif
#endif

#define  DRIVER_NAME   "maddevb.ko"
#include "madbus.h"
#include "maddevioctls.h"
#include "maddrvrdefs.h"

#define  MADDEVOBJNAME      "maddevb_objX"
#define  MADDEVOBJNUMDX     11 //.......^

#define MAD_MAJOR_DEVICE_NAME     "maddevb"
#define MAD_MAJOR_OBJECT_NAME     "maddevb_obj"

#define MADDEV_NBR_DEVS         MADBUS_NUMBER_SLOTS
#define MADDEVB_NUM_SUBMIT_QUEUES 1
#define MADDEVB_DISK_SIZE       MAD_DEVICE_DATA_SIZE
#define MADDEVB_IO_TIMEOUT_MILLI_SECS 1
#define MADDEVB_IO_TIMEOUT_NANO_SECS \
        (MADDEVB_IO_TIMEOUT_MILLI_SECS * 1000 * 1000)

//Build options
//#define MADDEVB_ZONED
//#define MADDEVB_POWER_MNGT
//#define MADDEVB_BAD_BLOCK

//#define MADDEVB_GENHD_FLAGS  (GENHD_FL_EXT_DEVT | GENHD_FL_NO_PART_SCAN)
#define MADDEVB_GENHD_FLAGS  GENHD_FL_NO_PART
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

struct maddevb_cmd
{
	struct list_head list;
	struct llist_node ll_list;
	struct __call_single_data csd;
	struct request *req;
	struct bio *bio;
	blk_status_t error;
	struct maddevb_queue *pmdbq;
	struct hrtimer cmdtimer;
	struct work_struct blk_mq_req_work;
	ssize_t iosize;
	U32 dma_size;
	enum dma_data_direction dmadir;
	struct scatterlist *sglist;
	struct blk_mq_hw_ctx *hctx;
	//struct delayed_work_struct blk_mq_req_work;
};

struct maddevb_queue
{
	unsigned int hctx_idx;
	spinlock_t splock;
	wait_queue_head_t wait;
	unsigned int queue_depth;
	struct maddev_blk_dev *pmbdev;
	struct maddev_blk_io *pmdblkio;
	unsigned int requeue_selection;
};

struct maddev_blk_io
{
	struct maddev_blk_dev *pmdblkdev;
	struct list_head list;
	unsigned int index;
	struct request_queue *reqQ;
	struct blk_mq_tag_set *tag_set;
	struct blk_mq_tag_set __tag_set;
	unsigned int queue_depth;
	atomic_long_t cur_bytes;
	struct hrtimer bw_timer;
	unsigned long cache_flush_pos;
	spinlock_t lock;
	struct maddevb_queue *queues; //Possibly multiple hardware context(s) submit-complete Qs
	unsigned int nr_queues;
};

struct maddev_blk_dev
{
	struct maddev_blk_io *pmdblkio;
	char disk_name[DISK_NAME_LEN];
	struct gendisk *gdisk;
	struct block_device *bdev;
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
	bool         blocking; /* blocking blk-mq device */
	bool         use_per_node_hctx; /* use per-node allocation for hardware context */
	bool         power; /* power on/off the device */
	bool         memory_backed; /* if data is stored in memory */
	bool         discard; /* if support discard */
	bool         zoned; /* if device is zoned */
    void*        pmaddev;
	int          major;
	unsigned int minors;
};

static inline u64 mb_per_tick(int mbps)
{
	return (1 << 20) / TICKS_PER_SEC * ((u64) mbps);
}

////////////////////////////////////////////////////////////////////////////////
//Defines to accomodate a big batch of block layer API churn between 5.15 and ~6.12.
//
/* ---------- request timeout op signature ---------- */
/* Pre-6.12: timeout(struct request *) 
 * 6.12+:    timeout(struct request *, bool reserved) */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,12,0)
    #define MADDEV_TIMEOUT_DECL(name) enum blk_eh_timer_return name(struct request *rq)
    #define MADDEV_TIMEOUT_FIELD(name) .timeout = name
#else
    #define MADDEV_TIMEOUT_DECL(name) enum blk_eh_timer_return name(struct request *rq, bool reserved)
    #define MADDEV_TIMEOUT_FIELD(name) .timeout = name
#endif

/* ---------- open/release bdev ops prototypes ---------- */
/* Older: int (*open)(struct block_device*, fmode_t);
 * 6.12+: int (*open)(struct gendisk*, blk_mode_t) and
 *        void (*release)(struct gendisk*) */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6,6,999)
    #define MADDEV_OPEN_PROTO   int maddevb_open(struct block_device *bdev, fmode_t mode)
    #define MADDEV_RELEASE_PROTO void maddevb_release(struct gendisk *gd, fmode_t mode)
    #define MADDEVB_DEVOPS_OPEN_FIELD   .open = maddevb_open
    #define MADDEVB_DEVOPS_RELEASE_FIELD .release = maddevb_release
    #define MADDEVB_DEVOPS_RW_PAGE_FIELD  .rw_page = maddevb_rw_page
#endif

/* ---------- blk_mq_init_queue removed; use blk_mq_alloc_disk path ---------- */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,12,0)
    #define MAD_HAVE_MQ_INIT_QUEUE 1
#else
    #define MAD_HAVE_MQ_INIT_QUEUE 0
#endif

/* ---------- queue write cache set/get split ---------- */
//#if LINUX_VERSION_CODE < KERNEL_VERSION(6,12,0)
    #define maddevb_blk_queue_set_write_cache(q, wc, fua) blk_queue_write_cache((q),(wc),(fua))
//#else
//    #define maddevb_blk_queue_set_write_cache(q, wc, fua) blk_queue_set_write_cache((q),(wc),(fua))
//#endif

/* ---------- queue cleanup helpers changed ---------- */
//#if LINUX_VERSION_CODE < KERNEL_VERSION(6,12,0)
    #define maddevb_blk_cleanup_queue(q) blk_cleanup_queue((q))
//#else
//    #define maddevb_blk_cleanup_queue(q) blk_put_queue((q))
//#endif

/* ---------- ADD_RANDOM / DISCARD flags removals ---------- */
/* Just no-op them on new kernels; entropy and discard are automatic via limits */
//#if LINUX_VERSION_CODE < KERNEL_VERSION(6,12,0)
    #define mad_queue_flag_clear_add_random(q)  blk_queue_flag_clear(QUEUE_FLAG_ADD_RANDOM, (q))
    #define mad_queue_flag_set_discard(q)       blk_queue_flag_set(QUEUE_FLAG_DISCARD, (q))
//#else
//    #define mad_queue_flag_clear_add_random(q)  do { } while (0)
//    #define mad_queue_flag_set_discard(q)       do { } while (0)
//#endif

/* ---------- blk_mq_alloc_disk macro args changed ---------- */
/* Newer: blk_mq_alloc_disk(tag_set, limits, queuedata) (macro) */
//#if LINUX_VERSION_CODE < KERNEL_VERSION(6,12,0)
//static inline struct gendisk *
//maddevb_blk_mq_alloc_disk(struct blk_mq_tag_set *set, void *queuedata)
//{
//	struct gendisk *gd = blk_mq_alloc_disk(set, queuedata);
//	return gd;
//}
//#else
static inline struct gendisk *
maddevb_blk_mq_alloc_disk(struct blk_mq_tag_set *set, struct 
	                      request_queue *q, void *queuedata)
{
	#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,15,999)
	struct gendisk *gd = blk_mq_alloc_disk(set, &q->limits, queuedata);
	#endif
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,6,0)
	struct gendisk *gd = blk_mq_alloc_disk(set, queuedata);
	#endif

	return gd;
}
//#endif

/* ---------- blkdev_get_by_dev / blkdev_put replaced ---------- */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,15,999)
    #define maddevb_blkdev_get_by_dev(dev, mode, holder) \
	                blkdev_get_by_dev((dev), (mode), NULL)
    #define maddevb_blkdev_put(bdev, mode)        blkdev_put((bdev), (mode))
#else
//# include <linux/blkdev.h>
static inline struct block_device *
maddevb_blkdev_get_by_dev(dev_t dev, blk_mode_t mode,  void* holder)
{
	/* No holder ops -> NULL is fine if your driver doesn’t need one */
	//return bdev_open_by_dev(dev, mode, NULL, NULL);
	//struct file *filp = bdev_file_open_by_dev(dev, mode, NULL, NULL);
    struct bdev_handle *hbdev = bdev_open_by_dev(dev, mode, holder, NULL);
	if (hbdev == NULL)
		return NULL;
    else
	    return hbdev->bdev;
}
#define maddevb_blkdev_put(bdev, holder) blkdev_put(bdev, holder)
#endif
///////////////////////////////////////////////////////////////////////////////

int maddevb_init_module(void);
int maddevr_setup_cdev(PMADDEVOBJ pmaddev, int indx);
long maddevr_ioctl(struct file *fp, unsigned int cmd, unsigned long arg);
extern ssize_t maddevb_direct_io(struct file *filp, const char __user *buf, 
                                 size_t count, loff_t *f_pos, bool bWr);
extern int maddevb_create_blockdev(/*PMADDEVOBJ pmaddev*/void* pv);

int maddevb_gendisk_add(struct maddev_blk_dev *pmdblkdev);
blk_qc_t maddevb_queue_bio(struct request_queue *q, struct bio *bio);
void maddevb_restart_queue_async(struct maddev_blk_io *maddevb);
void maddevb_end_cmd(struct maddevb_cmd *cmd);
bool maddevb_should_requeue_request(struct request *rq);
bool maddevb_should_timeout_request(struct request *rq);
blk_status_t maddevb_handle_cmd(struct maddevb_cmd *cmd, sector_t sector, 
	                            sector_t nr_sectors, blk_opf_t op);
int maddevb_process_io_request(PMADDEVOBJ pmaddev, struct maddevb_cmd *cmd, 
                               sector_t sector, sector_t nr_sectors, blk_opf_t op);
void maddevb_blk_mq_req_work_fn(struct work_struct *blk_mq_req_work);
int maddevb_alloc_init_tag_set(struct maddev_blk_io *maddevb, struct blk_mq_tag_set *set);
int maddevb_init_driver_queues(struct maddev_blk_io *maddevb);
void maddevb_init_queues(struct maddev_blk_io *maddevb);
void maddevb_config_discard(struct maddev_blk_io *maddevb);
//int maddevb_setup_request_queue(struct maddev_blk_io *maddevb);
int maddevb_setup_queues(struct maddev_blk_io *pmdblkio);
void maddevb_init_request_queue(struct maddev_blk_io *pmdblkio);
struct maddevb_queue *maddevb_to_queue(struct maddev_blk_io *pmdblkio);
void maddevb_validate_conf(struct maddev_blk_dev *dev);
bool maddevb_setup_fault(void);
void maddevb_setup_bwtimer(struct maddev_blk_io *maddevb);
void maddevb_cleanup_queues(struct maddev_blk_io *maddevb);
extern void maddevb_delete_blockdev(struct maddev_blk_dev *pmdblkdev);
void maddevb_free_blockdev(struct maddev_blk_dev *dev);
int maddevb_ioctl(struct block_device* bdev, fmode_t mode, 
                  unsigned int cmd, unsigned long arg);
//int maddevb_system_ioctl(struct block_device* bdev, fmode_t mode,
//                          unsigned int cmd, unsigned long arg);
//int maddevb_collect_biovecs(struct mad_dev_obj *pmaddev, struct bio* pbio,
//                            u32 nr_sectors, struct bio_vec* pbiovecs);
//extern ssize_t 
//maddevb_xfer_sglist(PMADDEVOBJ pmaddev, struct bio_vec* biovecs,
//                    long nr_bvecs, sector_t sector, u32 nr_sectors, bool bWr);

//Set the dma go bit so that the device executes the i/o 
static inline void maddevb_init_io(struct mad_dev_obj *pmaddev)
{
	U32 flags = 0;
	__iomem MADREGS *pmadregs = pmaddev->pDevBase;
	maddev_acquire_lock_disable_ints(&pmaddev->devlock, &flags);
    U32 CntlReg = ioread32(&pmadregs->Control); 
	CntlReg |= MAD_CONTROL_DMA_GO_BIT;
    iowrite32(CntlReg, &pmadregs->Control);
	maddev_enable_ints_release_lock(&pmaddev->devlock, &flags);
}

int maddevb_init_sglist_io(PMADDEVOBJ pmaddev, struct scatterlist *sglist, 
	                       U32 sgl_size, sector_t tsector, sector_t nr_sectors,
						   bool bWr, bool bInitIo);

ssize_t maddevb_complete_sglist_io(PMADDEVOBJ pmaddev,
	                               struct maddevb_cmd *cmd);	

static inline void 
maddevb_free_cmd(PMADDEVOBJ pmaddev, struct maddevb_cmd *cmd)
{
	struct maddevb_queue *pmdbq = cmd->pmdbq;

	if ((pmdbq->pmbdev->irqmode == NULL_IRQ_TIMER) || 
        (cmd->cmdtimer.base != NULL))
	        hrtimer_cancel(&cmd->cmdtimer);

	kmem_cache_free(pmaddev->pcmd_cache, cmd);
	atomic_set(&pmaddev->batomic, 0);
	//blk_mq_start_hw_queue(hctx);
}

static inline void maddevb_abort_req(struct request *req)
{
	blk_mq_start_request(req);
    blk_mq_end_request(req, BLK_STS_IOERR);
}

static inline void maddevb_abort_cmd(PMADDEVOBJ pmaddev, struct maddevb_cmd *cmd)
{
	maddevb_abort_req(cmd->req);
	maddevb_free_cmd(pmaddev, cmd);
}

static inline void 
maddevb_init_cde_from_sgle(MAD_DMA_CHAIN_ELEMENT __iomem *pHwSgle, 
                           struct scatterlist *psgle, U32 DevDataOfst, 
						   U32 DmaCntl, U64 CDPP)
{
	dma_addr_t HostAddr = sg_dma_address(psgle);
	uint32_t   xferlen = (uint32_t)sg_dma_len(psgle);
    ASSERT((int)(xferlen != 0));
    ASSERT((int)((xferlen % MAD_SECTOR_SIZE) == 0));

    iowrite64(HostAddr,    &pHwSgle->HostAddr);
    iowrite32(DevDataOfst, &pHwSgle->DevDataOfst);
    iowrite32(DmaCntl,     &pHwSgle->DmaCntl); 
    iowrite32(xferlen,     &pHwSgle->DXBC);
    iowrite64(CDPP,        &pHwSgle->CDPP);

	#ifdef _MAD_SIMULATION_MODE_
	maddev_assign_hwsgle(pHwSgle, psgle);
    #endif
}
 
//extern ssize_t 
//maddevb_xfer_sgdma_bvecs(PMADDEVOBJ pmaddev, struct bio_vec* biovecs,
//                         long nr_bvecs, sector_t sector, u32 nr_sectors, bool bWr);
//ssize_t maddevb_xfer_sgdma_bvecs_completion(PMADDEVOBJ pmaddev, 
//                                            struct maddevb_cmd *cmd);	

#ifndef CONFIG_BLK_DEV_ZONED
//int maddevb_zone_init(struct maddev_blk_dev *dev);
//void maddevb_zone_exit(struct maddev_blk_dev *dev);
static inline int maddevb_zone_init(struct maddev_blk_dev *dev)
{
	PERR("CONFIG_BLK_DEV_ZONED not enabled\n");
	return -EINVAL;
}
#endif

static inline void* 
maddevb_get_parent_from_bdev(struct block_device *bdev)
{
    struct maddev_blk_dev *pmdblkdev = bdev->bd_disk->private_data;
    ASSERT((int)(pmdblkdev != NULL));
    ASSERT((int)(pmdblkdev->pmaddev != NULL));

    return pmdblkdev->pmaddev;
}

blk_status_t maddevb_handle_zoned(struct maddevb_cmd *cmd, blk_opf_t op,
	                             sector_t sector, sector_t nr_sectors);

irqreturn_t maddevb_isr_worker_fn(int irq, void* dev_id, int msinum);
irqreturn_t maddevb_msi_one_isr(int irq, void* dev_id);
irqreturn_t maddevb_msi_two_isr(int irq, void* dev_id);
irqreturn_t maddevb_msi_three_isr(int irq, void* dev_id);
irqreturn_t maddevb_msi_four_isr(int irq, void* dev_id);
irqreturn_t maddevb_msi_five_isr(int irq, void* dev_id);
irqreturn_t maddevb_msi_six_isr(int irq, void* dev_id);
irqreturn_t maddevb_msi_seven_isr(int irq, void* dev_id);
irqreturn_t maddevb_msi_eight_isr(int irq, void* dev_id);

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,15,999)				
int maddevb_open(struct block_device *bdev, fmode_t mode);
void maddevb_release(struct gendisk *disk, fmode_t mode);
int maddevb_rw_page(struct block_device *bdev, sector_t sector,
		            struct page *pPages, unsigned int op);
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,6,0)				
int maddevb_open(struct gendisk *gdisk, fmode_t mode);
void maddevb_release(struct gendisk *gdisk);
#endif

int maddevb_revalidate_disk(struct gendisk *disk);
int maddevb_getgeo(struct block_device* bdev, struct hd_geometry* geo);
void maddevb_swap_slot_free_notify(struct block_device* bdev, 
                                          unsigned long offset);
//static int maddevb_zone_report(struct gendisk *disk, sector_t sector,
//		                       struct blk_zone *zones, unsigned int *nr_zones);
int maddevb_zone_report(struct gendisk *disk, sector_t sector,
		                       /*struct blk_zone *zones,*/ 
							   unsigned int nr_zones, report_zones_cb cb, void* data);
extern irqreturn_t maddevb_legacy_isr(int irq, void* dev_id);

//void maddevb_dpc(struct work_struct *work);
extern void maddevb_dpctask(ulong indx);
void maddev_complete_io(struct work_struct *work);
ssize_t maddev_queued_read(struct kiocb *pkiocb, struct iov_iter *piov);
ssize_t maddev_queued_write(struct kiocb *pkiocb, struct iov_iter *piov);
void maddev_dpcwork_rd(struct work_struct *work);
void maddev_dpcwork_wr(struct work_struct *work);

enum hrtimer_restart maddevb_cmd_timer_expired(struct hrtimer *ptimer);

static inline void maddevb_start_cmd_timer(struct maddevb_cmd *cmd)
{
	ktime_t rq_tlimit = ktime_set(0, NSEC_PER_MSEC * 3000);
    hrtimer_init(&cmd->cmdtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	cmd->cmdtimer.function = maddevb_cmd_timer_expired;
	hrtimer_start(&cmd->cmdtimer, rq_tlimit, HRTIMER_MODE_REL);
}

//#ifndef HAVE_PAGE_ENDIO  /* assume 6.x without page_endio */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,15,0) && LINUX_VERSION_CODE <= KERNEL_VERSION(5,15,999)
    /* If your 5.15 headers still provide page_endio(), forward to it */
static inline void maddevb_page_endio(struct page *page, bool is_write, int err)
{
    page_endio(page, is_write, err);
}
#else
static inline void maddevb_page_endio(struct page *page, bool bWR, int err)
{
	struct folio *f = page_folio(page);
    struct address_space *mapping = folio_mapping(f);

    if (bWR)
	    {
        if (err && mapping)
            mapping_set_error(mapping, err);
        end_page_writeback(page);              /* completes writeback */
        }
	else
	    {
        if (!err)
            SetPageUptodate(page);
        else
            ClearPageUptodate(page);

        unlock_page(page);                      /* completes read */
     }
}
#endif

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

#if 0
//static inline void maddevb_zone_exit(struct maddev_blk_dev *dev) {}
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

#if 0 //int maddevb_zone_report(struct gendisk *disk, sector_t sector,
//		                struct blk_zone *zones, unsigned int *nr_zones);
//static blk_status_t 
//maddevb_handle_zoned(struct maddevblk_cmd *cmd,
//				       enum req_opf op, sector_t sector,
//				       sector_t nr_sectors);
#endif 

//
#ifdef _MAD_SIMULATION_MODE_
//Alias the pci functions to the simulator replacements
#include "sim_aliases.h"
#endif //_MAD_SIMULATION_MODE_
//
#endif /* _MADDEVB_H_ */
