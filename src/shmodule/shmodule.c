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
	case MEMINFO_IOCTL:
		return perform_meminfo(arg);
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

static long perform_lsmod(unsigned long arg)
{
	int counter = 1;
	struct lsmod_struct *buf = (struct lsmod_struct *) arg;
	struct lsmod_struct me;
        struct list_head *head = &(THIS_MODULE->list);
	struct module *curs = THIS_MODULE;
	struct lsmod_list *elt, *tmp;
	LIST_HEAD(klist);

	/* gather inserted modules informations */
	mutex_lock(&module_mutex);
	list_for_each_entry_continue(curs, head, list) {
		if (curs->name[0] != '\0') {
			elt = kmalloc(sizeof(struct lsmod_list), GFP_KERNEL);
			strncpy(elt->mod.name, curs->name, MODULE_NAME_LEN);
			elt->mod.ref = module_refcount(curs);
			elt->mod.size = curs->init_size + curs->core_size;
			list_add(&(elt->list), &klist);
			counter++;
		}
	}
	mutex_unlock(&module_mutex);

	/* copy to user-space and destroy */
	strncpy(me.name, THIS_MODULE->name, MODULE_NAME_LEN);
	me.size = THIS_MODULE->init_size + THIS_MODULE->core_size;
	me.ref = module_refcount(THIS_MODULE);
	copy_to_user(buf, &me, sizeof(me));
	buf++;
	list_for_each_entry_safe_reverse(elt, tmp, &klist, list) {
		copy_to_user(buf, &(elt->mod), sizeof(struct lsmod_struct));
		buf++;
		list_del(&(elt->list));
		kfree(elt);
	}
	copy_to_user(buf->name, "", sizeof(char));

	return 0;
}

static long perform_meminfo(unsigned long arg)
{
	struct sysinfo kmeminfo;

	si_meminfo(&kmeminfo);
	si_swapinfo(&kmeminfo);

	copy_to_user((struct sysinfo *) arg, &kmeminfo, sizeof(struct sysinfo));

	return 0;
}
