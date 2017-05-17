/*
 * Playground module for the LSM framework.
 *
 */

#define pr_fmt(fmt) "ACS-Lab: " fmt

#include <linux/lsm_hooks.h>
#include <linux/time64.h>	// timespec64
#include <linux/time.h>		// timezone
#include <linux/path.h>		// path
#include <linux/dcache.h>	// dentry_path
#include <linux/string.h>	// strnstr
#include <linux/usb.h>		// USB

/*** Hook handler definitions ***/

/*
#define VENDOR_ID 0x0000
#define PRODUCT_ID 0x0000

static int match_usb_dev(struct usb_device *dev, void *unused)
{
	return ((dev->descriptor.idVendor == VENDOR_ID) &&
		(dev->descriptor.idProduct == PRODUCT_ID));
}

static int acslab_path_mkdir(const struct path *dir, struct dentry *dentry, umode_t mode)
{
	char buf_dir[256];
	char buf_path[256];
	char *ret_dir;
	char *ret_path;

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

	return 0;
}
*/

static int acslab_settime (const struct timespec64 *ts, const struct timezone *tz)
{
	pr_info("settime hooked\n");
	return 0;
}

/*** Add hook handler ***/

static struct security_hook_list acslab_hooks[] = {
	//LSM_HOOK_INIT(path_mkdir, acslab_path_mkdir),
	LSM_HOOK_INIT(settime, acslab_settime),
};

void __init acslab_add_hooks(void)
{
	pr_info("Hooks have been added!");
	security_add_hooks(acslab_hooks, ARRAY_SIZE(acslab_hooks));
}
