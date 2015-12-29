#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/swap.h>

#include "structs.h"
#include "shmodule.h"
#include "kill.c"
#include "waits.c"
#include "lsmod.c"
#include "meminfo.c"

MODULE_DESCRIPTION("sh helper module");
MODULE_AUTHOR("Redha Adrien");
MODULE_LICENSE("GPL");

static unsigned int major_nr;
static struct file_operations fops = {
	.unlocked_ioctl = perform_ioctl
};

static int shmodule_init(void)
{
	major_nr = register_chrdev(0, CHRDEV_NAME, &fops);

	if (major_nr == -EINVAL || major_nr == -EBUSY){
		pr_warn("Impossible to create the file "CHRDEV_NAME"\n");
		return 1;
	}

	//TODO mknod
	pr_warn("Major number : %u\n", major_nr);

	return 0;
}
module_init(shmodule_init);

static void shmodule_exit(void)
{
	pr_warn("Unregister device");	
	unregister_chrdev(major_nr, CHRDEV_NAME);
}
module_exit(shmodule_exit);

static long perform_ioctl(struct file *filp, unsigned int cmd,
			   unsigned long arg)
{
	switch (cmd) {
	case KILL_IOCTL:
		return perform_kill(arg);
	case LSMOD_IOCTL:
		return perform_lsmod(arg);
	case WAITALL_IOCTL:
		return perform_gen_wait(arg, 1);
	case WAIT_IOCTL:
		return perform_gen_wait(arg, 0);
	case MEMINFO_IOCTL:
		return perform_meminfo(arg);
	default:
		return -ENOTTY;
	}
}
