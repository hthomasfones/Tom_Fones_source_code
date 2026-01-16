// SPDX-License-Identifier: GPL-2.0
#include <linux/vmalloc.h>
//#include "maddev_blk.h"
#include "maddevb.h"

#ifdef MADDEVB_ZONED
#define MB_TO_SECTS(mb) (((sector_t)mb * SZ_1M) >> SECTOR_SHIFT)

static inline unsigned int maddevb_zone_no(struct maddev_blk_dev *dev, sector_t sect)
{
	return sect >> ilog2(dev->zone_size_sects);
}

int maddevb_zone_init(struct maddev_blk_dev *dev)
{
	sector_t dev_capacity_sects;
	sector_t sector = 0;
	unsigned int i;

    PINFO("maddevb_zone_init()... dev=%p\n", dev);

	if (!is_power_of_2(dev->zone_size))
        {
		PERR("zone_size must be power-of-two\n");
		return -EINVAL;
	    }

	if (dev->zone_size > dev->size)
        {
		PERR("Zone size larger than device capacity\n");
		return -EINVAL;
	    }

	dev_capacity_sects = MB_TO_SECTS(dev->size);
	dev->zone_size_sects = MB_TO_SECTS(dev->zone_size);
	dev->nr_zones = dev_capacity_sects >> ilog2(dev->zone_size_sects);
	if (dev_capacity_sects & (dev->zone_size_sects - 1))
		dev->nr_zones++;

	dev->zones = kvmalloc_array(dev->nr_zones, sizeof(struct blk_zone),
			GFP_KERNEL | __GFP_ZERO);
	if (!dev->zones)
		return -ENOMEM;

	if (dev->zone_nr_conv >= dev->nr_zones)
        {
		dev->zone_nr_conv = dev->nr_zones - 1;
		PINFO("changed the number of conventional zones to %u",
			  dev->zone_nr_conv);
	    }

	for (i = 0; i <  dev->zone_nr_conv; i++)
        {
		struct blk_zone *zone = &dev->zones[i];

		zone->start = sector;
		zone->len = dev->zone_size_sects;
		zone->wp = zone->start + zone->len;
		zone->type = BLK_ZONE_TYPE_CONVENTIONAL;
		zone->cond = BLK_ZONE_COND_NOT_WP;

		sector += dev->zone_size_sects;
	    }

	for (i = dev->zone_nr_conv; i < dev->nr_zones; i++)
        {
		struct blk_zone *zone = &dev->zones[i];

		zone->start = zone->wp = sector;
		if (zone->start + dev->zone_size_sects > dev_capacity_sects)
			zone->len = dev_capacity_sects - zone->start;
		else
			zone->len = dev->zone_size_sects;
		zone->type = BLK_ZONE_TYPE_SEQWRITE_REQ;
		zone->cond = BLK_ZONE_COND_EMPTY;

		sector += dev->zone_size_sects;
	    }

	return 0;
}

void maddevb_zone_exit(struct maddev_blk_dev *dev)
{
    PINFO("maddevb_zone_exit()... dev=%p\n", dev);
	kvfree(dev->zones);
}

int maddevb_zone_report(struct gendisk *disk, sector_t sector,
                        struct blk_zone *zones, unsigned int *nr_zones)
{
	struct maddev_blk_dev *dev = disk->private_data;
	//struct maddev_blk_dev *dev = maddevb->dev;
	unsigned int zno, nrz = 0;

	zno = maddevb_zone_no(dev, sector);
	if (zno < dev->nr_zones)
    {
		nrz = min_t(unsigned int, *nr_zones, dev->nr_zones - zno);
		memcpy(zones, &dev->zones[zno], nrz * sizeof(struct blk_zone));
	}

	*nr_zones = nrz;

	return 0;
}

static blk_status_t maddevb_zone_write(struct maddevb_cmd *cmd, sector_t sector,
		                               unsigned int nr_sectors)
{
	struct maddev_blk_dev *dev = cmd->nq->dev;
	unsigned int zno = maddevb_zone_no(dev, sector);
	struct blk_zone *zone = &dev->zones[zno];

    PINFO("maddevb_zone_write()... dev=%p\n", dev);

	switch (zone->cond) {
	case BLK_ZONE_COND_FULL:
		/* Cannot write to a full zone */
		cmd->error = BLK_STS_IOERR;
		return BLK_STS_IOERR;
	case BLK_ZONE_COND_EMPTY:
	case BLK_ZONE_COND_IMP_OPEN:
		/* Writes must be at the write pointer position */
		if (sector != zone->wp)
			return BLK_STS_IOERR;

		if (zone->cond == BLK_ZONE_COND_EMPTY)
			zone->cond = BLK_ZONE_COND_IMP_OPEN;

		zone->wp += nr_sectors;
		if (zone->wp == zone->start + zone->len)
			zone->cond = BLK_ZONE_COND_FULL;
		break;
	case BLK_ZONE_COND_NOT_WP:
		break;
	default:
		/* Invalid zone condition */
		return BLK_STS_IOERR;
	}
	return BLK_STS_OK;
}

static blk_status_t maddevb_zone_reset(struct maddevb_cmd *cmd, sector_t sector)
{
	struct maddev_blk_dev *dev = cmd->nq->dev;
	unsigned int zno = maddevb_zone_no(dev, sector);
	struct blk_zone *zone = &dev->zones[zno];
	size_t i;
	
    PINFO("maddevb_zone_reset()... dev=%p\n", dev);
        
	switch (req_op(cmd->rq))
        {
	    case REQ_OP_ZONE_RESET_ALL:
		    for (i = 0; i < dev->nr_zones; i++)
                {
			    if (zone[i].type == BLK_ZONE_TYPE_CONVENTIONAL)
				    continue;

                zone[i].cond = BLK_ZONE_COND_EMPTY;
		        zone[i].wp = zone[i].start;
		        }
		    break;

	    case REQ_OP_ZONE_RESET:
		    if (zone->type == BLK_ZONE_TYPE_CONVENTIONAL)
			    return BLK_STS_IOERR;

		    zone->cond = BLK_ZONE_COND_EMPTY;
		    zone->wp = zone->start;
		    break;

        default:
		    return BLK_STS_NOTSUPP;
	    }

	return BLK_STS_OK;
}

blk_status_t maddevb_handle_zoned(struct maddevb_cmd *cmd, enum req_opf op,
			                      sector_t sector, sector_t nr_sectors)
{
	switch (op) 
        {
	    case REQ_OP_WRITE:
		    return maddevb_zone_write(cmd, sector, nr_sectors);

        case REQ_OP_ZONE_RESET:
	    case REQ_OP_ZONE_RESET_ALL:
		    return maddevb_zone_reset(cmd, sector);

        default:
		    return BLK_STS_OK;
	}
}
#endif //MADDEVB_ZONED
