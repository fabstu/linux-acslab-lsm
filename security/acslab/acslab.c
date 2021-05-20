/*
 * Playground module for the LSM framework.
 *
 */

#define pr_fmt(fmt) "ACS-Lab: " fmt

#include <linux/lsm_hooks.h>
#include <linux/time64.h> // timespec64
#include <linux/time.h>	  // timezone
#include <linux/path.h>	  // path
#include <linux/dcache.h> // dentry_path
#include <linux/string.h> // strnstr
#include <linux/usb.h>	  // USB
#include <linux/blkdev.h> // block

/*** Hook handler definitions ***/

#define VENDOR_ID 0x930
#define PRODUCT_ID 0x6545

static int match_usb_dev(struct usb_device *dev, void *unused)
{
	return ((dev->descriptor.idVendor == VENDOR_ID) &&
			(dev->descriptor.idProduct == PRODUCT_ID));
}

struct device_found
{
	int counter;
};

static int print_usbs(struct usb_device *dev, void *data)
{
	struct device_found *result = data;
	int found;

	found = match_usb_dev(dev, NULL);
	if (found != 0)
	{
		pr_info("usb device found! - product: %s, manu: %s, vendorID: %d, productID: %d found=%d\n",
				dev->product, dev->manufacturer, dev->descriptor.idVendor, dev->descriptor.idProduct, found);
		result->counter += 1;
	}
	else
	{
		pr_info("usb device - product: %s, manu: %s, vendorID: %d, productID: %d\n",
				dev->product, dev->manufacturer, dev->descriptor.idVendor, dev->descriptor.idProduct);
	}

	return 0;
}

static int acslab_path_mkdir(const struct path *dir, struct dentry *dentry,
							 umode_t mode)
{
	char buf_dir[256];
	char buf_path[256];
	char *ret_dir;
	char *ret_path;
	int ret_for_each;
	struct device_found found = {0};

	ret_dir = dentry_path(dir->dentry, buf_dir, ARRAY_SIZE(buf_dir));
	if (IS_ERR(ret_dir))
	{
		pr_info("mkdir hooked: <failed to retrieve directory>\n");
		return 0;
	}

	ret_path = dentry_path(dentry, buf_path, ARRAY_SIZE(buf_path));
	if (IS_ERR(ret_path))
	{
		pr_info("mkdir hooked: <failed to retrieve path>\n");
		return 0;
	}

	if (strcmp(dentry->d_name.name, "dsn") != 0)
	{
		pr_info("mkdir hooked: strlen1=%zu strlen2=%zu dir=%s, path=%s, d_name=%s\n", strlen(ret_dir), strlen(ret_path), ret_dir, ret_path, dentry->d_name.name);
		return 0;
	}

	pr_info("mkdir hooked: is dsn, found.counter=%d\n", found.counter);

	ret_for_each = usb_for_each_dev(&found, print_usbs);
	if (ret_for_each != 0)
	{
		pr_err("mkdir hooked: failed to list usb devices\n");
		return 0;
	}

	if (found.counter != 0)
	{
		pr_info("mkdir hooked: device found: %d\n", found.counter);
		return 0;
	}

	pr_info("mkdir hooked: device not found: %d\n", found.counter);
	return -EPERM;
}

static int acslab_settime(const struct timespec64 *ts, const struct timezone *tz)
{
	pr_info("settime hooked\n");
	// printk_all_partitions();
	return 0;
}

/*** Add hook handler ***/

static struct security_hook_list acslab_hooks[] = {
	LSM_HOOK_INIT(path_mkdir, acslab_path_mkdir),
	LSM_HOOK_INIT(settime, acslab_settime),
};

void __init acslab_add_hooks(void)
{
	pr_info("Hooks have been added!");
	security_add_hooks(acslab_hooks, ARRAY_SIZE(acslab_hooks));
}
