/*
 * Playground module for the LSM framework.
 *
 */

#define pr_fmt(fmt) "ACS-Lab: " fmt

#include <linux/lsm_hooks.h>
#include <linux/time64.h> // timespec64
#include <linux/time.h> // timezone
#include <linux/path.h> // path
#include <linux/dcache.h> // dentry_path
#include <linux/string.h> // strnstr
// #include <linux/usb.h> // USB
#include <linux/sysctl.h>
#include <linux/string_helpers.h>
#include <linux/kernel.h>
#include <linux/time.h>

// #include <linux/buffer_head.h>
// #include <linux/time.h>

// #include <linux/device/bus.h>
// #include <linux/pci.h>
// #include <linux/mutex.h>
// #include <linux/blkdev.h> // block

#include <linux/module.h>
#include <linux/ctype.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/blkdev.h>
#include <linux/backing-dev.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/kmod.h>
#include <linux/mutex.h>
#include <linux/idr.h>
#include <linux/log2.h>
#include <linux/pm_runtime.h>
#include <linux/badblocks.h>

static char *bdevt_str(dev_t devt, char *buf)
{
	if (MAJOR(devt) <= 0xff && MINOR(devt) <= 0xff) {
		char tbuf[BDEVT_SIZE];
		snprintf(tbuf, BDEVT_SIZE, "%02x%02x", MAJOR(devt),
			 MINOR(devt));
		snprintf(buf, BDEVT_SIZE, "%-9s", tbuf);
	} else
		snprintf(buf, BDEVT_SIZE, "%03x:%05x", MAJOR(devt),
			 MINOR(devt));

	return buf;
}

char *disk_name2(struct gendisk *hd, int partno, char *buf)
{
	if (!partno)
		snprintf(buf, BDEVNAME_SIZE, "%s", hd->disk_name);
	else if (isdigit(hd->disk_name[strlen(hd->disk_name) - 1]))
		snprintf(buf, BDEVNAME_SIZE, "%sp%d", hd->disk_name, partno);
	else
		snprintf(buf, BDEVNAME_SIZE, "%s%d", hd->disk_name, partno);

	return buf;
}

void __init printk_all_partitions2(void)
{
	struct class_dev_iter iter;
	struct device *dev;

	class_dev_iter_init(&iter, &block_class, NULL, &disk_type);
	while ((dev = class_dev_iter_next(&iter))) {
		struct gendisk *disk = dev_to_disk(dev);
		struct disk_part_iter piter;
		struct block_device *part;
		char name_buf[BDEVNAME_SIZE];
		char devt_buf[BDEVT_SIZE];

		/*
		 * Don't show empty devices or things that have been
		 * suppressed
		 */
		if (get_capacity(disk) == 0 ||
		    (disk->flags & GENHD_FL_SUPPRESS_PARTITION_INFO))
			continue;

		/*
		 * Note, unlike /proc/partitions, I am showing the
		 * numbers in hex - the same format as the root=
		 * option takes.
		 */
		disk_part_iter_init(&piter, disk, DISK_PITER_INCL_PART0);
		while ((part = disk_part_iter_next(&piter))) {
			bool is_part0 = part == disk->part0;

			printk("%s%s %10llu %s %s", is_part0 ? "" : "  ",
			       bdevt_str(part->bd_dev, devt_buf),
			       bdev_nr_sectors(part) >> 1,
			       disk_name2(disk, part->bd_partno, name_buf),
			       part->bd_meta_info ? part->bd_meta_info->uuid :
							  "");
			if (is_part0) {
				if (dev->parent && dev->parent->driver)
					printk(" driver: %s\n",
					       dev->parent->driver->name);
				else
					printk(" (driver?)\n");
			} else
				printk("\n");
		}
		disk_part_iter_exit(&piter);
	}
	class_dev_iter_exit(&iter);
}

/*** Hook handler definitions ***/

#define VENDOR_ID 0x0000
#define PRODUCT_ID 0x0000

#define DRIVE_UUID "b14e81dd-24fc-4868-a1a4-4931cc621609"

// static int match_usb_dev(struct usb_device *dev, void *unused)
// {
// 	return ((dev->descriptor.idVendor == VENDOR_ID) &&
// 		(dev->descriptor.idProduct == PRODUCT_ID));
// }

// https://www.kernel.org/doc/html/latest/driver-api/driver-model/bus.html

#define to_amba_device(d) container_of(d, struct amba_device, dev)

static int print_devices(struct device *dev, void *data)
{
	int r = 0;

	pr_info("found device");

	return r;
}

// struct find_data {
// 	struct amba_device *dev;
// 	struct device *parent;
// 	const char *busid;
// 	unsigned int id;
// 	unsigned int mask;
// };

static char *list_uuids(void)
{
	// struct subsys_dev_iter iter;
	// struct find_data data;

	printk_all_partitions2();

	//Basis:
	// https://01.org/linuxgraphics/gfx-docs/drm/driver-api/driver-model/bus.html#device-and-driver-lists
	//
	// https://static.lwn.net/kerneldoc/driver-api/infrastructure.html#c.bus_for_each_dev
	bus_for_each_dev(&pci_bus_type, NULL, NULL, print_devices);

	// Maybe use partition iter
	// https://www.kernel.org/doc/htmldocs/kernel-api/blkdev.html

	// subsys_dev_iter_init(&iter, &pci_bus_type, NULL, NULL);
	// while ((dev = subsys_dev_iter_next(&iter)))
	// 	pr_info("Found device", dev->);
	// subsys_dev_iter_exit(&iter);

	return 0;

	// https://www.kernel.org/doc/html/v4.15/driver-api/infrastructure.html#c.subsys_dev_iter_next
}

static int acslab_path_mkdir(const struct path *dir, struct dentry *dentry,
			     umode_t mode)
{
	char buf_dir[256];
	char buf_path[256];
	char *ret_dir;
	char *ret_path;
	char *list_ret;

	ret_dir = dentry_path(dir->dentry, buf_dir, ARRAY_SIZE(buf_dir));
	if (IS_ERR(ret_dir)) {
		pr_info("mkdir hooked: <failed to retrieve directory>\n");
		return 0;
	}

	ret_path = dentry_path(dentry, buf_path, ARRAY_SIZE(buf_path));
	if (IS_ERR(ret_path)) {
		pr_info("mkdir hooked: <failed to retrieve path>\n");
		return 0;
	}

	pr_info("mkdir hooked: %s in %s\n", ret_path, ret_dir);

	// Add your code here //
	list_ret = list_uuids();
	if (list_ret != 0) {
		pr_info("mkdir hooked: <failed to read devices>\n");
		return 0;
	}

	return 0;
}

static int acslab_settime(const struct timespec64 *ts,
			  const struct timezone *tz)
{
	struct tm tm1;
	int timezone_correction = 2;

	if (ts == NULL) {
		pr_info("settime hooked - given timestamp is null\n");
		return 0;
	}

	time64_to_tm(ts->tv_sec, (3600 * timezone_correction), &tm1);

	pr_info("settime hooked - timestamp: %04ld-%02d-%02d %02d:%02d:%02d\n",
		tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday, tm1.tm_hour,
		tm1.tm_min, tm1.tm_sec);

	return 0;
}

/*** Add hook handler ***/

static struct security_hook_list acslab_hooks[] __lsm_ro_after_init = {
	LSM_HOOK_INIT(path_mkdir, acslab_path_mkdir),
	LSM_HOOK_INIT(settime, acslab_settime),
};

static int __init acslab_init(void)
{
	pr_info("ACSLab:hooks have been added.\n");
	security_add_hooks(acslab_hooks, ARRAY_SIZE(acslab_hooks), "acslab");
	return 0;
}

DEFINE_LSM(acslab) = {
	.name = "acslab",
	.init = acslab_init,
};
