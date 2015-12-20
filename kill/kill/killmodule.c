#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include "kill_header.h"


MODULE_DESCRIPTION("Kill command");
MODULE_AUTHOR("Redha Adrien");
MODULE_LICENSE("GPL");

static long perform_ioctl (struct file *filp, unsigned int cmd, unsigned long arg);

static unsigned int kill_major;

static struct file_operations kill_fops = {
    .unlocked_ioctl= perform_ioctl
};

static int kill_ioctl_init(void)
{
	kill_major = register_chrdev(0, CHRDEV_NAME, &kill_fops);

	if(kill_major==-EINVAL || kill_major==-EBUSY){
		pr_warn("Impossible to create the file "CHRDEV_NAME" \n");
		return 1;
	}

	pr_warn("Major number : %u\n", kill_major);
	return 0;
}

static long perform_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct kill_struct usr_struct;
	struct pid *dest_pid;

	if(copy_from_user(arg, usr_struct, sizeof(usr_struct)))
		pr_warn("Impossible to retrieve struct to kernel space\n");

	dest_pid = find_get_pid(usr_struct->pid);

	if(!dest_pid)
		pr_warn("No process with the given PID\n");
		//TODO return ESRCH ??

	switch(cmd){
		case KILL_IOCTL:
			return kill_pid(dest_pid, usr_struct->sig, 1);
		default:
			return -ENOTTY;
		break;
	}
}


static void kill_ioctl_exit(void)
{
	pr_warn("Unregister device");	
	unregister_chrdev(kill_major, CHRDEV_NAME);
}


module_init(kill_ioctl_init);
module_exit(kill_ioctl_exit);
