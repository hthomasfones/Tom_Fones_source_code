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
/*  Module NAME : maddevb_blk_utils.c                                          */
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
/* HTF Consulting assumes no responsibility for errors or fitness of use       */
/*                                                                             */
/* This source file was derived from /drivers/block/null_blk.X                 */
/* No warranty is attached to the original source code.                        */
/*                                                                             */
/*                                                                             */
/* $Id: maddev_blk_utils.c, v 1.0 2021/01/01 00:00:00 htf $                    */
/*                                                                             */
/*******************************************************************************/

#include "maddevb.h"

static struct blk_mq_tag_set tag_set;
#ifdef CONFIG_BLK_DEV_NULL_BLK_FAULT_INJECTION
static DECLARE_FAULT_ATTR(null_timeout_attr);
static DECLARE_FAULT_ATTR(maddevb_requeue_attr);
#endif

/*
 * maddevb_page is a page in memory for maddevb devices.
 *
 * @page:	The page holding the data.
 * @bitmap:	The bitmap represents which sector in the page has data.
 *		Each bit represents one block size. For example, sector 8
 *		will use the 7th bit
 * The highest 2 bits of bitmap are for special purpose. LOCK means the cache
 * page is being flushing to storage. FREE means the cache page is freed and
 * should be skipped from flushing to storage. Please see
 * maddevb_io_make_cache_space
 */

extern struct mutex lock;
extern int maddevb_major;
extern struct ida maddevb_indexes;
extern char MadDevNames[10][20];
static LIST_HEAD(maddevb_list);
extern int reg_blkdev_major;

static void maddevb_free_blockdev_storage(struct maddev_blk_dev *dev, bool is_cache);
static struct maddev_blk_dev *maddevb_alloc_blockdev(PMADDEVOBJ pmaddev);
static int maddevb_add_blockdev(struct maddev_blk_dev *dev);
//static int maddevb_rw_page(struct block_device *bdev, sector_t sector,
//		                     struct page *page, unsigned int op);

static int maddevb_param_store_val(const char *str, int *val, int min, int max)
{
	int ret, new_val;

	ret = kstrtoint(str, 10, &new_val);
	if (ret)
		return -EINVAL;

	if (new_val < min || new_val > max)
		return -EINVAL;

	*val = new_val;
	return 0;
}

static int maddevb_set_queue_mode(const char *str, const struct kernel_param *kp)
{
	return maddevb_param_store_val(str, &g_queue_mode, MADDEVB_Q_BIO, MADDEVB_Q_MQ);
}

static const struct kernel_param_ops maddevb_queue_mode_param_ops = {
	.set	= maddevb_set_queue_mode,
	.get	= param_get_int,
};

device_param_cb(queue_mode, &maddevb_queue_mode_param_ops, &g_queue_mode, 0444);
MODULE_PARM_DESC(queue_mode, "Block interface to use (0=bio,1=rq,2=multiqueue)");

static int maddevb_set_irqmode(const char *str, const struct kernel_param *kp)
{
	return maddevb_param_store_val(str, &g_irqmode, NULL_IRQ_NONE,
					               NULL_IRQ_TIMER);
}

static const struct kernel_param_ops maddevb_irqmode_param_ops = {
	.set	= maddevb_set_irqmode,
	.get	= param_get_int,
};

static inline struct maddev_blk_dev *to_maddevb_device(struct config_item *item)
{
	return item ? container_of(item, struct maddev_blk_dev, item) : NULL;
}

static inline ssize_t maddevb_device_uint_attr_show(unsigned int val, char *page)
{
	return snprintf(page, PAGE_SIZE, "%u\n", val);
}

static inline ssize_t
maddevb_device_ulong_attr_show(unsigned long val, char *page)
{
	return snprintf(page, PAGE_SIZE, "%lu\n", val);
}

static inline ssize_t maddevb_device_bool_attr_show(bool val, char *page)
{
	return snprintf(page, PAGE_SIZE, "%u\n", val);
}

static ssize_t maddevb_device_uint_attr_store(unsigned int *val,
	const char *page, size_t count)
{
	unsigned int tmp;
	int result;

	result = kstrtouint(page, 0, &tmp);
	if (result)
		return result;

	*val = tmp;
	return count;
}

static ssize_t maddevb_device_ulong_attr_store(unsigned long *val,
	const char *page, size_t count)
{
	int result;
	unsigned long tmp;

	result = kstrtoul(page, 0, &tmp);
	if (result)
		return result;

	*val = tmp;
	return count;
}

static ssize_t 
maddevb_device_bool_attr_store(bool *val, const char *page, size_t count)
{
	bool tmp;
	int result;

	result = kstrtobool(page,  &tmp);
	if (result)
		return result;

	*val = tmp;
	return count;
}

/* The following macro should only be used with TYPE = {uint, ulong, bool}. */
#define MADDEVB_DEVICE_ATTR(NAME, TYPE)						\
static ssize_t									\
maddevb_device_##NAME##_show(struct config_item *item, char *page)		\
{										\
	return maddevb_device_##TYPE##_attr_show(					\
				to_maddevb_device(item)->NAME, page);		\
}										\
static ssize_t									\
maddevb_device_##NAME##_store(struct config_item *item, const char *page,	\
			                   size_t count)					\
{										\
	if (test_bit(MADDEVB_DEV_FL_CONFIGURED, &to_maddevb_device(item)->flags))\
		return -EBUSY;  \
	return maddevb_device_##TYPE##_attr_store(				\
			&to_maddevb_device(item)->NAME, page, count);		\
}	\
CONFIGFS_ATTR(maddevb_device_, NAME);

MADDEVB_DEVICE_ATTR(size, ulong);
MADDEVB_DEVICE_ATTR(completion_nsec, ulong);
MADDEVB_DEVICE_ATTR(submit_queues, uint);
MADDEVB_DEVICE_ATTR(home_node, uint);
MADDEVB_DEVICE_ATTR(queue_mode, uint);
MADDEVB_DEVICE_ATTR(blocksize, uint);
MADDEVB_DEVICE_ATTR(irqmode, uint);
MADDEVB_DEVICE_ATTR(hw_queue_depth, uint);
MADDEVB_DEVICE_ATTR(index, uint);
MADDEVB_DEVICE_ATTR(blocking, bool);
MADDEVB_DEVICE_ATTR(use_per_node_hctx, bool);
MADDEVB_DEVICE_ATTR(memory_backed, bool);
MADDEVB_DEVICE_ATTR(discard, bool);
MADDEVB_DEVICE_ATTR(mbps, uint);
MADDEVB_DEVICE_ATTR(cache_size, ulong);
#ifdef MADDEVB_ZONED
MADDEVB_DEVICE_ATTR(zoned, bool);
MADDEVB_DEVICE_ATTR(zone_size, ulong);
MADDEVB_DEVICE_ATTR(zone_nr_conv, uint);
#endif

#ifdef MADDEVB_POWER_MNGT
static ssize_t maddevb_device_power_show(struct config_item *item, char *page)
{
	return maddevb_device_bool_attr_show(to_maddevb_device(item)->power, page);
}

static ssize_t maddevb_device_power_store(struct config_item *item,
				                          const char *page, size_t count)
{
	struct maddev_blk_dev *dev = to_maddevb_device(item);
	bool newp = false;
	ssize_t ret;

	ret = maddevb_device_bool_attr_store(&newp, page, count);
	if (ret < 0)
		return ret;

	if (!dev->power && newp)
        {
		if (test_and_set_bit(MADDEVB_DEV_FL_UP, &dev->flags))
			return count;

        if (maddevb_add_blockdev(dev))
            {
			clear_bit(MADDEVB_DEV_FL_UP, &dev->flags);
			return -ENOMEM;
		    }

		set_bit(MADDEVB_DEV_FL_CONFIGURED, &dev->flags);
		dev->power = newp;
	    } 
    else
        {
        if (dev->power && !newp)
            {
            if (test_and_clear_bit(MADDEVB_DEV_FL_UP, &dev->flags))
                {
                mutex_lock(&lock);
                dev->power = newp;
                maddevb_delete_blockdev(dev->pmdblkio );
                mutex_unlock(&lock);
                }
            clear_bit(MADDEVB_DEV_FL_CONFIGURED, &dev->flags);
            }
        }

    return count;
}
//
CONFIGFS_ATTR(maddevb_device_, power);
#endif //MADDEVB_POWER_MNGT

#ifdef MADDEVB_BAD_BLOCK
static ssize_t maddevb_device_badblocks_show(struct config_item *item, char *page)
{
	struct maddev_blk_dev *t_dev = to_maddevb_device(item);

	return badblocks_show(&t_dev->badblocks, page, 0);
}

static ssize_t maddevb_device_badblocks_store(struct config_item *item,
				                              const char *page, size_t count)
{
	struct maddev_blk_dev *t_dev = to_maddevb_device(item);
	char *orig, *buf, *tmp;
	u64 start, end;
	int ret;

	orig = kstrndup(page, count, GFP_KERNEL);
	if (!orig)
		return -ENOMEM;

	buf = strstrip(orig);

	ret = -EINVAL;
	if (buf[0] != '+' && buf[0] != '-')
		goto out;

	tmp = strchr(&buf[1], '-');
	if (!tmp)
		goto out;

	*tmp = '\0';
	ret = kstrtoull(buf + 1, 0, &start);
	if (ret)
		goto out;

	ret = kstrtoull(tmp + 1, 0, &end);
	if (ret)
		goto out;

	ret = -EINVAL;
	if (start > end)
		goto out;

	/* enable badblocks */
	cmpxchg(&t_dev->badblocks.shift, -1, 0);
	if (buf[0] == '+')
		ret = badblocks_set(&t_dev->badblocks, start,
			end - start + 1, 1);
	else
		ret = badblocks_clear(&t_dev->badblocks, start,
			end - start + 1);

	if (ret == 0)
		ret = count;
out:
	kfree(orig);
	return ret;
}
//
CONFIGFS_ATTR(maddevb_device_, badblocks);
#endif //MADDEVB_BAD_BLOCK

static struct configfs_attribute *maddevb_device_attrs[] =
{
	&maddevb_device_attr_size,
	&maddevb_device_attr_completion_nsec,
	&maddevb_device_attr_submit_queues,
	&maddevb_device_attr_home_node,
	&maddevb_device_attr_queue_mode,
	&maddevb_device_attr_blocksize,
	&maddevb_device_attr_irqmode,
	&maddevb_device_attr_hw_queue_depth,
	&maddevb_device_attr_index,
	&maddevb_device_attr_blocking,
	&maddevb_device_attr_use_per_node_hctx,
    #ifdef MADDEVB_POWER_MNGT
	&maddevb_device_attr_power,
    #endif
	&maddevb_device_attr_memory_backed,
	&maddevb_device_attr_discard,
	&maddevb_device_attr_mbps,
	&maddevb_device_attr_cache_size,
    #ifdef MADDEVB_BAD_BLOCK
	&maddevb_device_attr_badblocks,
    #endif
    #ifdef MADDEVB_ZONED
	&maddevb_device_attr_zoned,
	&maddevb_device_attr_zone_size,
	&maddevb_device_attr_zone_nr_conv,
    #endif
	NULL,
};

void maddevb_free_blockdev(struct maddev_blk_dev *dev)
{
    PMADDEVOBJ pmaddev;

	if (!dev)
        {
        PERR("maddevb_free_blockdev... dev=NULL\n");
		return;
        }

    pmaddev = (PMADDEVOBJ)dev->pmaddev;

    #ifdef MADDEVB_ZONED
	maddevb_zone_exit(dev);
    #endif
    #ifdef MADDEVB_BAD_BLOCK
	badblocks_exit(&dev->badblocks);
    #endif
	kfree(dev);

    PINFO("maddevb_free_blockdev... dev#=%u dev=%px\n", pmaddev->devnum, dev);
}

static void maddevb_device_release(struct config_item *item)
{
	struct maddev_blk_dev *dev = to_maddevb_device(item);
    PMADDEVOBJ pmaddev = (PMADDEVOBJ)dev->pmaddev;

    PINFO("maddevb_device_release... dev#=%d dev=%px item=%px\n",
          (int)pmaddev->devnum, dev, item);
	maddevb_free_blockdev_storage(dev, false);
	maddevb_free_blockdev(dev);
}

static struct configfs_item_operations maddevb_device_ops =
{
	.release = maddevb_device_release,
};

static const struct config_item_type maddevb_device_type =
{
	.ct_item_ops	= &maddevb_device_ops,
	.ct_attrs	= maddevb_device_attrs,
	.ct_owner	= THIS_MODULE,
};

static void
maddevb_group_drop_item(struct config_group *group, struct config_item *item)
{
	struct maddev_blk_dev *pmdbdev = to_maddevb_device(item);

	if (test_and_clear_bit(MADDEVB_DEV_FL_UP, &pmdbdev->flags)) 
        {
		mutex_lock(&lock);
		pmdbdev->power = false;
		maddevb_delete_blockdev(pmdbdev);
		mutex_unlock(&lock);
	    }

	//config_item_put(item);
}

static ssize_t memb_group_features_show(struct config_item *item, char *page)
{
	return snprintf(page, PAGE_SIZE,
                    "memory_backed,discard,bandwidth,cache,badblocks,zoned,zone_size\n");
}
//
CONFIGFS_ATTR_RO(memb_group_, features);

static struct configfs_attribute *maddevb_group_attrs[] = 
{
	&memb_group_attr_features,
	NULL,
};

static struct configfs_group_operations maddevb_group_ops = 
{
	//.make_item	= maddevb_group_make_item,
	.drop_item	= maddevb_group_drop_item,
};

static const struct config_item_type maddevb_group_type = 
{
	.ct_group_ops	= &maddevb_group_ops,
	.ct_attrs	= maddevb_group_attrs,
	.ct_owner	= THIS_MODULE,
};

static inline int maddevb_io_cache_active(struct maddev_blk_io *pmdblkio)
{
	return test_bit(MADDEVB_DEV_FL_CACHE, &pmdblkio->pmdblkdev->flags);
}

//static enum hrtimer_restart maddevb_cmd_timer_expired(struct hrtimer *timer);

void maddevb_end_cmd(struct maddevb_cmd *cmd)
{
	struct request* req = cmd->req;
	blk_status_t cmderr = cmd->error;
	ssize_t iosize = cmd->iosize;
	size_t req_size = blk_rq_bytes(cmd->req);
	//(blk_rq_sectors(req) * LINUX_LOGICAL_SECTOR_SIZE);
	struct maddevb_queue *pmdbq = cmd->pmdbq;
	struct mad_dev_obj *pmaddev = 
		               (struct mad_dev_obj *)pmdbq->pmbdev->pmaddev;
    //enum req_opf op;
    //enum dma_data_direction dmadir;
	
	if (cmd->sglist != NULL)
		{
		//op = req_op(req);
		//dmadir = (op == REQ_OP_WRITE) ? DMA_TO_DEVICE : DMA_FROM_DEVICE;
		dma_unmap_sg(pmaddev->pdevnode, 
			         cmd->sglist, cmd->dma_size, cmd->dmadir);
		cmd->sglist = NULL;
		}
	
	if (cmd->req != NULL)	
    	{
        blk_mq_end_request(req, cmd->error);
		cmd->req = POISON_VIRT_ADDR;
		ASSERT((int)(iosize == req_size));
		//if (iosize != req_size)
        //    {PWARN("maddevb_end_cmd... dev#=%d iosize(%lu) != req_size(%lu) *!*\n",
        //          (int)pmaddev->devnum, (ulong)iosize, (ulong)req_size);}
    	}

	PINFO("maddevb_end_cmd... dev#=%d cmd=%px req=%px iosize=%lu err=%d\n\n",
          (int)pmaddev->devnum, cmd, req, iosize, cmderr);
	
	maddevb_free_cmd(pmaddev, cmd);
}

static struct maddevb_page *maddevb_alloc_page(gfp_t gfp_flags)
{
	struct maddevb_page *t_page;

	t_page = kmalloc(sizeof(struct maddevb_page), gfp_flags);
	if (!t_page)
		goto out;

	t_page->page = alloc_pages(gfp_flags, 0);
	if (!t_page->page)
		goto out_freepage;

	memset(t_page->bitmap, 0, sizeof(t_page->bitmap));
	return t_page;

out_freepage:
	kfree(t_page);
out:
	return NULL;
}

static void maddevb_free_page(struct maddevb_page *t_page)
{
	__set_bit(MADDEVB_PAGE_FREE, t_page->bitmap);
	if (test_bit(MADDEVB_PAGE_LOCK, t_page->bitmap))
		return;

	__free_page(t_page->page);
	kfree(t_page);
}

static bool maddevb_page_empty(struct maddevb_page *page)
{
	int size = MAP_SZ - 2;

	return find_first_bit(page->bitmap, size) == size;
}

static void 
maddevb_free_sector(struct maddev_blk_io *pmdblkio, sector_t sector, bool is_cache)
{
	unsigned int sector_bit;
	u64 idx;
	struct maddevb_page *t_page, *ret;
	struct radix_tree_root *root;

	root = is_cache ? &pmdblkio->pmdblkdev->cache : &pmdblkio->pmdblkdev->data;
	idx = sector >> PAGE_SECTORS_SHIFT;
	sector_bit = (sector & SECTOR_MASK);

	t_page = radix_tree_lookup(root, idx);
	if (t_page)
        {
		__clear_bit(sector_bit, t_page->bitmap);

		if (maddevb_page_empty(t_page)) 
            {
			ret = radix_tree_delete_item(root, idx, t_page);
			WARN_ON_MACRO(ret != t_page);
			maddevb_free_page(ret);
			if (is_cache)
				pmdblkio->pmdblkdev->curr_cache -= PAGE_SIZE;
		    }
	    }
}

static struct maddevb_page*
maddevb_radix_tree_insert(struct maddev_blk_io *pmdblkio, u64 idx, 
                          struct maddevb_page *t_page, bool is_cache)
{
	struct radix_tree_root *root;

	root = is_cache ? &pmdblkio->pmdblkdev->cache : &pmdblkio->pmdblkdev->data;

	if (radix_tree_insert(root, idx, t_page)) 
    {
		maddevb_free_page(t_page);
		t_page = radix_tree_lookup(root, idx);
		WARN_ON_MACRO(!t_page || t_page->page->index != idx);
	}
    else if (is_cache)
		pmdblkio->pmdblkdev->curr_cache += PAGE_SIZE;

	return t_page;
}

static void 
maddevb_free_blockdev_storage(struct maddev_blk_dev *pmdblkdev, bool is_cache)
{
	unsigned long pos = 0;
	int nr_pages;
	struct maddevb_page *ret, *t_pages[FREE_BATCH];
	struct radix_tree_root *root;
    PMADDEVOBJ pmaddev = (PMADDEVOBJ)pmdblkdev->pmaddev;

	root = is_cache ? &pmdblkdev->cache : &pmdblkdev->data;

    PINFO("maddevb_free_blockdev_storage... dev#=%d pmdblkdev=%px\n",
          (int)pmaddev->devnum, pmdblkdev);

	do {
		int i;

		nr_pages = radix_tree_gang_lookup(root,
				(void **)t_pages, pos, FREE_BATCH);

		for (i = 0; i < nr_pages; i++)
            {
			pos = t_pages[i]->page->index;
			ret = radix_tree_delete_item(root, pos, t_pages[i]);
			WARN_ON_MACRO(ret != t_pages[i]);
			maddevb_free_page(ret);
		    }

		pos++;
	    } while (nr_pages == FREE_BATCH);

	if (is_cache)
		pmdblkdev->curr_cache = 0;
}

static 
struct maddevb_page *__lookup_page(struct maddev_blk_io *pmdblkio,
                                   sector_t sector, bool for_write, bool is_cache)
{
	unsigned int sector_bit;
	u64 idx;
	struct maddevb_page *t_page;
	struct radix_tree_root *root;

	idx = sector >> PAGE_SECTORS_SHIFT;
	sector_bit = (sector & SECTOR_MASK);

	root = is_cache ? &pmdblkio->pmdblkdev->cache : &pmdblkio->pmdblkdev->data;
	t_page = radix_tree_lookup(root, idx);
	WARN_ON_MACRO(t_page && t_page->page->index != idx);

	if (t_page && (for_write || test_bit(sector_bit, t_page->bitmap)))
		return t_page;

	return NULL;
}

static struct maddevb_page *maddevb_lookup_page(struct maddev_blk_io *pmdblkio,
	sector_t sector, bool for_write, bool ignore_cache)
{
	struct maddevb_page *page = NULL;

	if (!ignore_cache)
		page = __lookup_page(pmdblkio, sector, for_write, true);

    if (page)
		return page;

	return __lookup_page(pmdblkio, sector, for_write, false);
}

static struct maddevb_page *maddevb_insert_page(struct maddev_blk_io *pmdblkio,
	                                                   sector_t sector, bool ignore_cache)
	__releases(&pmdblkio->lock)
	__acquires(&pmdblkio->lock)
{
	u64 idx;
	struct maddevb_page *t_page;

	t_page = maddevb_lookup_page(pmdblkio, sector, true, ignore_cache);
	if (t_page)
		return t_page;

	spin_unlock_irq(&pmdblkio->lock);

	t_page = maddevb_alloc_page(GFP_NOIO);
	if (!t_page)
		goto out_lock;

	if (radix_tree_preload(GFP_NOIO))
		goto out_freepage;

	spin_lock_irq(&pmdblkio->lock);
	idx = sector >> PAGE_SECTORS_SHIFT;
	t_page->page->index = idx;
	t_page = maddevb_radix_tree_insert(pmdblkio, idx, t_page, !ignore_cache);
	radix_tree_preload_end();

	return t_page;

out_freepage:
	maddevb_free_page(t_page);
out_lock:
	spin_lock_irq(&pmdblkio->lock);
	return maddevb_lookup_page(pmdblkio, sector, true, ignore_cache);
}

static int maddevb_flush_cache_page(struct maddev_blk_io *pmdblkio, 
	                                struct maddevb_page *c_page)
{
	int i;
	unsigned int offset;
	u64 idx;
	struct maddevb_page *t_page, *ret;
	void *dst, *src;

	idx = c_page->page->index;

	t_page = maddevb_insert_page(pmdblkio, idx << PAGE_SECTORS_SHIFT, true);

	__clear_bit(MADDEVB_PAGE_LOCK, c_page->bitmap);
	if (test_bit(MADDEVB_PAGE_FREE, c_page->bitmap))
        {
		maddevb_free_page(c_page);
		if (t_page && maddevb_page_empty(t_page)) 
            {
			ret = 
			radix_tree_delete_item(&pmdblkio->pmdblkdev->data, idx, t_page);
			maddevb_free_page(t_page);
		    }

		return 0;
	    }

	if (!t_page)
        {
        PERR("maddevb_flush_cache_page... pmdblkio=%px rc=-ENOMEM\n", pmdblkio);
		return -ENOMEM;
        }

	src = kmap_atomic(c_page->page);
	dst = kmap_atomic(t_page->page);

	for (i = 0; i < PAGE_SECTORS;
		 i += ((pmdblkio->pmdblkdev->blocksize >> MAD_SECTOR_SHIFT)))
        {
		if (test_bit(i, c_page->bitmap))
            {
			offset = (i << MAD_SECTOR_SHIFT);
			memcpy(dst + offset, src + offset,
				pmdblkio->pmdblkdev->blocksize);
			__set_bit(i, t_page->bitmap);
		    }
	    }

	kunmap_atomic(dst);
	kunmap_atomic(src);

	ret = radix_tree_delete_item(&pmdblkio->pmdblkdev->cache, idx, c_page);
	maddevb_free_page(ret);
	pmdblkio->pmdblkdev->curr_cache -= PAGE_SIZE;

	return 0;
}

static int maddevb_make_cache_space(struct maddev_blk_io *pmdblkio, unsigned long n)
{
	int i, err, nr_pages;
	struct maddevb_page *c_pages[FREE_BATCH];
	unsigned long flushed = 0, one_round;

again:
	if ((pmdblkio->pmdblkdev->cache_size * 1024 * 1024) >
	     pmdblkio->pmdblkdev->curr_cache + n || pmdblkio->pmdblkdev->curr_cache == 0)
		    return 0;

	nr_pages = 
    radix_tree_gang_lookup(&pmdblkio->pmdblkdev->cache, (void **)c_pages,
                           pmdblkio->cache_flush_pos, FREE_BATCH);
	/*
	 * pmdblkio_flush_cache_page could unlock before using the c_pages. To
	 * avoid race, we don't allow page free
	 */
	for (i = 0; i < nr_pages; i++)
        {
		pmdblkio->cache_flush_pos = c_pages[i]->page->index;
		/*
		 * We found the page which is being flushed to disk by other
		 * threads
		 */
		if (test_bit(MADDEVB_PAGE_LOCK, c_pages[i]->bitmap))
			c_pages[i] = NULL;
		else
			__set_bit(MADDEVB_PAGE_LOCK, c_pages[i]->bitmap);
	    }

	one_round = 0;
	for (i = 0; i < nr_pages; i++)
        {
		if (c_pages[i] == NULL)
			continue;

		err = maddevb_flush_cache_page(pmdblkio, c_pages[i]);
		if (err)
			return err;

		one_round++;
	    }

	flushed += one_round << PAGE_SHIFT;

	if (n > flushed)
        {
		if (nr_pages == 0)
			pmdblkio->cache_flush_pos = 0;
		if (one_round == 0)
            {
			/* give other threads a chance */
			spin_unlock_irq(&pmdblkio->lock);
			spin_lock_irq(&pmdblkio->lock);
		    }

		goto again;
	    }

	return 0;
}

static int 
maddevb_write_waited(struct maddev_blk_io *pmdblkio, struct page *source,
	                 unsigned int off, sector_t sector, 
                     size_t len, bool is_fua)
{
	size_t temp, count = 0;
	unsigned int offset;
	struct maddevb_page *t_page;
	void *dst, *src;

    PINFO("maddevb_write_waited... sector=%ld len=%ld\n",
          (long int)sector, (long int)len);

	while (count < len)
        {
		temp = min_t(size_t, pmdblkio->pmdblkdev->blocksize, len - count);

		if (maddevb_io_cache_active(pmdblkio) && !is_fua)
			maddevb_make_cache_space(pmdblkio, PAGE_SIZE);

		offset = ((sector & SECTOR_MASK) << MAD_SECTOR_SHIFT);
		t_page = maddevb_insert_page(pmdblkio, sector,
			!maddevb_io_cache_active(pmdblkio) || is_fua);
		if (!t_page)
            {
            PERR("maddevb_write_waited... pmdblkio=%px rc=-ENOSPC\n", 
				 pmdblkio);
            return -ENOSPC;
            }

		src = kmap_atomic(source);
		dst = kmap_atomic(t_page->page);
		memcpy(dst + offset, src + off + count, temp);
		kunmap_atomic(dst);
		kunmap_atomic(src);

		__set_bit(sector & SECTOR_MASK, t_page->bitmap);

		if (is_fua)
			maddevb_free_sector(pmdblkio, sector, true);

		count += temp;
		sector += (temp >> MAD_SECTOR_SHIFT);
	    }

	return 0;
}

static int 
maddevb_read_waited(struct maddev_blk_io *pmdblkio, struct page *dest,
	                unsigned int off, sector_t sector, size_t len)
{
	size_t temp, count = 0;
	unsigned int offset;
	struct maddevb_page *t_page;
	void *dst, *src;

    PINFO("maddevb_read_waited... sector=%ld len=%ld\n", (long int)sector,
          (long int)len);

	while (count < len)
        {
		temp = min_t(size_t, pmdblkio->pmdblkdev->blocksize, len - count);

		offset = ((sector & SECTOR_MASK) << MAD_SECTOR_SHIFT);
		t_page = maddevb_lookup_page(pmdblkio, sector, false,
			                         !maddevb_io_cache_active(pmdblkio));

		dst = kmap_atomic(dest);
		if (!t_page)
            {
			memset(dst + off + count, 0, temp);
			goto next;
		    }

		src = kmap_atomic(t_page->page);
		memcpy(dst + off + count, src + offset, temp);
		kunmap_atomic(src);
next:
		kunmap_atomic(dst);

		count += temp;
		sector += (temp >> MAD_SECTOR_SHIFT);
	    }

    return 0;
}

static void maddevb_handle_discard(struct maddev_blk_io *pmdblkio, 
                                   sector_t sector, size_t n)
{
	size_t temp;

	spin_lock_irq(&pmdblkio->lock);
	while (n > 0) 
        {
		temp = min_t(size_t, n, pmdblkio->pmdblkdev->blocksize);
		maddevb_free_sector(pmdblkio, sector, false);
		if (maddevb_io_cache_active(pmdblkio))
			maddevb_free_sector(pmdblkio, sector, true);
		sector += (temp >> MAD_SECTOR_SHIFT);
		n -= temp;
	    }

	spin_unlock_irq(&pmdblkio->lock);
}

static int maddevb_handle_flush(struct maddev_blk_io *pmdblkio)
{
    struct mad_dev_obj *pmaddev = 
                       (struct mad_dev_obj *)pmdblkio->pmdblkdev->pmaddev;
	int err = 0;

	if (!maddevb_io_cache_active(pmdblkio))
		return 0;

	spin_lock_irq(&pmdblkio->lock);
	while (true)
        {
		err = maddevb_make_cache_space(pmdblkio,
			                           pmdblkio->pmdblkdev->cache_size * 1024 * 1024);
		if (err || pmdblkio->pmdblkdev->curr_cache == 0)
			break;
	    }

	WARN_ON_MACRO(!radix_tree_empty(&pmdblkio->pmdblkdev->cache));
	spin_unlock_irq(&pmdblkio->lock);

    if (err != 0)
        {PERR("maddevb_handle_flush... dev=%d err=%d\n",
              (int)pmaddev->devnum, err);}
	return err;
}

static int 
maddevb_transfer(struct maddev_blk_io *pmdblkio, struct page *page,
	             unsigned int len, unsigned int off, bool is_write,
                 sector_t sector, bool is_fua)
{
    struct mad_dev_obj *pmaddev = 
                       (struct mad_dev_obj *)pmdblkio->pmdblkdev->pmaddev;
    int err = 0;

	if (!is_write)
        {
		err = maddevb_read_waited(pmdblkio, page, off, sector, len);
		flush_dcache_page(page);
	    }
    else
        {
		flush_dcache_page(page);
		err = maddevb_write_waited(pmdblkio, page, off, sector, len, is_fua);
	    }

    if (err != 0)
        {PERR("maddevb_transfer... dev#=%d wr=%d err=%d\n", 
              (int)pmaddev->devnum, is_write, (int)err);}

	return err;
}

static int maddevb_handle_req(struct maddevb_cmd *cmd)
{
	struct request *req = cmd->req;
    sector_t sector = blk_rq_pos(req);
    struct maddev_blk_io *pmdblkio = cmd->pmdbq->pmbdev->pmdblkio ;
    struct mad_dev_obj *pmaddev = 
                       (struct mad_dev_obj*)cmd->pmdbq->pmbdev->pmaddev;

    int err;
	unsigned int len;
	struct req_iterator iter;
	struct bio_vec bvec;

	PINFO("maddevb_handle_req dev#=%d cmd=%px req=%px sector=%ld\n", 
          (int)pmaddev->devnum, (void *)cmd, (void *)req, (long)sector);

	if (req_op(req) == REQ_OP_DISCARD)
        {
		maddevb_handle_discard(pmdblkio , sector, blk_rq_bytes(req));
		return 0;
	    }

	spin_lock_irq(&pmdblkio->lock);
	rq_for_each_segment(bvec, req, iter)
        {
		len = bvec.bv_len;
		err = maddevb_transfer(pmdblkio , bvec.bv_page, len, bvec.bv_offset,
				               op_is_write(req_op(req)), sector,
				               req->cmd_flags & REQ_FUA);
		if (err)
            {
			spin_unlock_irq(&pmdblkio ->lock);
			//return err;
            break;
		    }
		sector += (len >> MAD_SECTOR_SHIFT);
	    }
	spin_unlock_irq(&pmdblkio->lock);

    if (err != 0)
        {PERR("maddevb_handle_req... dev#=%d req=%px err=%d\n", 
              (int)pmaddev->devnum, (void *)req, err);}

	return err;
}

static int maddevb_handle_bio(struct maddevb_cmd *cmd)
{
	struct bio *bio = cmd->bio;
	struct maddev_blk_io *pmdblkio = cmd->pmdbq->pmbdev->pmdblkio ;
	int err;
	unsigned int len;
	sector_t sector;
	struct bio_vec bvec;
	struct bvec_iter iter;

	sector = bio->bi_iter.bi_sector;

	if (bio_op(bio) == REQ_OP_DISCARD)
        {
		maddevb_handle_discard(pmdblkio , sector,
			                   (bio_sectors(bio) << MAD_SECTOR_SHIFT));
		return 0;
	    }

	spin_lock_irq(&pmdblkio->lock);
	bio_for_each_segment(bvec, bio, iter)
        {
		len = bvec.bv_len;
		err = maddevb_transfer(pmdblkio, bvec.bv_page, len, bvec.bv_offset,
				               op_is_write(bio_op(bio)), sector,
				               bio->bi_opf & REQ_FUA);
		if (err)
            {
			spin_unlock_irq(&pmdblkio->lock);
			return err;
		    }
		sector += (len >> MAD_SECTOR_SHIFT);
	    }
	spin_unlock_irq(&pmdblkio ->lock);
	return 0;
}

static void maddevb_stop_queue(struct maddev_blk_io *pmdblkio)
{
	struct request_queue *reqQ = pmdblkio ->reqQ;

	if (pmdblkio ->pmdblkdev->queue_mode == MADDEVB_Q_MQ)
		blk_mq_stop_hw_queues(reqQ);
}

void maddevb_restart_queue_async(struct maddev_blk_io *pmdblkio)
{
	struct request_queue *reqQ = pmdblkio->reqQ;

	if (pmdblkio->pmdblkdev->queue_mode == MADDEVB_Q_MQ)
		blk_mq_start_stopped_hw_queues(reqQ, true);
}

static inline blk_status_t maddevb_handle_throttled(struct maddevb_cmd *cmd)
{
	struct maddev_blk_dev *pmdblkdev = cmd->pmdbq->pmbdev;
	struct maddev_blk_io *pmdblkio = pmdblkdev->pmdblkio ;
	blk_status_t sts = BLK_STS_OK;
	struct request *req = cmd->req;

	if (!hrtimer_active(&pmdblkio->bw_timer))
		hrtimer_restart(&pmdblkio->bw_timer);

	if (atomic_long_sub_return(blk_rq_bytes(req), &pmdblkio->cur_bytes) < 0)
        {
		maddevb_stop_queue(pmdblkio);
		/* race with timer */
		if (atomic_long_read(&pmdblkio->cur_bytes) > 0)
			maddevb_restart_queue_async(pmdblkio);
		/* requeue request */
		sts = BLK_STS_DEV_RESOURCE;
	    }
	return sts;
}

#ifdef MADDEVB_BAD_BLOCK
static inline blk_status_t pmdblkio_handle_badblocks(struct pmdblkio_cmd *cmd,
						                            sector_t sector,
													sector_t nr_sectors)
{
	struct badblocks *bb = &cmd->nq->dev->badblocks;
	sector_t first_bad;
	int bad_sectors;

	if (badblocks_check(bb, sector, nr_sectors, &first_bad, &bad_sectors))
		return BLK_STS_IOERR;

	return BLK_STS_OK;
}
#endif

static inline blk_status_t
maddevb_handle_memory_backed(struct maddevb_cmd *cmd, /*enum req_opf op*/ int op)
{
	struct maddev_blk_dev *pmdblkdev = cmd->pmdbq->pmbdev;
    struct mad_dev_obj *pmaddev = (struct mad_dev_obj *)pmdblkdev->pmaddev;
    int err = 0;

	if (pmdblkdev->queue_mode == MADDEVB_Q_BIO)
		err = maddevb_handle_bio(cmd);
	else
		err = maddevb_handle_req(cmd);

    if (err != 0)
        {PERR("maddevb_handle_memory_backed... dev#=%d req=%px err=%d\n", 
              (int)pmaddev->devnum, cmd->req, (int)err);}

	return errno_to_blk_status(err);
}

int maddevb_process_io_request(PMADDEVOBJ pmaddev, struct maddevb_cmd *cmd, 
                               sector_t sector, sector_t nr_sectors, blk_opf_t op)
{
    sector_t tsector = sector;
	bool     bH2D = (op==REQ_OP_WRITE);
	cmd->dmadir = (bH2D) ? DMA_TO_DEVICE : DMA_FROM_DEVICE;
	cmd->sglist = pmaddev->sglist;
	U32 num_psegs = 0;
    int sgl_size = 0, dma_map_size = 0;
    int  iorc = 0;
    num_psegs = blk_rq_nr_phys_segments(cmd->req);

    PINFO("maddevb_process_io_request... \n    \
		  dev#=%d cmd=%px req=%px op=%d sector=%llu #sctrs=%u num_psegs=%u\n",
          (int)pmaddev->devnum, cmd, cmd->req, op, 
		  tsector, (uint32_t)nr_sectors, (uint32_t)num_psegs);

    sg_init_table(cmd->sglist, num_psegs);
	sgl_size = blk_rq_map_sg(cmd->req->q, cmd->req, cmd->sglist);
    if ((sgl_size < 1) || (sgl_size > num_psegs))
        {
		iorc = -EINVAL;
		{PERR("maddevb_process_io_request... invalid sg_list dev#=%d sgl_size=%d iorc=%d\n",
              (int)pmaddev->devnum, sgl_size, iorc);}
        return iorc;
        } 

    dma_map_size = 
	dma_map_sg(pmaddev->pdevnode, cmd->sglist, sgl_size, cmd->dmadir);
	if (dma_map_size < 0)
	    {iorc = -ENOBUFS;}
	else
        {
		if (dma_map_size != sgl_size)
	        {PWARN("maddevb_process_io_request:dma_map_sg... dev#=%d sglsz=%d dmasz=%d !\n",
                   (int)pmaddev->devnum, sgl_size, dma_map_size);}  

		iorc = maddevb_init_sglist_io(pmaddev, cmd->sglist, dma_map_size, 
		                              tsector, nr_sectors, bH2D, false);
		}										 
    
	cmd->iosize = 0;
	cmd->dma_size = (iorc == 0) ? dma_map_size : 0;
    cmd->sglist   = (iorc == 0) ? cmd->sglist : NULL;

    if (iorc != 0)
        {PERR("maddevb_process_io_request... dev#=%u iorc=%d\n",
              pmaddev->devnum, iorc);}

    return iorc;
}

blk_status_t maddevb_handle_cmd(struct maddevb_cmd *cmd, sector_t sector,
                                sector_t nr_sectors, blk_opf_t op)
{
	struct maddev_blk_dev *pmbdev = cmd->pmdbq->pmbdev;
	struct mad_dev_obj *pmaddev = (struct mad_dev_obj *)pmbdev->pmaddev;
	struct maddev_blk_io *pmdblkio = pmbdev->pmdblkio;
	blk_status_t sts = BLK_STS_OK;
    int err = -1;
	
	PINFO("maddevb_handle_cmd... dev#=%d cmd=%px req=%px sector=%u #sctrs=%u op=%d\n",
          (int)pmaddev->devnum, (void *)cmd, (void *)cmd->req, 
		  (uint32_t)sector, (uint32_t)nr_sectors, op);

	if (test_bit(MADDEVB_DEV_FL_THROTTLED, &pmbdev->flags))
        {
		maddevb_abort_cmd(pmaddev, cmd);
		return BLK_STS_OK;
		}

    #ifdef MADDEVB_BAD_BLOCK
	if (pmbdev->badblocks.shift != -1)
        {
		sts = maddevb_handle_badblocks(cmd, sector, nr_sectors);
	    if (sts != BLK_STS_OK)
		    {
     		maddevb_abort_cmd(pmaddev, cmd);
            return BLK_STS_OK;
            }
		}
    #endif

	if (pmbdev->memory_backed)
        {
		sts = maddevb_handle_memory_backed(cmd, op);
	    if (sts != BLK_STS_OK)
		    {
  			maddevb_abort_cmd(pmaddev, cmd);
            return BLK_STS_OK;
            }
	    }

    #ifdef MADDEVB_ZONED
	if (!cmd->error && pmbdev->zoned) 
        {
		sts = maddevb_handle_zoned(cmd, op, sector, nr_sectors);
	    if (sts != BLK_STS_OK) 
		    {
   			maddevb_abort_cmd(pmaddev, cmd);
            return BLK_STS_OK;
            }
	    }
    #endif

	if (sts != BLK_STS_OK)
	    {
		maddevb_abort_cmd(pmaddev, cmd);
	    return BLK_STS_OK; //Because we terminated the command
	    }
	
	blk_mq_start_request(cmd->req);

    switch (op)
        {
        case REQ_OP_READ:
        case REQ_OP_WRITE:
            err = 
			maddevb_process_io_request(pmaddev, cmd, sector, nr_sectors, op);
            cmd->error = sts = errno_to_blk_status(err);
            break;

        case REQ_OP_FLUSH:
            err = maddevb_handle_flush(pmdblkio );
            cmd->error = sts = errno_to_blk_status(err);
            break;

        default:
            PWARN("maddevb_handle_cmd... dev#=%d cmd=%px req=%px op=%d sts=BLK_STS_NOTSUPP\n",
                  (int)pmaddev->devnum, (void *)cmd, (void *)cmd->req, op);
            cmd->error = sts = BLK_STS_NOTSUPP;
        }

	if (WARN_ON_ONCE(sts == BLK_STS_RESOURCE || sts == BLK_STS_AGAIN || 
		sts == BLK_STS_DEV_RESOURCE))
		    {
			blk_mq_end_request(cmd->req, BLK_STS_IOERR);
			maddevb_free_cmd(pmaddev, cmd);
            return BLK_STS_OK;
			}

    if (sts != BLK_STS_OK)
        {
		PERR("maddevb_handle_cmd... dev#=%u cmd=%px err=%d blk_sts=%d\n",
             pmaddev->devnum, cmd, err, sts);
        blk_mq_end_request(cmd->req, cmd->error);
		maddevb_free_cmd(pmaddev, cmd);
		return BLK_STS_OK; //Because we terminated the command
		}
	else
		{
	    //We are in the queue_request execution path so we need to schedule a work item.
        //"Calling blk_mq_end_request() synchronously from within queue_rq() can lead to 
	    // stack corruption, plug bugs, and preempt count mismatches." - per the blk-mq maintainers
	    INIT_WORK(&cmd->blk_mq_req_work, maddevb_blk_mq_req_work_fn);
	    queue_work(system_wq, &cmd->blk_mq_req_work);
		return BLK_STS_OK;
		}
}

void maddevb_blk_mq_req_work_fn(struct work_struct *pBlkMqReq)
{
 	struct maddevb_cmd *cmd = container_of(pBlkMqReq, 
		                                   struct maddevb_cmd, blk_mq_req_work);
	struct maddev_blk_dev *pmbdev = cmd->pmdbq->pmbdev;
	struct mad_dev_obj *pmaddev = (struct mad_dev_obj *)pmbdev->pmaddev;
	ssize_t iocount = 0; 
    
	PINFO("maddevb_blk_mq_req_work_fn... dev#=%d cmd=%px req=%px\n",
          (int)pmaddev->devnum, (void *)cmd, (void *)cmd->req);
    
    //ASSERT((int)(VIRT_ADDR_VALID(cmd->req, sizeof(struct request))));
    ASSERT((int)(blk_mq_request_started(cmd->req)));
    WARN_ON_MACRO(in_atomic());
    WARN_ON_MACRO(preempt_count() != 0);

	//Kick off the i/o here in this work item
	maddevb_init_io(pmaddev);

    iocount = maddevb_complete_sglist_io(pmaddev, cmd);	
    cmd->error = (iocount < 0) ? errno_to_blk_status(iocount) : BLK_STS_OK;
    cmd->iosize = (iocount < 0) ? 0 : iocount;
	
	if (cmd->error != BLK_STS_OK)
	    {PWARN("maddevb_blk_mq_req_work_fn... dev#=%u cmd=%px req=%px cmderr=%d\n",
               pmaddev->devnum, cmd, cmd->req, cmd->error);}
    
	maddevb_end_cmd(cmd);
}

static enum hrtimer_restart maddevb_bwtimer_fn(struct hrtimer *timer)
{
	struct maddev_blk_io *pmdblkio = 
	                     container_of(timer, struct maddev_blk_io, bw_timer);
	ktime_t timer_interval = ktime_set(0, TIMER_INTERVAL);
	unsigned int mbps = pmdblkio->pmdblkdev->mbps;

	if (atomic_long_read(&pmdblkio->cur_bytes) == mb_per_tick(mbps))
		return HRTIMER_NORESTART;

	atomic_long_set(&pmdblkio->cur_bytes, mb_per_tick(mbps));
	maddevb_restart_queue_async(pmdblkio);

	hrtimer_forward_now(&pmdblkio->bw_timer, timer_interval);

	return HRTIMER_RESTART;
}

void maddevb_setup_bwtimer(struct maddev_blk_io *pmdblkio)
{
	ktime_t timer_interval = ktime_set(0, TIMER_INTERVAL);

	hrtimer_init(&pmdblkio->bw_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	pmdblkio->bw_timer.function = maddevb_bwtimer_fn;
	atomic_long_set(&pmdblkio->cur_bytes, mb_per_tick(pmdblkio->pmdblkdev->mbps));
	hrtimer_start(&pmdblkio->bw_timer, timer_interval, HRTIMER_MODE_REL);
}

struct maddevb_queue *maddevb_to_queue(struct maddev_blk_io *pmdblkio)
{
	int index = 0;

	if (pmdblkio->nr_queues != 1)
		index = raw_smp_processor_id() / 
		        ((nr_cpu_ids + pmdblkio->nr_queues - 1) / pmdblkio->nr_queues);

	return &pmdblkio->queues[index];
}

bool maddevb_should_timeout_request(struct request *rq)
{
#ifdef CONFIG_BLK_DEV_NULL_BLK_FAULT_INJECTION
	if (g_timeout_str[0])
		return should_fail(&maddevb_timeout_attr, 1);
#endif
	return false;
}

bool maddevb_should_requeue_request(struct request *rq)
{
#ifdef CONFIG_BLK_DEV_NULL_BLK_FAULT_INJECTION
	if (g_requeue_str[0])
		return should_fail(&maddevb_requeue_attr, 1);
#endif
	return false;
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,15,999)
static enum blk_eh_timer_return maddevb_timeout_req(struct request *preq, bool res)
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,6,0)
static enum blk_eh_timer_return maddevb_timeout_req(struct request *preq)
#endif
{
	ASSERT((int)(VIRT_ADDR_VALID(preq, sizeof(struct request))));
	struct maddevb_cmd *cmd = blk_mq_rq_to_pdu(preq);
	//container_of(preq, struct maddevb_cmd, req);
	PMADDEVOBJ pmaddev = (PMADDEVOBJ)cmd->pmdbq->pmbdev->pmaddev;

	PWARN("maddevb_timeout_req... expired timer *!*\n    \
		  dev=%u req=%px cmd=%px cmd->req=%px \n", 
		  pmaddev->devnum, preq, cmd, cmd->req);

	ASSERT((int)(VIRT_ADDR_VALID(cmd->req, sizeof(struct request))));
    ASSERT((int)blk_mq_request_started(preq));	
	cmd->req = POISON_VIRT_ADDR;
    maddevb_free_cmd(pmaddev, cmd);
	return BLK_EH_DONE;
}

static blk_status_t maddevb_queue_req(struct blk_mq_hw_ctx *hctx,
                                      const struct blk_mq_queue_data *bqd)
{
	struct maddevb_queue *pmdbq = hctx->driver_data;
	struct maddev_blk_dev *pmdblkdev = hctx->queue->queuedata;
	PMADDEVOBJ pmaddev = (PMADDEVOBJ)pmdblkdev->pmaddev;
	struct maddev_blk_io *pmdblkio = pmdblkdev->pmdblkio;
	struct request *req = bqd->rq;
	sector_t nr_sectors = blk_rq_sectors(req);
	sector_t sector = blk_rq_pos(req);
	#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,15,999)
	enum req_opf op = GET_OPF_FROM_REQ(req);
	#endif
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,6,0)
	blk_opf_t op = (blk_opf_t)(req->cmd_flags & REQ_OP_MASK);
	#endif

	struct maddevb_cmd *cmd;
	blk_status_t blk_sts = BLK_STS_OK;
	
	if (atomic_cmpxchg(&pmaddev->batomic, 0, 1))
	    {
        blk_sts = BLK_STS_DEV_RESOURCE;
		PWARN("maddevb_queue_req... dev#=%d concurrent requests: req=%px rc=%d\n",
			  (int)pmaddev->devnum, req, blk_sts);
	    return blk_sts;
		}
	
    //blk_mq_stop_hw_queue(hctx);

    PINFO("maddevb_queue_req... \n    \
		  dev#=%d hctx=%px pmdbq=%px req=%px #sctrs=%u op=%d\n",
          (int)pmaddev->devnum, hctx, pmdbq, bqd->rq, (uint32_t)nr_sectors, op);

	ASSERT((int)(hctx->queue == pmdblkio->reqQ));
	//ASSERT((int)(pmdbq == pmdblkio->queues)); //Works with the zeroth q
	ASSERT((int)(pmdbq->pmbdev == pmdblkdev));
	ASSERT((int)(pmaddev == pmdbq->pmbdev->pmaddev));

	might_sleep_if(hctx->flags & BLK_MQ_F_BLOCKING);
  
	if (maddevb_should_requeue_request(req))
        {
		//maddevb_free_cmd(pmaddev, cmd);
        /*
		 * Alternate between hitting the core BUSY path, and the
		 * driver driven requeue path
		 */
		pmdbq->requeue_selection++;
		if (pmdbq->requeue_selection & 1)
		    {
			//blk_mq_start_hw_queue(hctx);
			atomic_set(&pmaddev->batomic, 0);
			return BLK_STS_RESOURCE;
			}
		else
            {
			//blk_mq_start_hw_queue(hctx);
			atomic_set(&pmaddev->batomic, 0);
			return BLK_STS_RESOURCE;
		    }
	    }

	if (maddevb_should_timeout_request(req))
	    {
	    //maddevb_free_cmd(pmaddev, cmd);
		atomic_set(&pmaddev->batomic, 0);
		return BLK_STS_AGAIN;
		}

	cmd = kmem_cache_zalloc(pmaddev->pcmd_cache, GFP_ATOMIC);
	if (cmd == NULL)
	    {
		PERR("maddevb_queue_req... dev#=%u sts=BLK_STS_RESOURCE\n",
			 pmaddev->devnum);
		atomic_set(&pmaddev->batomic, 0);
	    return BLK_STS_RESOURCE;
		}

	cmd->req = req;
	cmd->error = BLK_STS_OK;
	cmd->pmdbq = pmdbq;
	cmd->hctx = hctx;
	if (pmdbq->pmbdev->irqmode == NULL_IRQ_TIMER)
        {maddevb_start_cmd_timer(cmd);}
	
	return maddevb_handle_cmd(cmd, sector, nr_sectors, op);
}

//This is a callback invoked within blk_alloc_tag_set()
static int 
maddevb_init_hctx(struct blk_mq_hw_ctx *hctx, void *data, unsigned int hctx_idx)
{
	struct maddev_blk_dev *pmdblkdev = (struct maddev_blk_dev *)data;// came from tagset->driver_data
	struct mad_dev_obj* pmaddev = (struct mad_dev_obj*)pmdblkdev->pmaddev;
    struct maddev_blk_io *pmdblkio = pmdblkdev->pmdblkio; 
    struct maddevb_queue *pmdbq = &pmdblkio->queues[hctx_idx];

	PINFO("maddevb_init_hctx... dev#=%u hctx=%px pmdblkdev=%px pmdbq=%px idx=%u\n",
          pmaddev->devnum, hctx, pmdblkdev, pmdbq, hctx_idx);

    // initialize per-hctx queue state here (locks, rings, stats, IRQ binding...)
    hctx->driver_data = pmdbq;   

    spin_lock_init(&pmdbq->splock);
    pmdbq->hctx_idx    = hctx_idx;
    pmdbq->pmdblkio    = pmdblkio;
	pmdbq->pmbdev      = pmdblkdev;
	pmdbq->queue_depth = pmdblkio->queue_depth;

    return 0;
}

static const struct blk_mq_ops maddevb_mq_ops = 
{
	.queue_rq   = maddevb_queue_req,
	.init_hctx  = maddevb_init_hctx,
    .exit_hctx  = NULL,
    .map_queues = NULL, //blk_mq_map_queues,
	.complete	= NULL, //maddevb_complete_req,

	#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,15,999)
	MADDEV_TIMEOUT_FIELD(maddevb_timeout_req),
	#endif
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,6,0)
	.timeout	= maddevb_timeout_req,
	#endif
};

void maddevb_cleanup_queues(struct maddev_blk_io *pmdblkio)
{
    PMADDEVOBJ pmaddev = pmdblkio->pmdblkdev->pmaddev;

    PINFO("maddevb_cleanup_queues... pmdblkdev#=%u dev=%px #Qs=%u\n",
          pmaddev->devnum, (void *)pmdblkio->pmdblkdev, 
		  pmdblkio->nr_queues);

	if (pmdblkio->queues != NULL)
		{kfree(pmdblkio->queues);}
}

void maddevb_delete_blockdev(struct maddev_blk_dev *pmdblkdev)
{
	struct maddev_blk_io *pmdblkio = pmdblkdev->pmdblkio;
    PMADDEVOBJ pmaddev = (PMADDEVOBJ)pmdblkdev->pmaddev;

    if ((pmaddev == NULL) || (pmdblkio == NULL))
        {
        PERR("maddevb_delete_blockdev... BAD ADDRESS! pmdblkdev=%px pmdblkio=%px pmaddev=%px\n",
             (void *)pmdblkdev, (void *)pmdblkio, (void *)pmaddev);
		return;
        }

    PINFO("maddevb_delete_blockdev... dev#=%u pmdblkdev=%px pmdblkio=%px\n",
          pmaddev->devnum, (void *)pmdblkdev, (void *)pmdblkio);

	//ida_simple_remove(&maddevb_indexes, (pmdblkio->index));

	if (pmdblkio->list.next != NULL)
	    list_del_init(&pmdblkio->list);

	if (pmdblkdev->gdisk != NULL)
	    {
	    del_gendisk(pmdblkdev->gdisk);
		#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,15,999)
	    blk_cleanup_disk(pmdblkdev->gdisk); //Calls put_disk(pmdblkdev->gdisk);
		#endif
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,6,0)
	    put_disk(pmdblkdev->gdisk); //Calls put_disk(pmdblkdev->gdisk);
		#endif
		}

	/*if (pmdblkio->reqQ != NULL)
	    {
		blk_mq_freeze_queue_wait(pmdblkio->reqQ); // or blk_mq_quiesce_queue(q)
	    blk_cleanup_queue(pmdblkio->reqQ);
        } */
	
	if (pmdblkio->tag_set != NULL)	
	    blk_mq_free_tag_set(pmdblkio->tag_set);
	maddevb_cleanup_queues(pmdblkio);

	if (test_bit(MADDEVB_DEV_FL_THROTTLED, &pmdblkio->pmdblkdev->flags))
        {
		hrtimer_cancel(&pmdblkio->bw_timer);
		atomic_long_set(&pmdblkio->cur_bytes, LONG_MAX);
		maddevb_restart_queue_async(pmdblkio);
	    }

	if (maddevb_io_cache_active(pmdblkio))
		maddevb_free_blockdev_storage(pmdblkio->pmdblkdev, true);

	kfree(pmdblkio);
    kfree(pmdblkdev);
	pmaddev->pmdblkdev = NULL;
	kfree(pmaddev->sglist);

    PINFO("maddevb_delete_blockdev exit... pmdblkdev#=%u pmdblkio=%px pmdblkio=%px\n",
          pmaddev->devnum, (void *)pmdblkdev, (void *)pmdblkio);
}

static const struct block_device_operations maddevb_fops =
{
	.owner           = THIS_MODULE,
	#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,15,999)
    MADDEVB_DEVOPS_OPEN_FIELD,
	MADDEVB_DEVOPS_RELEASE_FIELD,
 	MADDEVB_DEVOPS_RW_PAGE_FIELD,
	#endif
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,6,0)
	.open            = maddevb_open,
	.release         = maddevb_release,
    //.mmap            = maddev_mmap,
    //.rw_page         = maddevb_rw_page,
	#endif

    .getgeo          = maddevb_getgeo,
    .ioctl           = maddevb_ioctl,
    .compat_ioctl    = maddevb_ioctl,
    //.revalidate_disk = pmdblkio_revalidate_disk,
    .swap_slot_free_notify 
                     = maddevb_swap_slot_free_notify,
    .report_zones    = maddevb_zone_report,
};

static struct maddev_blk_dev *maddevb_alloc_blockdev(PMADDEVOBJ pmaddev)
{
	struct maddev_blk_dev *pmdblkdev = 
	                       kzalloc(sizeof(struct maddev_blk_dev), GFP_KERNEL);
	if (pmdblkdev == NULL)
        {
        PERR("maddevb_alloc_blockdev... returning NULL\n");
		return NULL;
        }

	INIT_RADIX_TREE(&pmdblkdev->data, GFP_ATOMIC);
	INIT_RADIX_TREE(&pmdblkdev->cache, GFP_ATOMIC);

    #ifdef _MADDEVB_BAD_BLOCK
	if (badblocks_init(&pmdblkdev->badblocks, 0))
        {
		kfree(pmdblkdev);
		return NULL;
	    }
    #endif

	pmdblkdev->size = (g_gs * g_bs);
	pmdblkdev->completion_nsec = g_completion_nsec;
	pmdblkdev->submit_queues = g_submit_queues;
	pmdblkdev->home_node = g_home_node;
	pmdblkdev->queue_mode = g_queue_mode;
	pmdblkdev->blocksize = g_bs;
	pmdblkdev->irqmode = g_irqmode;
	pmdblkdev->hw_queue_depth = g_hw_queue_depth;
    pmdblkdev->memory_backed = false;
    pmdblkdev->blocking = g_blocking;
	pmdblkdev->cache_size = 0;

	pmdblkdev->use_per_node_hctx = g_use_per_node_hctx;
    pmdblkdev->pmaddev = pmaddev;

   	pmdblkdev->zoned = false; //g_zoned;
    pmdblkdev->nr_zones = 0;
	pmdblkdev->pmaddev = pmaddev;

    #ifdef MADDEVB_ZONED
    pmdblkdev->zoned = g_zoned;
	pmdblkdev->zone_size = g_zone_size;
	pmdblkdev->zone_nr_conv = g_zone_nr_conv;
    #endif

    PINFO("maddevb_alloc_blockdev... pmdblkdev#=%u pmdblkdev=%px\n",
          pmaddev->devnum, (void *)pmdblkdev);

    return pmdblkdev;
}

int maddevb_gendisk_add(struct maddev_blk_dev *pmdblkdev)
{
	struct maddev_blk_io *pmdblkio = pmdblkdev->pmdblkio;
    PMADDEVOBJ pmaddev = pmdblkdev->pmaddev;
	sector_t size = (sector_t)pmdblkdev->size;
	struct gendisk *gdisk = NULL;
    int rc = 0;
	//int reg_mjr = 0;
 
    PINFO("maddevb_gendisk_add... dev#=%u pmdblkdev=%px mjr=%d size=%llu\n",
          pmaddev->devnum, (void *)pmdblkdev, pmdblkdev->major, size);

	gdisk = blk_mq_alloc_disk(pmdblkio->tag_set, pmdblkdev);
	if (gdisk == NULL)
        {
		rc = -ENOMEM;
        goto gendisk_reg_out;
        }
		
	pmdblkio->reqQ = gdisk->queue;
	ASSERT((int)(pmdblkio->reqQ->queuedata == pmdblkdev));
    maddevb_init_request_queue(pmdblkio);

    set_capacity(gdisk, (size >> MAD_SECTOR_SHIFT)); //9)); //sectors
	gdisk->flags       |= MADDEVB_GENHD_FLAGS;
	gdisk->major		= pmdblkdev->major;
	gdisk->first_minor  = pmdblkdev->index;
	gdisk->fops		    = &maddevb_fops;
	gdisk->private_data = pmdblkdev;
	gdisk->minors       = 1; //The whole disk plus zero partitions for a floppy-disk
	strncpy(gdisk->disk_name, pmdblkdev->disk_name, DISK_NAME_LEN);
	//disk->parent       =&pmaddev->pPcidev->dev;
	//disk->part0->bd_device = &pmaddev->pPcidev.dev;

    #ifdef MADDEVB_ZONED
	if (pmdblkdev->zoned) 
        {
		ret = blk_revalidate_disk_zones(gdisk);
		if (ret != 0)
			goto gendisk_reg_out;
	    }
    #endif

	//This registers the disk with sysfs & it becomes visible as /sys/block/<name> 
	rc = add_disk(gdisk); 
	if (rc != 0) 
	    {
		put_disk(gdisk);
		goto gendisk_reg_out;
	    }

	ASSERT((int)(gdisk->bdi != NULL));
    pmdblkdev->gdisk = gdisk;
	pmdblkdev->bdev = gdisk->part0;
	ASSERT((int)(pmdblkdev->bdev != NULL));
    ASSERT((int)(pmdblkdev->bdev->bd_disk == pmdblkdev->gdisk));

gendisk_reg_out:;    
    PINFO("maddevb_gendisk_add... \ndev#=%u gdisk=%px bdev=%px pmdblkdev=%px pmdblkio=%px rc=%d\n",
          pmaddev->devnum, gdisk, pmdblkdev->bdev,
		  pmdblkdev, pmdblkio, rc);

    return rc;
}

static int maddevb_add_blockdev(struct maddev_blk_dev *pmdblkdev)
{
	PMADDEVOBJ pmaddev = (PMADDEVOBJ)pmdblkdev->pmaddev;
    int devnum = (int)pmaddev->devnum;
	struct maddev_blk_io  *pmdblkio;
	int rc = 0;
	
    PINFO("maddevb_add_blockdev... dev#=%d pmaddev=%px pmdblkdev=%px\n",
		  devnum, pmaddev, pmdblkdev);

    maddevb_validate_conf(pmdblkdev);

	pmdblkio = 
    kzalloc_node(sizeof(struct maddev_blk_io), GFP_KERNEL, pmdblkdev->home_node);
	if (!pmdblkio)
        {
		rc = -ENOMEM;
		goto out;
	    }

    pmdblkio->pmdblkdev = pmdblkdev;
    pmdblkdev->pmdblkio = pmdblkio;

	pmaddev->sglist = kmalloc_array((MAD_SGDMA_MAX_ENTRYS + 1),
	                                sizeof(struct scatterlist), GFP_KERNEL);
    if (pmaddev->sglist == NULL) 
	   {
	   rc = -ENOMEM;
	   goto out;
	   }

	spin_lock_init(&pmdblkio->lock);

    switch (pmdblkdev->queue_mode)
        {
        case MADDEVB_Q_RQ:
        case MADDEVB_Q_MQ:
            if (shared_tags)
                {
                pmdblkio->tag_set = &tag_set;
                rc = 0;
                }
            else
                {
                pmdblkio->tag_set = &pmdblkio->__tag_set;
                rc = maddevb_alloc_init_tag_set(pmdblkio, pmdblkio->tag_set);
                }

            if (rc)
                goto out_cleanup_queues;

            if (!maddevb_setup_fault())
                goto out_cleanup_queues;

            pmdblkio->tag_set->timeout = 3 * HZ;
            pmdblkio->reqQ = blk_mq_init_queue(pmdblkio->tag_set);
            if (IS_ERR(pmdblkio->reqQ))
                {
                rc = -ENOMEM;
                goto out_cleanup_tags;
                }

            break;

		case MADDEVB_Q_BIO:
		    rc = -ENOTSUPP;
			#if 0
            pmdblkio->reqQ = blk_alloc_queue_node(GFP_KERNEL, dev->home_node);
            if (!pmdblkio->reqQ)
                {
                rv = -ENOMEM;
                goto out_cleanup_queues;
                }

            blk_queue_make_request(pmdblkio->reqQ, pmdblkio_queue_bio);
			
            rv = maddevb_init_driver_queues(pmdblkio);
            if (rv)
                goto out_cleanup_blk_queue;
			#endif
            break;

        default:
            PWARN("maddevb_add_blockdev... pmdblkdev#=%d unsupported Qmode=%d\n",
                  (int)pmaddev->devnum, pmdblkdev->queue_mode);
        }

	if (pmdblkdev->mbps) 
        {
		set_bit(MADDEVB_DEV_FL_THROTTLED, &pmdblkdev->flags);
		maddevb_setup_bwtimer(pmdblkio);
	    }

	if (pmdblkdev->cache_size > 0)
        {
		set_bit(MADDEVB_DEV_FL_CACHE, &pmdblkdev->flags);
		blk_queue_write_cache(pmdblkio->reqQ, true, true);
	    }

    #ifdef MADDEVB_ZONED
	if (pmdblkdev->zoned) 
        {
		rv = maddevb_zone_init(pmdblkdev);
		if (rv)
			goto out_cleanup_blk_queue;

		blk_queue_chunk_sectors(pmdblkio->q, pmdblkdev->zone_size_sects);
		pmdblkio->q->limits.zoned = BLK_ZONED_HM;
		blk_queue_flag_set(QUEUE_FLAG_ZONE_RESETALL, pmdblkio->reqQ);
		blk_queue_required_elevator_features(pmdblkio->reqQ,
						                     ELEVATOR_F_ZBD_SEQ_WRITE);
	    }
    #endif

	spin_lock(&pmdblkio->lock);
    //unsigned int indx = ida_simple_get(&pmdblkio_indexes,0,0,GFP_KERNEL);
    pmdblkio->index  = pmaddev->devnum;
    pmdblkdev->index = pmdblkio->index;
	pmdblkdev->major = reg_blkdev_major;
	spin_unlock(&pmdblkio->lock);

	//maddevb_config_discard(pmdblkio);
	sprintf(pmdblkdev->disk_name, "fd%d", pmdblkdev->index);
	rc = maddevb_gendisk_add(pmdblkdev);
	if (rc)
		goto out_cleanup_zone;

	spin_lock(&pmdblkio->lock);
	list_add_tail(&pmdblkio->list, &maddevb_list);
	spin_unlock(&pmdblkio->lock);
    WRITE_ONCE(pmaddev->bReady, true);
	return rc;

out_cleanup_zone:
    #ifdef MADDEVB_ZONED
	if (pmdblkdev->zoned)
		maddevb_zone_exit(pmdblkdev);
    #endif

//out_cleanup_blk_queue:
	#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,15,999)
    if (pmdblkio->reqQ  != NULL)
	    {maddevb_blk_cleanup_queue(pmdblkio->reqQ);}
	#endif

out_cleanup_tags:
	if (pmdblkdev->queue_mode == MADDEVB_Q_MQ && 
		pmdblkio->tag_set == &pmdblkio->__tag_set)
		    blk_mq_free_tag_set(pmdblkio->tag_set);

out_cleanup_queues:
	maddevb_cleanup_queues(pmdblkio);

//out_free_pmdblkio:
	kfree(pmdblkio);
	pmdblkdev->pmdblkio = NULL;

out:
    PERR("maddevb_add_blockdev... pmdblkdev#=%d dev=%px pmdblkio=%px rc=%d\n",
         (int)pmaddev->devnum,  pmdblkdev, pmdblkio, rc);

	return rc;
}
 
int maddevb_create_blockdev(/*PMADDEVOBJ pmaddev*/void* pv)
{
    PMADDEVOBJ pmaddev = (PMADDEVOBJ)pv;
    int rc = 0;

    PINFO("maddevb_create_blockdev... dev#=%u\n", pmaddev->devnum);

    pmaddev->pmdblkdev = maddevb_alloc_blockdev(pmaddev);
	if (pmaddev->pmdblkdev == NULL)
        {rc = -ENOMEM;}

    if (rc == 0)
        {rc = maddevb_add_blockdev(pmaddev->pmdblkdev);}

	if (rc != 0)
        {
        PERR("maddevb_create/add_device... pmdblkdev#=%d rc=%d\n",
             (int)pmaddev->devnum, rc);
        maddevb_free_blockdev(pmaddev->pmdblkdev);
        }

    return rc;
}  

void maddevb_config_discard(struct maddev_blk_io *pmdblkio)
{
    PMADDEVOBJ pmaddev = pmdblkio->pmdblkdev->pmaddev;

    if (pmdblkio->pmdblkdev->discard == false)
        {return;}

	pmdblkio->reqQ->limits.discard_granularity = pmdblkio->pmdblkdev->blocksize;
	pmdblkio->reqQ->limits.discard_alignment = pmdblkio->pmdblkdev->blocksize;
	blk_queue_max_discard_sectors(pmdblkio->reqQ, UINT_MAX >> 9);

	#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,15,999)
	blk_queue_flag_set(QUEUE_FLAG_DISCARD, pmdblkio->reqQ);
	#endif

    PINFO("maddevb_config_discard... dev#=%d reqQ=%px\n",
          (int)pmaddev->devnum, pmdblkio->reqQ);
}

int maddevb_getgeo(struct block_device* bdev, struct hd_geometry* geo)
{
    PMADDEVOBJ pmaddev = maddevb_get_parent_from_bdev(bdev);
    U32 capacity = get_capacity(bdev->bd_disk);
   
    geo->heads     = 1;
    geo->sectors   = MAD_SECTORS_PER_PAGE; 
    geo->cylinders = capacity / (geo->heads * geo->sectors);
    geo->start     = 0;
 
    PINFO("maddevb_getgeo... dev#=%d bdev=%px cap=%u #cylinders:#hds:#sectors=%d,%d,%d\n",
          (int)pmaddev->devnum, bdev, (uint32_t)capacity, 
		  geo->cylinders, geo->heads, geo->sectors);

    return 0;
}

void maddevb_swap_slot_free_notify(struct block_device* bdev,
                                   unsigned long offset)
{
    PMADDEVOBJ pmaddev = maddevb_get_parent_from_bdev(bdev);

    PINFO("maddevb_swap_slot_free_notify... dev#=%d bdev=%px offset=%u\n",
          (int)pmaddev->devnum, bdev, (uint32_t)offset);

    //Nothing to do
    return;
}

int maddevb_zone_report(struct gendisk *disk, sector_t sector,
		                /*struct blk_zone *zones,*/ 
						unsigned int nr_zones, report_zones_cb cb, void* data)
{
	static fmode_t mode = FMODE_READ | FMODE_WRITE;
    struct block_device *bdev = 
	                    maddevb_blkdev_get_by_dev(disk_devt(disk), mode, NULL);
	struct maddev_blk_dev *pmdblkdev = bdev->bd_disk->private_data;
    PMADDEVOBJ pmaddev   = (PMADDEVOBJ)pmdblkdev->pmaddev;
 
    PINFO("maddevb_io_zone_report... dev#=%d bdev=%px nr_zones=%u\n",
          (int)pmaddev->devnum, bdev, (uint32_t)pmdblkdev->nr_zones);

     //*nr_zones = pmdblkio->dev->nr_zones;

	/*zno = null_zone_no(dev, sector);
	if (zno < dev->nr_zones) {
		nrz = min_t(unsigned int, *nr_zones, dev->nr_zones - zno);
		memcpy(zones, &dev->zones[zno], nrz * sizeof(struct blk_zone));
	} */
    
	#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,15,999)
    maddevb_blkdev_put(bdev, mode);
	#endif
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,6,0)
    maddevb_blkdev_put(bdev, NULL);
	#endif

	return 0;
}

//
#if 0 //static void maddevb_init_queue(struct maddev_blk_io *pmdblkio,
	                           struct maddevb_queue *pmdbq)
{
	struct maddev_blk_dev *pmdblkdev = pmdblkio->pmdblkdev;
    PMADDEVOBJ pmaddev = (PMADDEVOBJ)pmdblkdev->pmaddev;

	init_waitqueue_head(&pmdbq->wait);
	pmdbq->queue_depth = pmdblkio->queue_depth;
	pmdbq->pmbdev = pmdblkdev;

	PINFO("maddevb_init_queue... dev#=%u pmdblkio=%px pmdblkdev=%px pmdbq=%px\n",
          pmaddev->devnum, pmdblkio, pmdblkdev, pmdbq);
}
#endif

#if 0
void maddevb_init_queues(struct maddev_blk_io *pmdblkio)
{
	struct maddev_blk_dev *pmdblkdev = pmdblkio->pmdblkdev;
    PMADDEVOBJ pmaddev = (PMADDEVOBJ)pmdblkdev->pmaddev;
	struct request_queue *reqQ = pmdblkio->reqQ;
	struct blk_mq_hw_ctx *hctx;
	struct maddevb_queue *pmdbq = pmdblkio->queues;
	int i = 0;
   
	//blk_queue_flag_set(QUEUE_FLAG_NONROT, pmdblkio->reqQ);
	blk_queue_flag_clear(QUEUE_FLAG_ADD_RANDOM, pmdblkio->reqQ);
	blk_queue_logical_block_size(pmdblkio->reqQ, pmdblkdev->blocksize);
	blk_queue_physical_block_size(pmdblkio->reqQ, pmdblkdev->blocksize);
    blk_queue_max_hw_sectors(pmdblkio->reqQ, MAD_SGDMA_MAX_SECTORS);
	blk_queue_max_segments(pmdblkio->reqQ, MAD_SGDMA_MAX_SECTORS);
	blk_queue_io_min(pmdblkio->reqQ, MAD_SECTOR_SIZE);
    blk_queue_io_opt(pmdblkio->reqQ, PAGE_SIZE);

    pmdblkio->nr_queues = 0;
	queue_for_each_hw_ctx(reqQ, hctx, i)
        {
		if (!hctx->nr_ctx || !hctx->tags)
			goto nextq;

		maddevb_init_queue(pmdblkio, pmdbq);
		hctx->driver_data = pmdbq;
		hctx->queue = reqQ;
		pmdblkio->nr_queues++;
nextq:;		
		pmdbq++;
	    }

	ASSERT((int)(pmdblkio->nr_queues == 
		         pmdblkio->pmdblkdev->submit_queues));

    PINFO("maddevb_init_queues... dev#=%u pmdblkio=%px num_qs=%d\n",
          pmaddev->devnum, pmdblkio, (U32)pmdblkio->nr_queues);
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,12,0)
void maddevb_init_request_queue(struct maddev_blk_io *pmdblkio)
{
	struct maddev_blk_dev *pmdblkdev = pmdblkio->pmdblkdev;
    PMADDEVOBJ pmaddev = (PMADDEVOBJ)pmdblkdev->pmaddev;
	struct request_queue *reqQ =pmdblkio->reqQ;
    struct queue_limits *qlim = &reqQ->limits;

    PINFO("maddevb_init_request_queue... dev#=%d pmdblkio=%px num_qs=%d qlim=%px\n",
          (int)pmaddev->devnum, pmdblkio, 
		  (int)pmdblkio->pmdblkdev->submit_queues, qlim);

    qlim->logical_block_size  = pmdblkdev->blocksize;
    qlim->physical_block_size = pmdblkdev->blocksize;
    qlim->max_hw_sectors      = MAD_SGDMA_MAX_SECTORS;
    qlim->max_segments        = MAD_SGDMA_MAX_SECTORS;
    qlim->max_segment_size    = PAGE_SIZE;
    qlim->seg_boundary_mask   = PAGE_SIZE - 1;
    qlim->virt_boundary_mask  = PAGE_SIZE - 1;
    qlim->io_min              = MAD_SECTOR_SIZE;
    qlim->io_opt              = PAGE_SIZE;		  
}
#else
void maddevb_init_request_queue(struct maddev_blk_io *pmdblkio)
{
	struct maddev_blk_dev *pmdblkdev = pmdblkio->pmdblkdev;
    PMADDEVOBJ pmaddev = (PMADDEVOBJ)pmdblkdev->pmaddev;

    PINFO("maddevb_init_request_queue... dev#=%d pmdblkio=%px num_qs=%d\n",
          (int)pmaddev->devnum, pmdblkio, 
		  (int)pmdblkio->pmdblkdev->submit_queues);

	//blk_queue_flag_set(QUEUE_FLAG_NONROT, pmdblkio->reqQ);
	#ifdef QUEUE_FLAG_ADD_RANDOM
        blk_queue_flag_clear(QUEUE_FLAG_ADD_RANDOM, pmdblkio->reqQ);
    #endif
	#ifdef QUEUE_FLAG_NOMERGES
        blk_queue_flag_set(QUEUE_FLAG_NOMERGES, pmdblkio->reqQ);
    #endif
	blk_queue_logical_block_size(pmdblkio->reqQ, pmdblkdev->blocksize);
	blk_queue_physical_block_size(pmdblkio->reqQ, pmdblkdev->blocksize);
    blk_queue_max_hw_sectors(pmdblkio->reqQ, MAD_SGDMA_MAX_SECTORS);
	blk_queue_max_segments(pmdblkio->reqQ, MAD_SGDMA_MAX_SECTORS);
    blk_queue_max_segment_size(pmdblkio->reqQ, PAGE_SIZE);
    blk_queue_segment_boundary(pmdblkio->reqQ, PAGE_SIZE - 1);
	#ifdef blk_queue_virt_boundary      /* some trees macro-wrap this; safe guard */
        blk_queue_virt_boundary(q, PAGE_SIZE - 1);
    #endif
    blk_queue_dma_alignment(pmdblkio->reqQ, 0);
    blk_queue_flag_set(QUEUE_FLAG_NOMERGES, pmdblkio->reqQ); //optional but helpful

	blk_queue_io_min(pmdblkio->reqQ, MAD_SECTOR_SIZE);
    blk_queue_io_opt(pmdblkio->reqQ, PAGE_SIZE);
    //pmdblkio->nr_queues = pmdblkio->pmdblkdev->submit_queues;
}
#endif

int maddevb_setup_queues(struct maddev_blk_io *pmdblkio)
{
	struct maddev_blk_dev *pmdblkdev = pmdblkio->pmdblkdev;
    PMADDEVOBJ pmaddev = (PMADDEVOBJ)pmdblkdev->pmaddev;
    int rc = 0;

	pmdblkio->nr_queues = 0;   
	pmdblkio->queues = kcalloc(pmdblkio->pmdblkdev->submit_queues,
				               sizeof(struct maddevb_queue), GFP_KERNEL);
	if (pmdblkio->queues == NULL)
        {
		rc = -ENOMEM;	
        PERR("maddevb_setup_queues... dev#=%u pmdblkio=%px rc=%d\n",
			 pmaddev->devnum, pmdblkio, rc);
		return rc;
        }
   
	pmdblkio->queue_depth = pmdblkio->pmdblkdev->hw_queue_depth;
    pmdblkio->nr_queues = pmdblkio->pmdblkdev->submit_queues; 
    
	PINFO("maddevb_setup_queues... dev#=%u pmdblkio=%px #queues=%d Qdepth=%d rc=%d\n",
		  pmaddev->devnum, pmdblkio, 
		  pmdblkio->nr_queues, pmdblkio->queue_depth, rc);

	return 0;
}

int maddevb_alloc_init_tag_set(struct maddev_blk_io *pmdblkio, 
	                           struct blk_mq_tag_set *tgset)
{
	struct maddev_blk_dev *pmdblkdev = pmdblkio->pmdblkdev;
    PMADDEVOBJ pmaddev = (PMADDEVOBJ)pmdblkdev->pmaddev;
    int rc = 0;
   
	PINFO("maddevb_alloc_init_tag_set... dev#=%u\n", pmaddev->devnum);
	
	memset(tgset, 0x00, sizeof(struct blk_mq_tag_set));
	tgset->ops          = &maddevb_mq_ops;
	tgset->nr_hw_queues = g_submit_queues;
	tgset->queue_depth  = g_hw_queue_depth;
	tgset->numa_node    = g_home_node;
	tgset->cmd_size	    = 0; //sizeof(struct maddevb_cmd);
	//tgset->map_queues   = NULL;

	//if (g_no_sched)
	tgset->flags = (BLK_MQ_F_NO_SCHED | BLK_MQ_F_BLOCKING);
	tgset->driver_data = pmdblkio->pmdblkdev;
	//if (pmdblkio->pmdblkdev->blocking || g_blocking)
	//    tgset->flags |= BLK_MQ_F_BLOCKING;
    //tgset->flags |= BLK_MQ_F_SHOULD_MERGE;

	rc = maddevb_setup_queues(pmdblkio);
	if (rc == 0)
	    {rc = blk_mq_alloc_tag_set(tgset);} //Invokes callback maddevb_init_hctx
	
    PINFO("maddevb_alloc_init_tag_set... dev#=%u tagset=%px Qdepth=%d flags=x%X rc=%d\n",
          pmaddev->devnum, tgset, tgset->queue_depth, tgset->flags, rc);

    return rc;
}

void maddevb_validate_conf(struct maddev_blk_dev *pmdblkdev)
{
    PMADDEVOBJ pmaddev = (PMADDEVOBJ)pmdblkdev->pmaddev;

    PINFO("maddevb_validate_conf... dev#=%d pmdblkdev=%px pmaddev=%px\n",
          (int)pmaddev->devnum, pmdblkdev, pmaddev);

	pmdblkdev->blocksize = round_down(pmdblkdev->blocksize, 512);
	pmdblkdev->blocksize = clamp_t(unsigned int, pmdblkdev->blocksize, 512, 4096);

	if ((pmdblkdev->queue_mode == MADDEVB_Q_MQ) && pmdblkdev->use_per_node_hctx)
        {
		if (pmdblkdev->submit_queues != nr_online_nodes)
			pmdblkdev->submit_queues = nr_online_nodes;
	    }
     else
        {
         if (pmdblkdev->submit_queues > nr_cpu_ids)
            pmdblkdev->submit_queues = nr_cpu_ids;
         else
             if (pmdblkdev->submit_queues == 0)
                 pmdblkdev->submit_queues = 1;
        }

	pmdblkdev->queue_mode = min_t(unsigned int, pmdblkdev->queue_mode, MADDEVB_Q_MQ);
	pmdblkdev->irqmode = min_t(unsigned int, pmdblkdev->irqmode, NULL_IRQ_TIMER);

	/* Do memory allocation, so set blocking */
	if (pmdblkdev->memory_backed)
		pmdblkdev->blocking = true;
	else /* cache is meaningless */
		pmdblkdev->cache_size = 0;

    pmdblkdev->cache_size = 
    min_t(unsigned long, ULONG_MAX / 1024 / 1024, pmdblkdev->cache_size);

	pmdblkdev->mbps = min_t(unsigned int, 1024 * 40, pmdblkdev->mbps);
	/* can not stop a queue */
	if (pmdblkdev->queue_mode == MADDEVB_Q_BIO)
		pmdblkdev->mbps = 0;

	return;	
}

#ifdef CONFIG_BLK_DEV_NULL_BLK_FAULT_INJECTION
static bool __maddevb_setup_fault(struct fault_attr *attr, char *str)
{
	if (!str[0])
		return true;

	if (!setup_fault_attr(attr, str))
		return false;

	attr->verbose = 0;
	return true;
}
#endif

bool maddevb_setup_fault(void)
{
#ifdef CONFIG_BLK_DEV_NULL_BLK_FAULT_INJECTION
	if (!__pmdblkio_setup_fault(&pmdblkio_timeout_attr, g_timeout_str))
		return false;

	if (!__pmdblkio_setup_fault(&pmdblkio_requeue_attr, g_requeue_str))
		return false;
#endif
	return true;
}

enum hrtimer_restart maddevb_cmd_timer_expired(struct hrtimer *ptimer)
{
	struct maddevb_cmd *cmd = container_of(ptimer, struct maddevb_cmd, cmdtimer);
	maddevb_end_cmd(cmd);

	return HRTIMER_NORESTART;
}

#if 0 //int maddevb_process_io_request_old(struct mad_dev_obj *pmaddev, struct bio* pbio,
                            sector_t Sector, sector_t nr_sectors, enum req_opf op)
{
    static struct   bio_vec biovecs[MAD_SGDMA_MAX_SECTORS];
    //
    sector_t tsector = Sector;
    int      iorc = 0;
    int      nr_bvecs = 0;
	bool     bH2D = (op==REQ_OP_WRITE);
    
    PINFO("maddevb_process_io_request... \n    \
		  dev#=%d op=%d sector=%llu bio=%px bio_next=%px #biovecs=%d biovecs=%px\n",
          (int)pmaddev->devnum, op, tsector, (void *)pbio, (void *)pbio->bi_next, 
          pbio->bi_vcnt, (void *)pbio->bi_io_vec);

    mutex_lock(&pmaddev->devmutex);
    nr_bvecs = maddevb_collect_biovecs(pmaddev, pbio, nr_sectors, biovecs);
    if ((nr_bvecs == 0) || (nr_bvecs > MAD_SGDMA_MAX_SECTORS))
        {
        ASSERT((int)false);
        mutex_unlock(&pmaddev->devmutex);
        return -EINVAL;
        } 

    iorc = maddevb_xfer_sgdma_bvecs(pmaddev, biovecs, nr_bvecs, tsector,
		                            nr_sectors, bH2D);
    mutex_unlock(&pmaddev->devmutex);

    if (iorc != 0)
        {PERR("maddevb_process_io_request... dev#=%d iorc=%d\n",
              (int)pmaddev->devnum, iorc);}

    return iorc;
}
#endif

#if 0 //static inline void maddevb_complete_cmd(struct pmdblkio_cmd *cmd)
{
    struct mad_dev_obj *pmaddev = 
		               (struct mad_dev_obj *)cmd->nq->dev->pmaddev;

	PINFO("maddevb_complete_cmd... dev#=%d cmd=%px req=%px\n",
		  (int)pmaddev->devnum, (void *)cmd, (void *)cmd->req);

	/* Complete IO by inline, softirq or timer */
	switch (cmd->nq->dev->irqmode)
        {
	    case NULL_IRQ_SOFTIRQ:
		    switch (cmd->nq->dev->queue_mode)
                {
		        case MADDEVB_Q_MQ:
				case MADDEVB_Q_RQ:
				    //blk_mq_complete_request(cmd->req);
					//blk_mq_end_request(cmd->req);
					maddevb_end_cmd(cmd);
			        break;

		        case MADDEVB_Q_BIO:
			    /*
			     * XXX: no proper submitting cpu information available.
			     */
			        maddevb_end_cmd(cmd);
			        break;

                default:
                    PERR("maddevb_complete_cmd... dev#=%d invalid_qmode=%d\n",
                         (int)pmaddev->devnum, (int)cmd->nq->dev->queue_mode);
		        }
		    break;

	    case NULL_IRQ_NONE:
		    maddevb_end_cmd(cmd);
		    break;

        case NULL_IRQ_TIMER:
		    maddevb_cmd_end_timer(cmd);
		    break;

        default:
            PERR("maddevb_complete_cmd... dev#=%d invalid_irqmode=%d\n",
                 (int)pmaddev->devnum, (int)cmd->nq->dev->irqmode);
	    }
}
#endif

#if 0 //int maddevb_init_driver_queues(struct maddev_blk_io *pmdblkio)
{
	struct maddevb_queue *pmdbq;
	int i, ret = 0;
    PMADDEVOBJ pmaddev = (PMADDEVOBJ)pmdblkio->pmdblkdev->pmaddev;

	for (i = 0; i < pmdblkio->pmdblkdev->submit_queues; i++)
        {
		pmdbq = &pmdblkio->queues[i];

		maddevb_init_queue(pmdblkio, pmdbq);

		ret = maddevb_setup_commands(pmdbq);
		if (ret)
            {
            PWARN("maddevb_init_driver_queues... dev#=%d pmdblkio=%px rc=%d\n",
                  (int)pmaddev->devnum, pmdblkio, ret);
			return ret;
            }

        pmdblkio->nr_queues++;
	    }

    PINFO("maddevb_init_driver_queues... dev#=%d nr_qs=%d\n",
          (int)pmaddev->devnum, (int)pmdblkio->nr_queues);
		//cmd->tag = -1U;

	return 0;
}
#endif //0

#if 0 //static int maddevb_setup_commands(struct maddevb_queue *pmdbq)
{
	PMADDEVOBJ pmaddev = (PMADDEVOBJ)pmdbq->dev->pmaddev;
	struct maddevb_cmd *cmd;
	int i, tag_size;
	int rc = 0;
	
	pmdbq->cmds = kcalloc(pmdbq->queue_depth, sizeof(*cmd), GFP_KERNEL);
	if (!pmdbq->cmds)
	    {
		PERR("maddevb_setup_commands... dev#=%d pmdbq=%px rc=-ENOMEM\n",
             (int)pmaddev->devnum, pmdbq, pmdbq->tag_map);
		return -ENOMEM;
		}

	tag_size = ALIGN(pmdbq->queue_depth, BITS_PER_LONG) / BITS_PER_LONG;
	pmdbq->tag_map = kcalloc(tag_size, sizeof(unsigned long), GFP_KERNEL);
	if (!pmdbq->tag_map) 
        {
		kfree(pmdbq->cmds);
		rc = -ENOMEM;
	    }
    else 
	    {
	    for (i = 0; i < pmdbq->queue_depth; i++)
            {
		    cmd = &pmdbq->cmds[i];
		    cmd->pmdbq = pmdbq;
		    INIT_LIST_HEAD(&cmd->list);
		    cmd->ll_list.next = NULL;
		    cmd->tag = -1U;
	        }
		}

    PINFO("maddevb_setup_commands... dev#=%d pmdbq=%px q_tg_map=%px rc=%d\n",
          (int)pmaddev->devnum, pmdbq, pmdbq->tag_map, rc);

	return rc;
}
#endif

#if 0 //int maddevb_revalidate_disk(struct gendisk *disk)
{
    struct block_device *bdev = bdget_disk(disk, 0);
    PMADDEVOBJ pmaddev = maddevb_get_parent_from_bdev(bdev);

    PINFO("maddevb_revalidate_disk... dev#=%d disk=%px bdev=%px\n",
          (int)pmaddev->devnum, disk, bdev);

    mutex_lock(&pmaddev->devmutex);
    //Nothing to do
    mutex_unlock(&pmaddev->devmutex);

    return 0;
}
#endif
#if 0 //static struct
config_item *maddevb_group_make_item(struct config_group *group, const char *name)
{
	struct maddev_blk_dev *dev = maddevb_alloc_blockdev();
	if (dev == NULL)
        {
        PERR("maddevb_group_make_item... rc=-ENOMEM\n");
        return ERR_PTR(-ENOMEM);
        }

	config_item_init_type_name(&dev->item, name, &maddevb_device_type);

	return &dev->item;
}
#endif

#if 0 //static void maddevb_put_tag(struct maddevb_queue *pmdbq, unsigned int tag)
{
	ASSERT((int)(pmdbq->tag_map != NULL));
	clear_bit_unlock(tag, pmdbq->tag_map);

	if (waitqueue_active(&pmdbq->wait))
		wake_up(&pmdbq->wait);
}
#endif

#if 0 //static unsigned int maddevb_get_tag(struct maddevb_queue *pmdbq)
{
	unsigned int tag;
    ASSERT((int)(pmdbq->tag_map != NULL));

	do {
		tag = find_first_zero_bit(pmdbq->tag_map, pmdbq->queue_depth);
		if (tag >= pmdbq->queue_depth)
			return -1U;

	    } while (test_and_set_bit_lock(tag, pmdbq->tag_map));

	return tag;
}
#endif

#if 0 //static void maddevb_free_cmd(struct maddevb_cmd *cmd)
{ 
	struct mad_dev_obj *pmaddev; 
	
	if (cmd->pmdbq != NULL)
	    {
		pmaddev = (struct mad_dev_obj *)cmd->pmdbq->dev->pmaddev;
		PINFO("maddevb_free_cmd... dev#=%d cmd=%px pmdbq=%px tg_map=%px\n",
              (int)pmaddev->devnum, cmd, cmd->pmdbq, cmd->pmdbq->tag_map);

        ASSERT((int)(cmd->pmdbq->tag_map != NULL));
	    if (cmd->pmdbq->tag_map != NULL)
	        if (cmd->tag != (unsigned int)-1)
	            maddevb_put_tag(cmd->pmdbq, cmd->tag);
		}

	cmd->req = NULL;	
	//No memory to free		 
}
#endif
#if 0 //static struct configfs_subsystem maddevb_subsys = 
{
	.su_group = {
		.cg_item = {
			.ci_namebuf = "maddevb",
			.ci_type = &maddevb_group_type,
		},
	},
};
#endif

#if 0 //static void maddevb_cmd_end_timer(struct pmdblkio_cmd *cmd)
{
	ktime_t kt = cmd->nq->dev->completion_nsec;
	struct mad_dev_obj *pmaddev = 
		               (struct mad_dev_obj *)cmd->nq->dev->pmaddev;

	PINFO("maddevb_cmd_end_timer.. dev#=%d cmd=%px\n",
		  (int)pmaddev->devnum, cmd);

	hrtimer_start(&cmd->timer, kt, HRTIMER_MODE_REL);
}
#endif

#if 0 //static void maddevb_complete_req(struct request *req)
{
	struct pmdblkio_cmd *cmd = blk_mq_rq_to_pdu(req);
	struct mad_dev_obj *pmaddev = 
		               (struct mad_dev_obj *)cmd->nq->dev->pmaddev;

    PINFO("maddevb_complete_req... dev#=%d cmd=%px req=%px\n", 
		  (int)pmaddev->devnum, cmd, req);

	maddevb_end_cmd(cmd);
}
#endif	

#if 0 //static struct maddevb_cmd *__alloc_cmd(struct maddevb_queue *pmdbq)
{
	struct maddevb_cmd *cmd;
	unsigned int tag;

	tag = maddevb_get_tag(pmdbq);
	if (tag != -1U) 
        {
		cmd = &pmdbq->cmds[tag];
		cmd->tag = tag;
		cmd->error = BLK_STS_OK;
		cmd->pmdbq = pmdbq;
		if (pmdbq->dev->irqmode == NULL_IRQ_TIMER) 
            {
			hrtimer_init(&cmd->timer, CLOCK_MONOTONIC,
				         HRTIMER_MODE_REL);
			cmd->timer.function = maddevb_cmd_timer_expired;
		    }

        return cmd;
	    }

	return NULL;
}
#endif

#if 0 //static struct maddevb_cmd *maddevb_alloc_cmd(struct maddevb_queue *pmdbq, int can_wait)
{
	struct maddevb_cmd *cmd;
	DEFINE_WAIT(wait);

	cmd = __alloc_cmd(pmdbq);
	if (cmd || !can_wait)
		return cmd;

	do {
		prepare_to_wait(&pmdbq->wait, &wait, TASK_UNINTERRUPTIBLE);
		cmd = __alloc_cmd(pmdbq);
		if (cmd)
			break;

		io_schedule();
	    } while (1);

	finish_wait(&pmdbq->wait, &wait);
	return cmd;
}
#endif

#if 0 //blk_qc_t maddevb_queue_bio(struct request_queue *q, struct bio *bio)
{
	sector_t sector = bio->bi_iter.bi_sector;
	sector_t nr_sectors = bio_sectors(bio);
	struct maddev_blk_io *pmdblkio = q->queuedata;
	struct maddevb_queue *pmdbq = maddevb_to_queue(pmdblkio);
	struct maddevb_cmd *cmd;

    PINFO("maddevb_queue_bio... sector=%ld nr_sectors=%ld op=x%X\n",
          (long int)sector, (long int)nr_sectors, bio_op(bio));

	cmd = maddevb_alloc_cmd(pmdbq, 1);
	cmd->bio = bio;

	maddevb_handle_cmd(cmd, sector, nr_sectors, bio_op(bio));
	return BLK_QC_T_NONE;
}
#endif

#if 0 //static blk_status_t maddevb_queue_req(struct blk_mq_hw_ctx *hctx,
                                      const struct blk_mq_queue_data *bqd)
{
	struct maddevb_cmd *cmd = blk_mq_rq_to_pdu(bqd->rq);
	struct maddevb_queue *pmdbq = hctx->driver_data;
	PMADDEVOBJ pmaddev = (PMADDEVOBJ)pmdbq->dev->pmaddev;
	sector_t nr_sectors = blk_rq_sectors(bqd->rq);
	sector_t sector = blk_rq_pos(bqd->rq);

    PINFO("maddevb_queue_req... dev#=%d pmdbq=%px q_tg_map=%px blkmq_data=%px cmd=%px req=%px #sctrs=%d\n",
          (int)pmaddev->devnum, 
		  pmdbq, pmdbq->tag_map, bqd, cmd, bqd->rq, (int)nr_sectors);

	might_sleep_if(hctx->flags & BLK_MQ_F_BLOCKING);

	if (pmdbq->dev->irqmode == NULL_IRQ_TIMER)
        {
		hrtimer_init(&cmd->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		cmd->timer.function = maddevb_cmd_timer_expired;
	    }

	cmd->req = bqd->rq;
	cmd->error = BLK_STS_OK;
	cmd->pmdbq = pmdbq;
	ASSERT((int)(cmd->pmdbq->tag_map != NULL));

	blk_mq_start_request(bqd->rq);
    ASSERT((int)(cmd->pmdbq->tag_map != NULL));

	if (maddevb_should_requeue_request(bqd->rq))
        {
		/*
		 * Alternate between hitting the core BUSY path, and the
		 * driver driven requeue path
		 */
		pmdbq->requeue_selection++;
		if (pmdbq->requeue_selection & 1)
			return BLK_STS_RESOURCE;
		else
            {
			blk_mq_requeue_request(bqd->rq, true);
			return BLK_STS_OK;
		    }
	    }

	if (maddevb_should_timeout_request(bqd->rq))
		return BLK_STS_OK;

	return maddevb_handle_cmd(cmd, sector, nr_sectors, req_op(bqd->rq));
}
#endif
#if 0 //static void maddevb_cleanup_queue(struct maddevb_queue *pmdbq)
{
	kfree(pmdbq->tag_map);
	pmdbq->tag_map = NULL;

	kfree(pmdbq->cmds);
	pmdbq->cmds = NULL;
}
#endif