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
 * maddevb_make_cache_space
 */

extern struct mutex lock;
extern int maddevb_major;
extern struct ida maddevb_indexes;

static LIST_HEAD(maddevb_list);

static void maddevb_free_device_storage(struct maddevblk_device *dev, bool is_cache);
static struct maddevblk_device *maddevb_alloc_device(PMADDEVOBJ pmaddevobj);
static int maddevb_add_device(struct maddevblk_device *dev);
static int maddevb_rw_page(struct block_device *bdev, sector_t sector,
		                   struct page *page, unsigned int op);

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

void maddevb_delete_device(struct maddevb *maddevb);

static int maddevb_set_irqmode(const char *str, const struct kernel_param *kp)
{
	return maddevb_param_store_val(str, &g_irqmode, NULL_IRQ_NONE,
					               NULL_IRQ_TIMER);
}

static const struct kernel_param_ops maddevb_irqmode_param_ops = {
	.set	= maddevb_set_irqmode,
	.get	= param_get_int,
};

static inline struct maddevblk_device *to_maddevb_device(struct config_item *item)
{
	return item ? container_of(item, struct maddevblk_device, item) : NULL;
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
	struct maddevblk_device *dev = to_maddevb_device(item);
	bool newp = false;
	ssize_t ret;

	ret = maddevb_device_bool_attr_store(&newp, page, count);
	if (ret < 0)
		return ret;

	if (!dev->power && newp)
        {
		if (test_and_set_bit(MADDEVB_DEV_FL_UP, &dev->flags))
			return count;

        if (maddevb_add_device(dev))
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
                maddevb_delete_device(dev->pmaddevb);
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
	struct maddevblk_device *t_dev = to_maddevb_device(item);

	return badblocks_show(&t_dev->badblocks, page, 0);
}

static ssize_t maddevb_device_badblocks_store(struct config_item *item,
				                              const char *page, size_t count)
{
	struct maddevblk_device *t_dev = to_maddevb_device(item);
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

static void maddevb_free_device(struct maddevblk_device *dev)
{
    PMADDEVOBJ pmaddevobj;

	if (!dev)
        {
        PERR("maddev_free_device... dev=NULL\n");
		return;
        }

    pmaddevobj = (PMADDEVOBJ)dev->pmaddevobj;

    #ifdef MADDEVB_ZONED
	maddevb_zone_exit(dev);
    #endif
    #ifdef MADDEVB_BAD_BLOCK
	badblocks_exit(&dev->badblocks);
    #endif
	kfree(dev);

    PINFO("maddevb_free_device... dev#=%d dev=%px\n",
          (int)pmaddevobj->devnum, dev);

}

static void maddevb_device_release(struct config_item *item)
{
	struct maddevblk_device *dev = to_maddevb_device(item);
    PMADDEVOBJ pmaddevobj = (PMADDEVOBJ)dev->pmaddevobj;

    PINFO("maddevb_device_release... dev#=%d dev=%px item=%px\n",
          (int)pmaddevobj->devnum, dev, item);
	maddevb_free_device_storage(dev, false);
	maddevb_free_device(dev);
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

#if 0
static struct
config_item *maddevb_group_make_item(struct config_group *group, const char *name)
{
	struct maddevblk_device *dev = maddevb_alloc_device();
	if (dev == NULL)
        {
        PERR("maddevb_group_make_item... rc=-ENOMEM\n");
        return ERR_PTR(-ENOMEM);
        }

	config_item_init_type_name(&dev->item, name, &maddevb_device_type);

	return &dev->item;
}
#endif

static void
maddevb_group_drop_item(struct config_group *group, struct config_item *item)
{
	struct maddevblk_device *dev = to_maddevb_device(item);

	if (test_and_clear_bit(MADDEVB_DEV_FL_UP, &dev->flags)) 
        {
		mutex_lock(&lock);
		dev->power = false;
		maddevb_delete_device(dev->pmaddevb);
		mutex_unlock(&lock);
	    }

	config_item_put(item);
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

#if 0
static struct configfs_subsystem maddevb_subsys = 
{
	.su_group = {
		.cg_item = {
			.ci_namebuf = "maddevb",
			.ci_type = &maddevb_group_type,
		},
	},
};
#endif

static inline int maddevb_cache_active(struct maddevb *maddevb)
{
	return test_bit(MADDEVB_DEV_FL_CACHE, &maddevb->mbdev->flags);
}

static void maddevb_put_tag(struct maddevb_queue *nq, unsigned int tag)
{
	clear_bit_unlock(tag, nq->tag_map);

	if (waitqueue_active(&nq->wait))
		wake_up(&nq->wait);
}

static unsigned int maddevb_get_tag(struct maddevb_queue *nq)
{
	unsigned int tag;

	do {
		tag = find_first_zero_bit(nq->tag_map, nq->queue_depth);
		if (tag >= nq->queue_depth)
			return -1U;

	    } while (test_and_set_bit_lock(tag, nq->tag_map));

	return tag;
}

static void maddevb_free_cmd(struct maddevb_cmd *cmd)
{
	maddevb_put_tag(cmd->nq, cmd->tag);
}

static enum hrtimer_restart maddevb_cmd_timer_expired(struct hrtimer *timer);

static struct maddevb_cmd *__alloc_cmd(struct maddevb_queue *nq)
{
	struct maddevb_cmd *cmd;
	unsigned int tag;

	tag = maddevb_get_tag(nq);
	if (tag != -1U) 
        {
		cmd = &nq->cmds[tag];
		cmd->tag = tag;
		cmd->error = BLK_STS_OK;
		cmd->nq = nq;
		if (nq->dev->irqmode == NULL_IRQ_TIMER) 
            {
			hrtimer_init(&cmd->timer, CLOCK_MONOTONIC,
				         HRTIMER_MODE_REL);
			cmd->timer.function = maddevb_cmd_timer_expired;
		    }

        return cmd;
	    }

	return NULL;
}

static struct maddevb_cmd *maddevb_alloc_cmd(struct maddevb_queue *nq, int can_wait)
{
	struct maddevb_cmd *cmd;
	DEFINE_WAIT(wait);

	cmd = __alloc_cmd(nq);
	if (cmd || !can_wait)
		return cmd;

	do {
		prepare_to_wait(&nq->wait, &wait, TASK_UNINTERRUPTIBLE);
		cmd = __alloc_cmd(nq);
		if (cmd)
			break;

		io_schedule();
	    } while (1);

	finish_wait(&nq->wait, &wait);
	return cmd;
}

static void maddevb_end_cmd(struct maddevb_cmd *cmd)
{
	int queue_mode = cmd->nq->dev->queue_mode;
	struct mad_dev_obj *pmaddevobj = 
		               (struct mad_dev_obj *)cmd->nq->dev->pmaddevobj;

    PINFO("maddevb_end_cmd... dev#=%d cmd=%px req=%px err=%d\n",
          (int)pmaddevobj->devnum, cmd, cmd->req, (int)cmd->error);

	switch (queue_mode) 
        {
        case MADDEVB_Q_MQ:
        case MADDEVB_Q_RQ:
		    blk_mq_end_request(cmd->req, cmd->error);
		    return;

	    case MADDEVB_Q_BIO:
		    cmd->bio->bi_status = cmd->error;
		    bio_endio(cmd->bio);
		    break;

        default:
            PERR("maddevb_end_cmd... dev#=%d invalid_qmode=%d\n",
                 (int)pmaddevobj->devnum, (int)cmd->nq->dev->queue_mode);
            //return;
	    }

	maddevb_free_cmd(cmd);
}

static void maddevb_cmd_end_timer(struct maddevb_cmd *cmd)
{
	ktime_t kt = cmd->nq->dev->completion_nsec;
	struct mad_dev_obj *pmaddevobj = 
		               (struct mad_dev_obj *)cmd->nq->dev->pmaddevobj;

	PINFO("maddevb_cmd_end_timer.. dev#=%d cmd=%px\n",
		  (int)pmaddevobj->devnum, cmd);

	hrtimer_start(&cmd->timer, kt, HRTIMER_MODE_REL);
}

static void maddevb_complete_req(struct request *req)
{
    PINFO("maddevb_complete_req... req=%px\n", req);
	maddevb_end_cmd(blk_mq_rq_to_pdu(req));
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
maddevb_free_sector(struct maddevb *maddevb, sector_t sector, bool is_cache)
{
	unsigned int sector_bit;
	u64 idx;
	struct maddevb_page *t_page, *ret;
	struct radix_tree_root *root;

	root = is_cache ? &maddevb->mbdev->cache : &maddevb->mbdev->data;
	idx = sector >> PAGE_SECTORS_SHIFT;
	sector_bit = (sector & SECTOR_MASK);

	t_page = radix_tree_lookup(root, idx);
	if (t_page)
        {
		__clear_bit(sector_bit, t_page->bitmap);

		if (maddevb_page_empty(t_page)) 
            {
			ret = radix_tree_delete_item(root, idx, t_page);
			WARN_ON(ret != t_page);
			maddevb_free_page(ret);
			if (is_cache)
				maddevb->mbdev->curr_cache -= PAGE_SIZE;
		    }
	    }
}

static struct maddevb_page*
maddevb_radix_tree_insert(struct maddevb *maddevb, u64 idx, 
                          struct maddevb_page *t_page, bool is_cache)
{
	struct radix_tree_root *root;

	root = is_cache ? &maddevb->mbdev->cache : &maddevb->mbdev->data;

	if (radix_tree_insert(root, idx, t_page)) 
    {
		maddevb_free_page(t_page);
		t_page = radix_tree_lookup(root, idx);
		WARN_ON(!t_page || t_page->page->index != idx);
	}
    else if (is_cache)
		maddevb->mbdev->curr_cache += PAGE_SIZE;

	return t_page;
}

static void 
maddevb_free_device_storage(struct maddevblk_device *mbdev, bool is_cache)
{
	unsigned long pos = 0;
	int nr_pages;
	struct maddevb_page *ret, *t_pages[FREE_BATCH];
	struct radix_tree_root *root;
    PMADDEVOBJ pmaddevobj = (PMADDEVOBJ)mbdev->pmaddevobj;

	root = is_cache ? &mbdev->cache : &mbdev->data;

    PINFO("maddevb_free_device_storage... dev#=%d mbdev=%px\n",
          (int)pmaddevobj->devnum, mbdev);

	do {
		int i;

		nr_pages = radix_tree_gang_lookup(root,
				(void **)t_pages, pos, FREE_BATCH);

		for (i = 0; i < nr_pages; i++)
            {
			pos = t_pages[i]->page->index;
			ret = radix_tree_delete_item(root, pos, t_pages[i]);
			WARN_ON(ret != t_pages[i]);
			maddevb_free_page(ret);
		    }

		pos++;
	    } while (nr_pages == FREE_BATCH);

	if (is_cache)
		mbdev->curr_cache = 0;
}

static 
struct maddevb_page *__lookup_page(struct maddevb *maddevb,
                                   sector_t sector, bool for_write, bool is_cache)
{
	unsigned int sector_bit;
	u64 idx;
	struct maddevb_page *t_page;
	struct radix_tree_root *root;

	idx = sector >> PAGE_SECTORS_SHIFT;
	sector_bit = (sector & SECTOR_MASK);

	root = is_cache ? &maddevb->mbdev->cache : &maddevb->mbdev->data;
	t_page = radix_tree_lookup(root, idx);
	WARN_ON(t_page && t_page->page->index != idx);

	if (t_page && (for_write || test_bit(sector_bit, t_page->bitmap)))
		return t_page;

	return NULL;
}

static struct maddevb_page *maddevb_lookup_page(struct maddevb *maddevb,
	sector_t sector, bool for_write, bool ignore_cache)
{
	struct maddevb_page *page = NULL;

	if (!ignore_cache)
		page = __lookup_page(maddevb, sector, for_write, true);

    if (page)
		return page;

	return __lookup_page(maddevb, sector, for_write, false);
}

static struct maddevb_page *
maddevb_insert_page(struct maddevb *maddevb, sector_t sector, bool ignore_cache)
	__releases(&maddevb->lock)
	__acquires(&maddevb->lock)
{
	u64 idx;
	struct maddevb_page *t_page;

	t_page = maddevb_lookup_page(maddevb, sector, true, ignore_cache);
	if (t_page)
		return t_page;

	spin_unlock_irq(&maddevb->lock);

	t_page = maddevb_alloc_page(GFP_NOIO);
	if (!t_page)
		goto out_lock;

	if (radix_tree_preload(GFP_NOIO))
		goto out_freepage;

	spin_lock_irq(&maddevb->lock);
	idx = sector >> PAGE_SECTORS_SHIFT;
	t_page->page->index = idx;
	t_page = maddevb_radix_tree_insert(maddevb, idx, t_page, !ignore_cache);
	radix_tree_preload_end();

	return t_page;

out_freepage:
	maddevb_free_page(t_page);
out_lock:
	spin_lock_irq(&maddevb->lock);
	return maddevb_lookup_page(maddevb, sector, true, ignore_cache);
}

static int maddevb_flush_cache_page(struct maddevb *maddevb, struct maddevb_page *c_page)
{
	int i;
	unsigned int offset;
	u64 idx;
	struct maddevb_page *t_page, *ret;
	void *dst, *src;

	idx = c_page->page->index;

	t_page = maddevb_insert_page(maddevb, idx << PAGE_SECTORS_SHIFT, true);

	__clear_bit(MADDEVB_PAGE_LOCK, c_page->bitmap);
	if (test_bit(MADDEVB_PAGE_FREE, c_page->bitmap))
        {
		maddevb_free_page(c_page);
		if (t_page && maddevb_page_empty(t_page)) 
            {
			ret = radix_tree_delete_item(&maddevb->mbdev->data,
				idx, t_page);
			maddevb_free_page(t_page);
		    }

		return 0;
	    }

	if (!t_page)
        {
        PERR("maddevb_flush_cache_page... maddevb=%px rc=-ENOMEM\n", maddevb);
		return -ENOMEM;
        }

	src = kmap_atomic(c_page->page);
	dst = kmap_atomic(t_page->page);

	for (i = 0; i < PAGE_SECTORS;
		 i += ((maddevb->mbdev->blocksize >> MAD_SECTOR_SHIFT)))
        {
		if (test_bit(i, c_page->bitmap))
            {
			offset = (i << MAD_SECTOR_SHIFT);
			memcpy(dst + offset, src + offset,
				maddevb->mbdev->blocksize);
			__set_bit(i, t_page->bitmap);
		    }
	    }

	kunmap_atomic(dst);
	kunmap_atomic(src);

	ret = radix_tree_delete_item(&maddevb->mbdev->cache, idx, c_page);
	maddevb_free_page(ret);
	maddevb->mbdev->curr_cache -= PAGE_SIZE;

	return 0;
}

static int maddevb_make_cache_space(struct maddevb *maddevb, unsigned long n)
{
	int i, err, nr_pages;
	struct maddevb_page *c_pages[FREE_BATCH];
	unsigned long flushed = 0, one_round;

again:
	if ((maddevb->mbdev->cache_size * 1024 * 1024) >
	     maddevb->mbdev->curr_cache + n || maddevb->mbdev->curr_cache == 0)
		    return 0;

	nr_pages = 
    radix_tree_gang_lookup(&maddevb->mbdev->cache, (void **)c_pages,
                           maddevb->cache_flush_pos, FREE_BATCH);
	/*
	 * maddevb_flush_cache_page could unlock before using the c_pages. To
	 * avoid race, we don't allow page free
	 */
	for (i = 0; i < nr_pages; i++)
        {
		maddevb->cache_flush_pos = c_pages[i]->page->index;
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

		err = maddevb_flush_cache_page(maddevb, c_pages[i]);
		if (err)
			return err;

		one_round++;
	    }

	flushed += one_round << PAGE_SHIFT;

	if (n > flushed)
        {
		if (nr_pages == 0)
			maddevb->cache_flush_pos = 0;
		if (one_round == 0)
            {
			/* give other threads a chance */
			spin_unlock_irq(&maddevb->lock);
			spin_lock_irq(&maddevb->lock);
		    }

		goto again;
	    }

	return 0;
}

static int 
maddevb_write_waited(struct maddevb *maddevb, struct page *source,
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
		temp = min_t(size_t, maddevb->mbdev->blocksize, len - count);

		if (maddevb_cache_active(maddevb) && !is_fua)
			maddevb_make_cache_space(maddevb, PAGE_SIZE);

		offset = ((sector & SECTOR_MASK) << MAD_SECTOR_SHIFT);
		t_page = maddevb_insert_page(maddevb, sector,
			!maddevb_cache_active(maddevb) || is_fua);
		if (!t_page)
            {
            PERR("maddevb_write_waited... maddevb=%px rc=-ENOSPC\n", maddevb);
            return -ENOSPC;
            }

		src = kmap_atomic(source);
		dst = kmap_atomic(t_page->page);
		memcpy(dst + offset, src + off + count, temp);
		kunmap_atomic(dst);
		kunmap_atomic(src);

		__set_bit(sector & SECTOR_MASK, t_page->bitmap);

		if (is_fua)
			maddevb_free_sector(maddevb, sector, true);

		count += temp;
		sector += (temp >> MAD_SECTOR_SHIFT);
	    }

	return 0;
}

static int 
maddevb_read_waited(struct maddevb *maddevb, struct page *dest,
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
		temp = min_t(size_t, maddevb->mbdev->blocksize, len - count);

		offset = ((sector & SECTOR_MASK) << MAD_SECTOR_SHIFT);
		t_page = maddevb_lookup_page(maddevb, sector, false,
			                         !maddevb_cache_active(maddevb));

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

static void maddevb_handle_discard(struct maddevb *maddevb, 
                                   sector_t sector, size_t n)
{
	size_t temp;

	spin_lock_irq(&maddevb->lock);
	while (n > 0) 
        {
		temp = min_t(size_t, n, maddevb->mbdev->blocksize);
		maddevb_free_sector(maddevb, sector, false);
		if (maddevb_cache_active(maddevb))
			maddevb_free_sector(maddevb, sector, true);
		sector += (temp >> MAD_SECTOR_SHIFT);
		n -= temp;
	    }

	spin_unlock_irq(&maddevb->lock);
}

static int maddevb_handle_flush(struct maddevb *maddevb)
{
    struct mad_dev_obj *pmaddevobj = 
                       (struct mad_dev_obj *)maddevb->mbdev->pmaddevobj;
	int err = 0;

	if (!maddevb_cache_active(maddevb))
		return 0;

	spin_lock_irq(&maddevb->lock);
	while (true)
        {
		err = maddevb_make_cache_space(maddevb,
			                           maddevb->mbdev->cache_size * 1024 * 1024);
		if (err || maddevb->mbdev->curr_cache == 0)
			break;
	    }

	WARN_ON(!radix_tree_empty(&maddevb->mbdev->cache));
	spin_unlock_irq(&maddevb->lock);

    if (err != 0)
        {PERR("maddevb_handle_flush... dev=%d err=%d\n",
              pmaddevobj->devnum, err);}
	return err;
}

static int 
maddevb_transfer(struct maddevb *maddevb, struct page *page,
	             unsigned int len, unsigned int off, bool is_write,
                 sector_t sector, bool is_fua)
{
    struct mad_dev_obj *pmaddevobj = 
                       (struct mad_dev_obj *)maddevb->mbdev->pmaddevobj;
    int err = 0;

	if (!is_write)
        {
		err = maddevb_read_waited(maddevb, page, off, sector, len);
		flush_dcache_page(page);
	    }
    else
        {
		flush_dcache_page(page);
		err = maddevb_write_waited(maddevb, page, off, sector, len, is_fua);
	    }

    if (err != 0)
        {PERR("maddevb_transfer... dev#=%d wr=%d err=%d\n", 
              (int)pmaddevobj->devnum, is_write, (int)err);}

	return err;
}

static int maddevb_handle_req(struct maddevb_cmd *cmd)
{
	struct request *req = cmd->req;
    sector_t sector = blk_rq_pos(req);
    struct maddevb *pmaddevb = cmd->nq->dev->pmaddevb;
    struct mad_dev_obj *pmaddevobj = 
                       (struct mad_dev_obj*)cmd->nq->dev->pmaddevobj;

    int err;
	unsigned int len;
	struct req_iterator iter;
	struct bio_vec bvec;

	PINFO("maddevb_handle_req dev#=%d cmd=%px req=%px sector=%ld\n", 
          (int)pmaddevobj->devnum, cmd, req, sector);

	if (req_op(req) == REQ_OP_DISCARD)
        {
		maddevb_handle_discard(pmaddevb, sector, blk_rq_bytes(req));
		return 0;
	    }

	spin_lock_irq(&pmaddevb->lock);
	rq_for_each_segment(bvec, req, iter)
        {
		len = bvec.bv_len;
		err = maddevb_transfer(pmaddevb, bvec.bv_page, len, bvec.bv_offset,
				               op_is_write(req_op(req)), sector,
				               req->cmd_flags & REQ_FUA);
		if (err)
            {
			spin_unlock_irq(&pmaddevb->lock);
			//return err;
            break;
		    }
		sector += (len >> MAD_SECTOR_SHIFT);
	    }
	spin_unlock_irq(&pmaddevb->lock);

    if (err != 0)
        {PERR("maddevb_handle_req... dev#=%d req=%px err=%d\n", 
              (int)pmaddevobj->devnum, req, (int)err);}

	return err;
}

static int maddevb_handle_bio(struct maddevb_cmd *cmd)
{
	struct bio *bio = cmd->bio;
	struct maddevb *pmaddevb = cmd->nq->dev->pmaddevb;
	int err;
	unsigned int len;
	sector_t sector;
	struct bio_vec bvec;
	struct bvec_iter iter;

	sector = bio->bi_iter.bi_sector;

	if (bio_op(bio) == REQ_OP_DISCARD)
        {
		maddevb_handle_discard(pmaddevb, sector,
			                   (bio_sectors(bio) << MAD_SECTOR_SHIFT));
		return 0;
	    }

	spin_lock_irq(&pmaddevb->lock);
	bio_for_each_segment(bvec, bio, iter)
        {
		len = bvec.bv_len;
		err = maddevb_transfer(pmaddevb, bvec.bv_page, len, bvec.bv_offset,
				               op_is_write(bio_op(bio)), sector,
				               bio->bi_opf & REQ_FUA);
		if (err)
            {
			spin_unlock_irq(&pmaddevb->lock);
			return err;
		    }
		sector += (len >> MAD_SECTOR_SHIFT);
	    }
	spin_unlock_irq(&pmaddevb->lock);
	return 0;
}

static void maddevb_stop_queue(struct maddevb *pmaddevb)
{
	struct request_queue *reqQ = pmaddevb->reqQ;

	if (pmaddevb->mbdev->queue_mode == MADDEVB_Q_MQ)
		blk_mq_stop_hw_queues(reqQ);
}

void maddevb_restart_queue_async(struct maddevb *pmaddevb)
{
	struct request_queue *reqQ = pmaddevb->reqQ;

	if (pmaddevb->mbdev->queue_mode == MADDEVB_Q_MQ)
		blk_mq_start_stopped_hw_queues(reqQ, true);
}

static inline blk_status_t maddevb_handle_throttled(struct maddevb_cmd *cmd)
{
	struct maddevblk_device *mbdev = cmd->nq->dev;
	struct maddevb *maddevb = mbdev->pmaddevb;
	blk_status_t sts = BLK_STS_OK;
	struct request *req = cmd->req;

	if (!hrtimer_active(&maddevb->bw_timer))
		hrtimer_restart(&maddevb->bw_timer);

	if (atomic_long_sub_return(blk_rq_bytes(req), &maddevb->cur_bytes) < 0)
        {
		maddevb_stop_queue(maddevb);
		/* race with timer */
		if (atomic_long_read(&maddevb->cur_bytes) > 0)
			maddevb_restart_queue_async(maddevb);
		/* requeue request */
		sts = BLK_STS_DEV_RESOURCE;
	    }
	return sts;
}

#ifdef MADDEVB_BAD_BLOCK
static inline blk_status_t maddevb_handle_badblocks(struct maddevb_cmd *cmd,
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
maddevb_handle_memory_backed(struct maddevb_cmd *cmd, enum req_opf op)
{
	struct maddevblk_device *mbdev = cmd->nq->dev;
    struct maddevb *pmaddevb = mbdev->pmaddevb;
    struct mad_dev_obj *pmaddevobj = (struct mad_dev_obj *)mbdev->pmaddevobj;
    int err = 0;

	if (mbdev->queue_mode == MADDEVB_Q_BIO)
		err = maddevb_handle_bio(cmd);
	else
		err = maddevb_handle_req(cmd);

    if (err != 0)
        {PERR("maddevb_handle_memory_backed... dev#=%d req=%px err=%d\n", 
              (int)pmaddevobj->devnum, cmd->req, (int)err);}

	return errno_to_blk_status(err);
}

static inline void maddevb_complete_cmd(struct maddevb_cmd *cmd)
{
    struct mad_dev_obj *pmaddevobj = 
		               (struct mad_dev_obj *)cmd->nq->dev->pmaddevobj;

	PINFO("maddevb_complete_cmd... dev#=%d cmd=%px req=%px\n",
		  (int)pmaddevobj->devnum, cmd, cmd->req);

	/* Complete IO by inline, softirq or timer */
	switch (cmd->nq->dev->irqmode)
        {
	    case NULL_IRQ_SOFTIRQ:
		    switch (cmd->nq->dev->queue_mode)
                {
		        case MADDEVB_Q_MQ:
				case MADDEVB_Q_RQ:
				    blk_mq_complete_request(cmd->req);
			        break;

		        case MADDEVB_Q_BIO:
			    /*
			     * XXX: no proper submitting cpu information available.
			     */
			        maddevb_end_cmd(cmd);
			        break;

                default:
                    PERR("maddevb_complete_cmd... dev#=%d invalid_qmode=%d\n",
                         (int)pmaddevobj->devnum, (int)cmd->nq->dev->queue_mode);
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
                 (int)pmaddevobj->devnum, (int)cmd->nq->dev->irqmode);
	    }
}

int 
maddevb_process_bio_command(struct mad_dev_obj *pmaddevobj, struct bio* pbio,
                            sector_t Sector, sector_t nr_sectors,
                            enum req_opf op)
{
    sector_t tsector = Sector;
    int      iorc = 0;
    int      nr_bvecs = 0;
    struct   bio_vec biovecs[MAD_SGDMA_MAX_SECTORS];
    ssize_t  iocount;

    mutex_lock(&pmaddevobj->devmutex);

    PINFO("maddevb_process_bio_command... dev#=%d op=%d bio=%px bio_next=%px #biovecs=%d biovecs=%px\n",
          pmaddevobj->devnum, op, pbio, pbio->bi_next, 
          pbio->bi_vcnt, pbio->bi_io_vec);

    nr_bvecs = maddevb_collect_biovecs(pmaddevobj, pbio, nr_sectors, biovecs);
    if ((nr_bvecs == 0) || (nr_bvecs > MAD_SGDMA_MAX_SECTORS))
        {
        ASSERT((int)false);
        mutex_unlock(&pmaddevobj->devmutex);
        return -EINVAL;
        }

    iocount = 
    maddevb_xfer_sgdma_bvecs(pmaddevobj, biovecs, 
                             nr_bvecs, tsector, nr_sectors, (op==REQ_OP_WRITE));
    if (iocount < 0)
        {iorc = (int)iocount;}

    if (iorc != 0)
        {PERR("maddevb_process_bio_command... dev#=%d iorc=%d\n",
              (int)pmaddevobj->devnum, iorc);}

    mutex_unlock(&pmaddevobj->devmutex);

    return iorc;
}

static blk_status_t
maddevb_handle_cmd(struct maddevb_cmd *cmd, sector_t sector,
                   sector_t nr_sectors, enum req_opf op)
{
	struct maddevblk_device *dev = cmd->nq->dev;
	struct mad_dev_obj *pmaddevobj = (struct mad_dev_obj *)dev->pmaddevobj;
	struct maddevb *pmaddevb = dev->pmaddevb;
	blk_status_t sts = BLK_STS_OK;
    int err = -1;

	PINFO("maddevb_handle_cmd... dev#=%d cmd=%px req=%px bio=%px sector=%d #sctrs=%d op=%d\n",
          pmaddevobj->devnum, cmd, cmd->req, cmd->req->bio,
          (int)sector, (int)nr_sectors, op);

	if (test_bit(MADDEVB_DEV_FL_THROTTLED, &dev->flags))
        {
		sts = maddevb_handle_throttled(cmd);
		if (sts != BLK_STS_OK)
			return sts;
	    }

    #ifdef MADDEVB_BAD_BLOCK
	if (maddevb->dev->badblocks.shift != -1)
        {
		cmd->error = sts = maddevb_handle_badblocks(cmd, sector, nr_sectors);
		if (cmd->error != BLK_STS_OK)
			goto out;
	    }
    #endif

	if (dev->memory_backed)
        {
		cmd->error = sts = maddevb_handle_memory_backed(cmd, op);
        goto out;
        }

    #ifdef MADDEVB_ZONED
	if (!cmd->error && dev->zoned) 
        {
		cmd->error = sts = maddevb_handle_zoned(cmd, op, sector, nr_sectors);
        goto out;
        }
    #endif

    switch (op)
        {
        case REQ_OP_READ:
        case REQ_OP_WRITE:
            err = maddevb_process_bio_command(pmaddevobj, cmd->req->bio,
                                              sector, nr_sectors, op);
            cmd->error = sts = errno_to_blk_status(err);
            break;

        case REQ_OP_FLUSH:
            err = maddevb_handle_flush(pmaddevb);
            cmd->error = sts = errno_to_blk_status(err);
            break;

        default:
            PWARN("maddevb_handle_cmd... dev#=%d cmd=%px req=%px op=%d sts=BLK_STS_NOTSUPP\n",
                  pmaddevobj->devnum, cmd, cmd->req, op);
            cmd->error = sts = BLK_STS_NOTSUPP;
        }

out:
    if (sts != BLK_STS_OK)
        {PERR("maddevb_complete_cmd... dev#=%d err=%d blkstat=%d\n",
              (int)pmaddevobj->devnum, err, sts);}

	maddevb_complete_cmd(cmd);
    return sts;
}

static enum hrtimer_restart maddevb_bwtimer_fn(struct hrtimer *timer)
{
	struct maddevb *maddevb = container_of(timer, struct maddevb, bw_timer);
	ktime_t timer_interval = ktime_set(0, TIMER_INTERVAL);
	unsigned int mbps = maddevb->mbdev->mbps;

	if (atomic_long_read(&maddevb->cur_bytes) == mb_per_tick(mbps))
		return HRTIMER_NORESTART;

	atomic_long_set(&maddevb->cur_bytes, mb_per_tick(mbps));
	maddevb_restart_queue_async(maddevb);

	hrtimer_forward_now(&maddevb->bw_timer, timer_interval);

	return HRTIMER_RESTART;
}

static void maddevb_setup_bwtimer(struct maddevb *maddevb)
{
	ktime_t timer_interval = ktime_set(0, TIMER_INTERVAL);

	hrtimer_init(&maddevb->bw_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	maddevb->bw_timer.function = maddevb_bwtimer_fn;
	atomic_long_set(&maddevb->cur_bytes, mb_per_tick(maddevb->mbdev->mbps));
	hrtimer_start(&maddevb->bw_timer, timer_interval, HRTIMER_MODE_REL);
}

static struct maddevb_queue *maddevb_to_queue(struct maddevb *maddevb)
{
	int index = 0;

	if (maddevb->nr_queues != 1)
		index = raw_smp_processor_id() / ((nr_cpu_ids + maddevb->nr_queues - 1) / maddevb->nr_queues);

	return &maddevb->queues[index];
}

static blk_qc_t maddevb_queue_bio(struct request_queue *q, struct bio *bio)
{
	sector_t sector = bio->bi_iter.bi_sector;
	sector_t nr_sectors = bio_sectors(bio);
	struct maddevb *maddevb = q->queuedata;
	struct maddevb_queue *nq = maddevb_to_queue(maddevb);
	struct maddevb_cmd *cmd;

    PINFO("maddevb_queue_bio... sector=%ld nr_sectors=%ld op=x%X\n",
          (long int)sector, (long int)nr_sectors, bio_op(bio));

	cmd = maddevb_alloc_cmd(nq, 1);
	cmd->bio = bio;

	maddevb_handle_cmd(cmd, sector, nr_sectors, bio_op(bio));
	return BLK_QC_T_NONE;
}

static bool maddevb_should_timeout_request(struct request *rq)
{
#ifdef CONFIG_BLK_DEV_NULL_BLK_FAULT_INJECTION
	if (g_timeout_str[0])
		return should_fail(&maddevb_timeout_attr, 1);
#endif
	return false;
}

static bool maddevb_should_requeue_request(struct request *rq)
{
#ifdef CONFIG_BLK_DEV_NULL_BLK_FAULT_INJECTION
	if (g_requeue_str[0])
		return should_fail(&maddevb_requeue_attr, 1);
#endif
	return false;
}

static enum blk_eh_timer_return maddevb_timeout_req(struct request *req, bool res)
{
	PINFO("maddevb_timeout_req... req(%px) timed out\n", req);

	blk_mq_complete_request(req);
	return BLK_EH_DONE;
}

static blk_status_t maddevb_queue_req(struct blk_mq_hw_ctx *hctx,
                                      const struct blk_mq_queue_data *bd)
{
	struct maddevb_cmd *cmd = blk_mq_rq_to_pdu(bd->rq);
	struct maddevb_queue *nq = hctx->driver_data;
	sector_t nr_sectors = blk_rq_sectors(bd->rq);
	sector_t sector = blk_rq_pos(bd->rq);

    PINFO("maddevb_queue_req... hctx=%px blkmq_data=%px cmd=%px req=%px #sctrs=%d\n",
          hctx, bd, cmd, bd->rq, (int)nr_sectors);

	might_sleep_if(hctx->flags & BLK_MQ_F_BLOCKING);

	if (nq->dev->irqmode == NULL_IRQ_TIMER)
        {
		hrtimer_init(&cmd->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		cmd->timer.function = maddevb_cmd_timer_expired;
	    }

	cmd->req = bd->rq;
	cmd->error = BLK_STS_OK;
	cmd->nq = nq;

	blk_mq_start_request(bd->rq);

	if (maddevb_should_requeue_request(bd->rq))
        {
		/*
		 * Alternate between hitting the core BUSY path, and the
		 * driver driven requeue path
		 */
		nq->requeue_selection++;
		if (nq->requeue_selection & 1)
			return BLK_STS_RESOURCE;
		else
            {
			blk_mq_requeue_request(bd->rq, true);
			return BLK_STS_OK;
		    }
	    }

	if (maddevb_should_timeout_request(bd->rq))
		return BLK_STS_OK;

	return maddevb_handle_cmd(cmd, sector, nr_sectors, req_op(bd->rq));
}

static const struct blk_mq_ops maddevb_mq_ops = {
	.queue_rq   = maddevb_queue_req,
	.complete	= maddevb_complete_req,
	.timeout	= maddevb_timeout_req,
};

static void maddevb_cleanup_queue(struct maddevb_queue *nq)
{
	kfree(nq->tag_map);
	kfree(nq->cmds);
}

void maddevb_cleanup_queues(struct maddevb *maddevb)
{
	int i;
    PMADDEVOBJ pmaddevobj = maddevb->mbdev->pmaddevobj;

	for (i = 0; i < maddevb->nr_queues; i++)
		maddevb_cleanup_queue(&maddevb->queues[i]);

	kfree(maddevb->queues);

    PINFO("maddevb_cleanup_queues... mbdev#=%d dev=%px #Qs=%d\n",
          pmaddevobj->devnum, maddevb->mbdev, (int)maddevb->nr_queues);
}

void maddevb_delete_device(struct maddevb *maddevb)
{
	struct maddevblk_device *mbdev;
    PMADDEVOBJ pmaddevobj = NULL;

	if (maddevb != NULL)
        {
        mbdev = maddevb->mbdev;
        if (mbdev != NULL)
            pmaddevobj = (PMADDEVOBJ)mbdev->pmaddevobj;
        }

    if (pmaddevobj == NULL) 
        {
        PERR("maddevb_delete_device... BAD ADDRESS! maddevb=%px mbdev=#px pmaddevobj=%px\n");
		return;
        }

    PDEBUG("maddevb_delete_device... dev#=%d mbdev=%px maddevb=%px\n",
           pmaddevobj->devnum, mbdev, maddevb);

	//ida_simple_remove(&maddevb_indexes, (maddevb->index));

	list_del_init(&maddevb->list);

	del_gendisk(maddevb->disk);

	if (test_bit(MADDEVB_DEV_FL_THROTTLED, &maddevb->mbdev->flags))
        {
		hrtimer_cancel(&maddevb->bw_timer);
		atomic_long_set(&maddevb->cur_bytes, LONG_MAX);
		maddevb_restart_queue_async(maddevb);
	    }

	blk_cleanup_queue(maddevb->reqQ);
	if (mbdev->queue_mode == MADDEVB_Q_MQ &&
	    maddevb->tag_set == &maddevb->__tag_set)
		    blk_mq_free_tag_set(maddevb->tag_set);

	put_disk(maddevb->disk);
	maddevb_cleanup_queues(maddevb);

	if (maddevb_cache_active(maddevb))
		maddevb_free_device_storage(maddevb->mbdev, true);

	kfree(maddevb);
	mbdev->pmaddevb = NULL;

    PINFO("maddevb_delete_device exit... mbdev#=%d dev=%px maddevb=%px\n",
          pmaddevobj->devnum, mbdev, maddevb);
}

static const struct block_device_operations maddevb_fops =
{
	.owner           = THIS_MODULE,
	.open            = maddevb_open,
	.release         = maddevb_release,
    .rw_page         = maddevb_rw_page,
    .getgeo          = maddevb_getgeo,
    .ioctl           = maddevb_ioctl,
    .compat_ioctl    = maddevb_ioctl,
    .revalidate_disk = maddevb_revalidate_disk,
    .swap_slot_free_notify 
                     = maddevb_swap_slot_free_notify,
    .report_zones    = maddevb_zone_report,
};

static struct maddevblk_device *maddevb_alloc_device(PMADDEVOBJ pmaddevobj)
{
	struct maddevblk_device *mbdev;

	mbdev = kzalloc(sizeof(struct maddevblk_device), GFP_KERNEL);
	if (mbdev == NULL)
        {
        PERR("maddevblk_alloc_device... returning NULL\n");
		return NULL;
        }

	INIT_RADIX_TREE(&mbdev->data, GFP_ATOMIC);
	INIT_RADIX_TREE(&mbdev->cache, GFP_ATOMIC);

    #ifdef _MADDEVB_BAD_BLOCK
	if (badblocks_init(&mbdev->badblocks, 0))
        {
		kfree(dev);
		return NULL;
	    }
    #endif

	mbdev->size = (g_gs * g_bs);
	mbdev->completion_nsec = g_completion_nsec;
	mbdev->submit_queues = g_submit_queues;
	mbdev->home_node = g_home_node;
	mbdev->queue_mode = g_queue_mode;
	mbdev->blocksize = g_bs;
	mbdev->irqmode = g_irqmode;
	mbdev->hw_queue_depth = g_hw_queue_depth;
    mbdev->memory_backed = false;
    mbdev->blocking = g_blocking;
	mbdev->blocking = false;
	mbdev->cache_size = 0;

	mbdev->use_per_node_hctx = g_use_per_node_hctx;
    mbdev->pmaddevobj = pmaddevobj;

   	mbdev->zoned = false; //g_zoned;
    mbdev->nr_zones = 0;

    #ifdef MADDEVB_ZONED
    mbdev->zoned = g_zoned;
	mbdev->zone_size = g_zone_size;
	mbdev->zone_nr_conv = g_zone_nr_conv;
    #endif

    PINFO("maddevblk_alloc_device... mbdev#=%d dev=%px\n",
          pmaddevobj->devnum,mbdev);
    return mbdev;
}

static int maddevb_gendisk_register(struct maddevb *maddevb)
{
	struct gendisk *disk;
	sector_t size;
    struct block_device *bdev;
    PMADDEVOBJ pmaddevobj = maddevb->mbdev->pmaddevobj;
    int ret = 0;

    PINFO("maddevb_gendisk_register... dev#=%d maddevb=%px\n",
          pmaddevobj->devnum, maddevb);

	disk = alloc_disk_node(1, maddevb->mbdev->home_node);
	if (disk == NULL)
        {
		ret = -ENOMEM;
        goto gendisk_reg_out;
        }

    maddevb->disk = disk;
    size = (sector_t)maddevb->mbdev->size; // * 1024 * 1024ULL;
	set_capacity(disk, (size >> MAD_SECTOR_SHIFT)); //9)); //sectors

	disk->flags       |= MADDEVB_GENHD_FLAGS;
	disk->major		   = maddevb_major;
	disk->first_minor  = maddevb->index;
	disk->fops		   = &maddevb_fops;
	disk->private_data = maddevb;
	disk->queue		   = maddevb->reqQ;
	strncpy(disk->disk_name, maddevb->disk_name, DISK_NAME_LEN);

    #ifdef MADDEVB_ZONED
	if (maddevb->dev->zoned) 
        {
		ret = blk_revalidate_disk_zones(disk);
		if (ret != 0)
			goto gendisk_reg_out;
	    }
    #endif

    bdev = bdget_disk(disk, 0);
    bdev->bd_disk = disk;

	add_disk(disk); //Calls revalidate_disk()

gendisk_reg_out:;    
    PINFO("maddevb_gendisk_register... maddevb=%px disk=%px rc=%d\n",
          maddevb, disk, ret);
    return ret;
}

static int maddevb_add_device(struct maddevblk_device *mbdev)
{
	struct maddevb* pmaddevb;
    PMADDEVOBJ pmaddevobj;
	int rv;

    PINFO("maddevb_add_device... mbdev=%px\n", mbdev);

    maddevb_validate_conf(mbdev);

	pmaddevb = 
    kzalloc_node(sizeof(struct maddevb), GFP_KERNEL, mbdev->home_node);
	if (!pmaddevb)
        {
		rv = -ENOMEM;
		goto out;
	    }

    pmaddevb->mbdev = mbdev;
	mbdev->pmaddevb = pmaddevb;
    pmaddevobj = (PMADDEVOBJ)mbdev->pmaddevobj;

	spin_lock_init(&pmaddevb->lock);

	rv = maddevb_setup_queues(pmaddevb);
	if (rv)
		goto out_free_maddevb;

    switch (mbdev->queue_mode)
        {
        case MADDEVB_Q_RQ:
        case MADDEVB_Q_MQ:
            if (shared_tags)
                {
                pmaddevb->tag_set = &tag_set;
                rv = 0;
                }
            else
                {
                pmaddevb->tag_set = &pmaddevb->__tag_set;
                rv = maddevb_init_tag_set(pmaddevb, pmaddevb->tag_set);
                }

            if (rv)
                goto out_cleanup_queues;

            if (!maddevb_setup_fault())
                goto out_cleanup_queues;

            pmaddevb->tag_set->timeout = 5 * HZ;
            pmaddevb->reqQ = blk_mq_init_queue(pmaddevb->tag_set);
            if (IS_ERR(pmaddevb->reqQ))
                {
                rv = -ENOMEM;
                goto out_cleanup_tags;
                }

            maddevb_init_queues(pmaddevb);
            break;

		case MADDEVB_Q_BIO:
			#if 0
            pmaddevb->reqQ = blk_alloc_queue_node(GFP_KERNEL, dev->home_node);
            if (!pmaddevb->reqQ)
                {
                rv = -ENOMEM;
                goto out_cleanup_queues;
                }

            blk_queue_make_request(pmaddevb->reqQ, maddevb_queue_bio);
			#endif
            rv = maddevb_init_driver_queues(pmaddevb);
            if (rv)
                goto out_cleanup_blk_queue;
            break;

        default:
            PWARN("maddevb_add_device... mbdev#=%d unsupported Qmode=%d\n",
                  pmaddevobj->devnum, mbdev->queue_mode);
        }

	if (mbdev->mbps) 
        {
		set_bit(MADDEVB_DEV_FL_THROTTLED, &mbdev->flags);
		maddevb_setup_bwtimer(pmaddevb);
	    }

	if (mbdev->cache_size > 0)
        {
		set_bit(MADDEVB_DEV_FL_CACHE, &pmaddevb->mbdev->flags);
		blk_queue_write_cache(pmaddevb->reqQ, true, true);
	    }

    #ifdef MADDEVB_ZONED
	if (mbdev->zoned) 
        {
		rv = maddevb_zone_init(mbdev);
		if (rv)
			goto out_cleanup_blk_queue;

		blk_queue_chunk_sectors(pmaddevb->q, mbdev->zone_size_sects);
		pmaddevb->q->limits.zoned = BLK_ZONED_HM;
		blk_queue_flag_set(QUEUE_FLAG_ZONE_RESETALL, pmaddevb->reqQ);
		blk_queue_required_elevator_features(pmaddevb->reqQ,
						                     ELEVATOR_F_ZBD_SEQ_WRITE);
	    }
    #endif

	pmaddevb->reqQ->queuedata = pmaddevb;
	blk_queue_flag_set(QUEUE_FLAG_NONROT, pmaddevb->reqQ);
	blk_queue_flag_clear(QUEUE_FLAG_ADD_RANDOM, pmaddevb->reqQ);

	mutex_lock(&lock);
    //unsigned int indx = ida_simple_get(&maddevb_indexes,0,0,GFP_KERNEL);
    pmaddevb->index = pmaddevobj->devnum; //numindx + 1;
    mbdev->index = pmaddevb->index;
	mutex_unlock(&lock);

	blk_queue_logical_block_size(pmaddevb->reqQ, mbdev->blocksize);
	blk_queue_physical_block_size(pmaddevb->reqQ, mbdev->blocksize);

	maddevb_config_discard(pmaddevb);

	sprintf(pmaddevb->disk_name, "fd%d", pmaddevb->index);

	rv = maddevb_gendisk_register(pmaddevb);

	if (rv)
		goto out_cleanup_zone;

	mutex_lock(&lock);
	list_add_tail(&pmaddevb->list, &maddevb_list);
	mutex_unlock(&lock);

	return 0;

out_cleanup_zone:
    #ifdef MADDEVB_ZONED
	if (mbdev->zoned)
		maddevb_zone_exit(dev);
    #endif

out_cleanup_blk_queue:
	blk_cleanup_queue(pmaddevb->reqQ);

out_cleanup_tags:
	if (mbdev->queue_mode == MADDEVB_Q_MQ && pmaddevb->tag_set == &pmaddevb->__tag_set)
		blk_mq_free_tag_set(pmaddevb->tag_set);

out_cleanup_queues:
	maddevb_cleanup_queues(pmaddevb);

out_free_maddevb:
	kfree(pmaddevb);
	mbdev->pmaddevb = NULL;
out:
    PERR("maddevb_add_device... mbdev#=%d dev=%px maddevb=%px rc=%d\n",
         (int)pmaddevobj->devnum,  mbdev, pmaddevb, rv);

	return rv;
}
 
int maddevb_create_device(/*PMADDEVOBJ pmaddevobj*/void* pv)
{
    PMADDEVOBJ pmaddevobj = (PMADDEVOBJ)pv;
    int rc = 0;

    PINFO("maddevb_create_device... dev#=%d\n", (int)pmaddevobj->devnum);

    pmaddevobj->maddevblk_dev = maddevb_alloc_device(pmaddevobj);
	if (pmaddevobj->maddevblk_dev == NULL)
        {rc = -ENOMEM;}

    if (rc == 0)
        {rc = maddevb_add_device(pmaddevobj->maddevblk_dev);}

	if (rc != 0)
        {
        PERR("maddevb_create_device... mbdev#=%d rc=%d\n",
             (int)pmaddevobj->devnum, rc);
        maddevb_free_device(pmaddevobj->maddevblk_dev);
        }

    return rc;
}  

static void maddevb_config_discard(struct maddevb *maddevb)
{
    PMADDEVOBJ pmaddevobj = maddevb->mbdev->pmaddevobj;

    if (maddevb->mbdev->discard == false)
        return;

	maddevb->reqQ->limits.discard_granularity = maddevb->mbdev->blocksize;
	maddevb->reqQ->limits.discard_alignment = maddevb->mbdev->blocksize;
	blk_queue_max_discard_sectors(maddevb->reqQ, UINT_MAX >> 9);
	blk_queue_flag_set(QUEUE_FLAG_DISCARD, maddevb->reqQ);

    PINFO("maddevb_config_discard... dev#=%d rq=%px\n",
          (int)pmaddevobj->devnum, maddevb->reqQ);
}

static int maddevb_rw_page(struct block_device *bdev, sector_t sector,
		                    struct page *pPage, unsigned int op)
{
	//struct pmem_device *pmem = bdev->bd_queue->queuedata;
    PMADDEVOBJ pmaddevobj = maddevb_get_parent_from_bdev(bdev);

    //u32 num_pgs = hpage_nr_pages(pPages); 
    //struct page*  *page_list = pPages;
    bool bWrite = op_is_write(op);
    bool bSgDma = true;
    ssize_t iocount = 0;
    blk_status_t rc = 0;
    u32 j;

    mutex_lock(&pmaddevobj->devmutex);

    PINFO("maddevb_rw_page... dev#=%d bdev=%px PA=x%llX sector=%ld op=%d\n",
          pmaddevobj->devnum, bdev, page_to_phys(pPage), (long int)sector, op);

    iocount = maddev_xfer_dma_page(pmaddevobj, pPage, sector, bWrite);
    if (iocount < 0)
        {rc = (int)iocount;}

    //Specific to a read - each page needs to be marked 'dirty' (updated)
    // *if* it is not in the reserved region - (never swapped out to disk)
    //PDEBUG("maddevb_rw_page... set the dirty bit if read page(s) reserved? wr=%d\n",
    //       bWrite);
    if (bWrite==false)
        if (!PageReserved(pPage))
            {SetPageDirty(pPage);}

	/*
	 * The ->rw_page interface is subtle and tricky.  The core
	 * retries on any error, so we can only invoke page_endio() in
	 * the successful completion case.  Otherwise, we'll see crashes
	 * caused by double completion.
	 */
	if (rc == 0)
	    page_endio(pPage, op_is_write(op), 0);

    PINFO("maddevb_rw_page... dev#=%d rc=%d iocount=%ld\n",
          (int)pmaddevobj->devnum, rc, (long int)iocount);

    BUG_ON(rc != 0);
    mutex_unlock(&pmaddevobj->devmutex);

    return rc;
}

static int maddevb_open(struct block_device *bdev, fmode_t mode)
{
    PMADDEVOBJ pmaddevobj = maddevb_get_parent_from_bdev(bdev);
    int wrc = 0;

    ASSERT((int)(pmaddevobj != NULL));
    ASSERT((int)(pmaddevobj->maddevblk_dev->pmaddevb->disk == bdev->bd_disk));
    PINFO("maddevb_open... dev#=%d bdev=%px disk=%px Qctx=%px mode=%X\n", 
          (int)pmaddevobj->devnum, bdev, bdev->bd_disk, 
          bdev->bd_queue->queuedata, mode);

    mutex_lock(&pmaddevobj->devmutex);

	if (pmaddevobj->bReady != true)
	    {
        mutex_unlock(&pmaddevobj->devmutex);
        PWARN("maddevb_open... dev#=%d not initialized!\n", (int)pmaddevobj->devnum);
        ASSERT((int)false);
        BUG_ON(true);
		return -ENODEV;
	    }

    mutex_unlock(&pmaddevobj->devmutex);

    //PDEBUG("maddevb_open... dev#=%d inode=%px bdev_info=%px exit :)\n", 
    //       (int)pmaddevobj->devnum, bdev->bd_inode, bdev->bd_bdi);

    return 0;
}

static void maddevb_release(struct gendisk *disk, fmode_t mode)
{
    struct block_device *bdev = bdget_disk(disk, 0);
    PMADDEVOBJ pmaddevobj = maddevb_get_parent_from_bdev(bdev);

    PINFO("maddevb_release... dev#=%d disk=%px bdev=%px mode=x%X\n",
          (int)pmaddevobj->devnum, disk, bdev, mode);

    mutex_lock(&pmaddevobj->devmutex);
    //Nothing to do
    mutex_unlock(&pmaddevobj->devmutex);

    return;
}

static int maddevb_revalidate_disk(struct gendisk *disk)
{
    struct block_device *bdev = bdget_disk(disk, 0);
    PMADDEVOBJ pmaddevobj = maddevb_get_parent_from_bdev(bdev);

    PINFO("maddevb_revalidate_disk... dev#=%d disk=%px bdev=%px\n",
          (int)pmaddevobj->devnum, disk, bdev);

    mutex_lock(&pmaddevobj->devmutex);
    //Nothing to do
    mutex_unlock(&pmaddevobj->devmutex);

    return 0;
}

static int maddevb_getgeo(struct block_device* bdev, struct hd_geometry* geo)
{
    PMADDEVOBJ pmaddevobj = maddevb_get_parent_from_bdev(bdev);

    PINFO("maddevb_getgeo... dev#=%d bdev=%px\n",
          (int)pmaddevobj->devnum, bdev);

    mutex_lock(&pmaddevobj->devmutex);
    geo->heads     = (u8)(1 << 6);
    geo->sectors   = (u8)(1 << 5);
    geo->cylinders = (u16)(get_capacity(bdev->bd_disk) >> 11);
    geo->start     = 0;
    mutex_unlock(&pmaddevobj->devmutex);

    return 0;
}

static void maddevb_swap_slot_free_notify(struct block_device* bdev,
                                          unsigned long offset)
{
    PMADDEVOBJ pmaddevobj = maddevb_get_parent_from_bdev(bdev);

    PINFO("maddevb_swap_slot_free_notify... dev#=%d bdev=%px offset=%ld\n",
          (int)pmaddevobj->devnum, bdev, offset);

    //Nothing to do
    return;
}

//static int maddevb_zone_report(struct gendisk *disk, sector_t sector,
//		                       struct blk_zone *zones, unsigned int *nr_zones)
static int maddevb_zone_report(struct gendisk *disk, sector_t sector,
		                       /*struct blk_zone *zones,*/ 
							   unsigned int nr_zones, report_zones_cb cb, void* data)
{
	//struct nullb *nullb = disk->private_data;
	//struct nullb_device *dev = nullb->dev;
	//unsigned int zno, nrz = 0;
    struct block_device *bdev = bdget_disk(disk, 0);
    struct maddevb *maddevb = bdev->bd_disk->private_data;
    PMADDEVOBJ pmaddevobj   = (PMADDEVOBJ)maddevb->mbdev->pmaddevobj;
 
     PINFO("maddevb_zone_report... dev#=%d bdev=%px nr_zones=%d\n",
           (int)pmaddevobj->devnum, bdev, maddevb->mbdev->nr_zones);

     //*nr_zones = maddevb->dev->nr_zones;

	/*zno = null_zone_no(dev, sector);
	if (zno < dev->nr_zones) {
		nrz = min_t(unsigned int, *nr_zones, dev->nr_zones - zno);
		memcpy(zones, &dev->zones[zno], nrz * sizeof(struct blk_zone));
	} */

	return 0;
}

//
static void maddevb_init_queue(struct maddevb *maddevb, struct maddevb_queue *nq)
{
	BUG_ON(!maddevb);
	BUG_ON(!nq);

	init_waitqueue_head(&nq->wait);
	nq->queue_depth = maddevb->queue_depth;
	nq->dev = maddevb->mbdev;
}

static void maddevb_init_queues(struct maddevb *maddevb)
{
	struct request_queue *reqQ = maddevb->reqQ;
	struct blk_mq_hw_ctx *hctx;
	struct maddevb_queue *nq;
	int i;

    PMADDEVOBJ pmaddevobj = (PMADDEVOBJ)maddevb->mbdev->pmaddevobj;

	queue_for_each_hw_ctx(reqQ, hctx, i)
        {
		if (!hctx->nr_ctx || !hctx->tags)
			continue;

        nq = &maddevb->queues[i];
		hctx->driver_data = nq;
		maddevb_init_queue(maddevb, nq);
		maddevb->nr_queues++;
	    }

    PINFO("maddevb_init_queues... dev#=%d rq=%px nr_qs=%d\n",
           (int)pmaddevobj->devnum, reqQ, (int)maddevb->nr_queues);
}

static int maddevb_setup_commands(struct maddevb_queue *nq)
{
	struct maddevb_cmd *cmd;
	int i, tag_size;

	nq->cmds = kcalloc(nq->queue_depth, sizeof(*cmd), GFP_KERNEL);
	if (!nq->cmds)
		return -ENOMEM;

	tag_size = ALIGN(nq->queue_depth, BITS_PER_LONG) / BITS_PER_LONG;
	nq->tag_map = kcalloc(tag_size, sizeof(unsigned long), GFP_KERNEL);
	if (!nq->tag_map) 
        {
		kfree(nq->cmds);
		return -ENOMEM;
	    }

	for (i = 0; i < nq->queue_depth; i++)
        {
		cmd = &nq->cmds[i];
		INIT_LIST_HEAD(&cmd->list);
		cmd->ll_list.next = NULL;
		cmd->tag = -1U;
	    }

	return 0;
}

static int maddevb_setup_queues(struct maddevb *maddevb)

{
	maddevb->queues = kcalloc(maddevb->mbdev->submit_queues,
				              sizeof(struct maddevb_queue), GFP_KERNEL);
	if (maddevb->queues == NULL)
        {
        PERR("maddevb_setup_queues maddevb=%px rc=-ENOMEM\n", maddevb);
		return -ENOMEM;
        }
   
	maddevb->queue_depth = maddevb->mbdev->hw_queue_depth;

	return 0;
}

static int maddevb_init_driver_queues(struct maddevb *maddevb)
{
	struct maddevb_queue *nq;
	int i, ret = 0;
    PMADDEVOBJ pmaddevobj = (PMADDEVOBJ)maddevb->mbdev->pmaddevobj;

	for (i = 0; i < maddevb->mbdev->submit_queues; i++)
        {
		nq = &maddevb->queues[i];

		maddevb_init_queue(maddevb, nq);

		ret = maddevb_setup_commands(nq);
		if (ret)
            {
            PWARN("maddevb_init_driver_queues... maddevb=%px rc=%d\n",
                  maddevb, ret);
			return ret;
            }

        maddevb->nr_queues++;
	    }

    PINFO("maddevb_init_driver_queues... dev#=%d nr_qs=%d\n",
           (int)pmaddevobj->devnum, (int)maddevb->nr_queues);

	return 0;
}

static int maddevb_init_tag_set(struct maddevb *maddevb, struct blk_mq_tag_set *set)
{
    PMADDEVOBJ pmaddevobj = (PMADDEVOBJ)maddevb->mbdev->pmaddevobj;
    int rc;

	set->ops = &maddevb_mq_ops;
	set->nr_hw_queues = maddevb ? maddevb->mbdev->submit_queues :
						         g_submit_queues;
	set->queue_depth = maddevb ? maddevb->mbdev->hw_queue_depth :
						         g_hw_queue_depth;
	set->numa_node = maddevb ? maddevb->mbdev->home_node : g_home_node;
	set->cmd_size	= sizeof(struct maddevb_cmd);
	set->flags = BLK_MQ_F_SHOULD_MERGE;
	if (g_no_sched)
		set->flags |= BLK_MQ_F_NO_SCHED;
	set->driver_data = NULL;

	if ((maddevb && maddevb->mbdev->blocking) || g_blocking)
		set->flags |= BLK_MQ_F_BLOCKING;

	rc = blk_mq_alloc_tag_set(set);

    PINFO("maddevb_init_tag_set... dev#=%d tagset=%px Qdepth=%d flags=x%X rc=%d\n",
          (int)pmaddevobj->devnum, set, set->queue_depth, set->flags, rc);

    return rc;
}

static void maddevb_validate_conf(struct maddevblk_device *mbdev)
{
    PMADDEVOBJ pmaddevobj = (PMADDEVOBJ)mbdev->pmaddevobj;

    PINFO("maddevb_validate_conf... dev#=%d mbdev=%px\n",
          (int)pmaddevobj->devnum, mbdev);

	mbdev->blocksize = round_down(mbdev->blocksize, 512);
	mbdev->blocksize = clamp_t(unsigned int, mbdev->blocksize, 512, 4096);

	if (mbdev->queue_mode == MADDEVB_Q_MQ && mbdev->use_per_node_hctx)
        {
		if (mbdev->submit_queues != nr_online_nodes)
			mbdev->submit_queues = nr_online_nodes;
	    }
     else
        {
         if (mbdev->submit_queues > nr_cpu_ids)
            mbdev->submit_queues = nr_cpu_ids;
         else
            if (mbdev->submit_queues == 0)
                mbdev->submit_queues = 1;
        }

	mbdev->queue_mode = min_t(unsigned int, mbdev->queue_mode, MADDEVB_Q_MQ);
	mbdev->irqmode = min_t(unsigned int, mbdev->irqmode, NULL_IRQ_TIMER);

	/* Do memory allocation, so set blocking */
	if (mbdev->memory_backed)
		mbdev->blocking = true;
	else /* cache is meaningless */
		mbdev->cache_size = 0;

    mbdev->cache_size = 
    min_t(unsigned long, ULONG_MAX / 1024 / 1024, mbdev->cache_size);

	mbdev->mbps = min_t(unsigned int, 1024 * 40, mbdev->mbps);
	/* can not stop a queue */
	if (mbdev->queue_mode == MADDEVB_Q_BIO)
		mbdev->mbps = 0;
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
	if (!__maddevb_setup_fault(&maddevb_timeout_attr, g_timeout_str))
		return false;

	if (!__maddevb_setup_fault(&maddevb_requeue_attr, g_requeue_str))
		return false;
#endif
	return true;
}

static enum hrtimer_restart maddevb_cmd_timer_expired(struct hrtimer *timer)
{
	maddevb_end_cmd(container_of(timer, struct maddevb_cmd, timer));

	return HRTIMER_NORESTART;
}

