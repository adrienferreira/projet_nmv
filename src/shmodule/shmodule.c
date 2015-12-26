#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include "shmodule.h"
#include "structs.h"


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
	default:
		return -ENOTTY;
	}
}

static long perform_kill(unsigned long arg)
{
	struct kill_struct usr_struct;
	struct pid *dest_pid;

	if (copy_from_user(&usr_struct, (void*)arg, sizeof(usr_struct)))
		pr_warn("Cannot retrieve kill_struct from user space\n");

	dest_pid = find_get_pid(usr_struct.pid);

	if (!dest_pid)
		pr_warn("No process with the given PID\n");
	//TODO return ESRCH ??

	return kill_pid(dest_pid, usr_struct.sig, 1);
}
