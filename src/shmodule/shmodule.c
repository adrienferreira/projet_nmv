#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <asm-generic/errno-base.h>
#include <linux/wait.h>

#include "structs.h"
#include "shmodule.h"
#include "kill.h"
#include "waits.h"
#include "lsmod.h"
#include "meminfo.h"
#include "return.h"

MODULE_DESCRIPTION("sh helper module");
MODULE_AUTHOR("Redha Adrien");
MODULE_LICENSE("GPL");

static unsigned int major_nr;
static const struct file_operations fops = {
	.unlocked_ioctl = perform_ioctl
};

DEFINE_MUTEX(mutex_pend_results);
static struct list_head lst_pend_results;
static long cur_id_pend;
DECLARE_WAIT_QUEUE_HEAD(return_waitqueue);


static int shmodule_init(void)
{
	INIT_LIST_HEAD(&lst_pend_results);
	cur_id_pend = 0;

	major_nr = register_chrdev(0, CHRDEV_NAME, &fops);

	if (major_nr == -EINVAL || major_nr == -EBUSY) {
		pr_warn("Impossible to create the file "CHRDEV_NAME"\n");
		return 1;
	}

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

long perform_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
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
	case RETURN_IOCTL:
		return perform_return(arg);
	default:
		return -ENOTTY;
	}
}

struct pend_result *add_pending_result(void)
{
	struct pend_result *pr;

	pr = kmalloc(sizeof(struct pend_result), GFP_KERNEL);

	if (pr == NULL)
		return NULL;

	INIT_LIST_HEAD(&(pr->list));
	pr->data = NULL;
	pr->done = false;

	mutex_lock(&mutex_pend_results);
	pr->id_pend = cur_id_pend++;
	list_add_tail(&(pr->list), &lst_pend_results);
	mutex_unlock(&mutex_pend_results);

	return pr;
}

struct pend_result *get_result(long id)
{
	struct pend_result *pr;

	list_for_each_entry(pr, &lst_pend_results, list)
		if (pr->id_pend == id)
			return pr;

	return NULL;
}
