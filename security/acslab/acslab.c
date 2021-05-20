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
#include <linux/sysctl.h> // zero, one

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
	result->counter += match_usb_dev(dev, NULL);
	return 0;
}

static int usb_connected(void)
{
	int ret_for_each;
	struct device_found found = {0};

	ret_for_each = usb_for_each_dev(&found, print_usbs);
	if (ret_for_each != 0)
	{
		return -EPERM;
	}

	if (found.counter != 0)
	{
		return 1;
	}

	return 0;
}

struct acs
{
	int enforced;
	int denied_counter;
};

struct acs acslab = {1, 0};

static int acslab_path_mkdir(const struct path *dir, struct dentry *dentry,
							 umode_t mode)
{
	int connected;

	if (acslab.enforced == 0)
	{
		return 0;
	}

	if (strcmp(dentry->d_name.name, "dsn") != 0)
	{
		return 0;
	}

	connected = usb_connected();
	if (connected != 0 && connected != 1)
	{
		pr_err("acslab: checking ubs devices failed");
		return 0;
	}

	if (connected != 0)
	{
		return 0;
	}
	acslab.denied_counter += 1;
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

#ifdef CONFIG_SYSCTL
static int acslab_dointvec_minmax(struct ctl_table *table, int write,
								  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	// struct ctl_table table_copy;

	if (write)
		return -EROFS;

	// /* Lock the max value if it ever gets set. */
	// table_copy = *table;
	// if (*(int *)table_copy.data == *(int *)table_copy.extra2)
	// 	table_copy.extra1 = table_copy.extra2;

	return proc_dointvec_minmax(table, write, buffer, lenp, ppos);
}

struct ctl_path acslab_sysctl_path[] = {
	{
		.procname = "kernel",
	},
	{
		.procname = "acslab",
	},
	{}};

static int zero = 0;
static int one = 1;

static struct ctl_table acslab_sysctl_table[] = {
	{.procname = "enforced",
	 .data = &acslab.enforced,
	 .maxlen = sizeof(int),
	 .mode = 0644,
	 .proc_handler = proc_dointvec_minmax,
	 .extra1 = &zero,
	 .extra2 = &one},
	{.procname = "denied_counter",
	 .maxlen = sizeof(int),
	 .data = &acslab.denied_counter,
	 .mode = 0444,
	 .proc_handler = proc_dointvec_minmax},
	{}};

static void __init acslab_init_sysctl(void)
{
	if (!register_sysctl_paths(acslab_sysctl_path, acslab_sysctl_table))
		panic("acsctl: sysctl registration failed.\n");
}
#else
static inline void acslab_init_sysctl(void)
{
}
#endif /* CONFIG_SYSCTL */

void __init acslab_add_hooks(void)
{
	pr_info("Hooks have been added!");
	security_add_hooks(acslab_hooks, ARRAY_SIZE(acslab_hooks));
	acslab_init_sysctl();
}
