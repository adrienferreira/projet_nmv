#include <linux/mm.h>
#include <linux/swap.h>
#include <asm/uaccess.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/slab.h>

#include "shmodule.h"
#include "structs.h"
#include "meminfo.h"

static void meminfo_worker(struct work_struct *work)
{
	struct meminfo_work *work_args = container_of(work, struct meminfo_work,
						      work);
	struct pend_result *res = work_args->pend_res;
	struct sysinfo *data = (struct sysinfo *) res->data;

	si_meminfo(data);
	si_swapinfo(data);

	res->size = sizeof(struct sysinfo);
	res->ioctl_nr = MEMINFO_IOCTL;
	res->done = true;
	wake_up(&return_waitqueue);
}

long perform_meminfo(unsigned long arg)
{
	struct sysinfo kmeminfo;
	struct meminfo_cmd *cmd = (struct meminfo_cmd *) arg;
	struct meminfo_cmd kcmd;
	struct meminfo_work *work = NULL;
	long ret = 0;
	
	if (copy_from_user(&kcmd, cmd, sizeof(struct meminfo_cmd)) != 0)
		return -EFAULT;

	if (kcmd.async) {
		work = kmalloc(sizeof(struct meminfo_work), GFP_KERNEL);
		if (work == NULL)
			return -ENOMEM;
		INIT_WORK(&(work->work), meminfo_worker);
		work->cond = false;
		work->pend_res = add_pending_result();
		work->pend_res->data = kmalloc(sizeof(struct sysinfo), GFP_KERNEL);
		if (work->pend_res->data == NULL) {
			ret = -ENOMEM;
			goto clean_meminfo;
		}
		work->pend_res->size = sizeof(struct sysinfo);
		kcmd.id_pend = work->pend_res->id_pend;
		if (copy_to_user(cmd, &kcmd, sizeof(struct meminfo_cmd)) != 0) {
			ret = -EFAULT;
		clean_meminfo:
			kfree(work);
			return ret;
		}
		schedule_work(&(work->work));
		return 0;
	}

	si_meminfo(&kmeminfo);
	si_swapinfo(&kmeminfo);

	pr_warn("meminfo: MemTotal: %lu\n", kmeminfo.totalram * kmeminfo.mem_unit / 1024);

	if (copy_to_user(kcmd.data, &kmeminfo, sizeof(struct sysinfo)) != 0)
		return -EFAULT;

	return 0;
}
