#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "shmodule.h"
#include "structs.h"
#include "lsmod.h"


DECLARE_WAIT_QUEUE_HEAD(lsmod_waitqueue);

static void gather_modules(struct work_struct *work)
{
	struct lsmod_work *work_args = container_of(work,
						    struct lsmod_work, work);
	struct module *curs = container_of(work_args->head,
					   struct module,
					   list);
	struct lsmod_struct *elt;
	unsigned int counter = 0;

	pr_warn("lsmod_worker (l. %d) : start\n", __LINE__);
	elt = work_args->data;
	work_args->done = true;

	strncpy(elt->name, curs->name, MODULE_NAME_LEN);
	elt->ref = module_refcount(curs);
	elt->size = curs->init_size + curs->core_size;
	elt++;
	counter++;

	/* gather inserted modules informations */
	mutex_lock(&module_mutex);
	list_for_each_entry_continue(curs, work_args->head, list) {
		if (counter > work_args->size)
			work_args->done = false;
		if (curs->name[0] != '\0') {
			if (work_args->done == true) {
				strncpy(elt->name, curs->name, MODULE_NAME_LEN);
				elt->ref = module_refcount(curs);
				elt->size = curs->init_size + curs->core_size;
				elt++;
			}
			counter++;
		}
	}
	mutex_unlock(&module_mutex);
	work_args->size = counter;

	if (!work_args->async) {
		work_args->cond = true;
		wake_up(&lsmod_waitqueue);
		pr_warn("lsmod_worker (l. %d) : wake_up()\n", __LINE__);
	} else {
		work_args->pend_res->data = work_args->data;
		work_args->pend_res->size = counter * sizeof(struct lsmod_struct);
		work_args->pend_res->ioctl_nr = LSMOD_IOCTL;
		work_args->pend_res->done = true;
		wake_up(&return_waitqueue);
		pr_warn("lsmod_worker (l. %d) : wake_up()\n", __LINE__);
	}
	pr_warn("lsmod_worker (l. %d) : end\n", __LINE__);
}

long perform_lsmod(unsigned long arg)
{
	struct lsmod_cmd *cmd = (struct lsmod_cmd *) arg;
	struct lsmod_cmd *kcmd = kmalloc(sizeof(struct lsmod_cmd), GFP_KERNEL);
	struct lsmod_work *work = kmalloc(sizeof(struct lsmod_work),
					  GFP_KERNEL);
	struct lsmod_struct *buf;
	struct pend_result *pend_res;
	long ret = 0;

	if (copy_from_user(kcmd, cmd, sizeof(struct lsmod_cmd)) != 0) {
		ret = -EFAULT;
		goto copy_fail;
	}

	work->size = kcmd->size;
	pr_warn("lsmod_ioctl (l. %d) : kcmd->size = %u\n",
		__LINE__, work->size);
	if (work->size == 0)
		goto no_kmalloc;

	buf = kmalloc_array(work->size, sizeof(struct lsmod_struct),
			    GFP_KERNEL);
	if (IS_ERR(buf)) {
		ret = PTR_ERR(buf);
		goto copy_fail;
	}
	if (buf == NULL) {
		ret = -EFAULT;
		goto copy_fail;
	}
	work->data = buf;
no_kmalloc:
	work->head = &(THIS_MODULE->list);
	INIT_WORK(&(work->work), gather_modules);
	work->async = kcmd->async;
	work->cond = false;

	if (kcmd->async) {
		pend_res = add_pending_result();
		kcmd->id_pend = pend_res->id_pend;
		kcmd->done = 1;
		work->pend_res = pend_res;
		if (copy_to_user(cmd,
				 kcmd,
				 sizeof(struct lsmod_cmd)) != 0) {
			ret = -EFAULT;
		}
		pr_warn("lsmod_ioctl (l. %d) : schedule_work()\n", __LINE__);
		schedule_work(&(work->work));

		return ret;
	}

	pr_warn("lsmod_ioctl (l. %d) : schedule_work()\n", __LINE__);
	schedule_work(&(work->work));
	pr_warn("lsmod_ioctl (l. %d) : wait_event()\n", __LINE__);
	wait_event(lsmod_waitqueue, work->cond);
	pr_warn("lsmod_ioctl (l. %d) : woke up\n", __LINE__);

	pr_warn("lsmod_ioctl (l. %d) : copy to user space\n", __LINE__);
	/* copy to user-space */
	kcmd->done = work->done;
	kcmd->size = work->size;
	if (kcmd->done) {
		pr_warn("lsmod_ioctl (l. %d) : done! (copy %u elements)\n",
			__LINE__, kcmd->size);
		if (copy_to_user(kcmd->data, work->data, work->size
				 * sizeof(struct lsmod_struct)) != 0) {
			ret = -EFAULT;
			goto return_fail;
		}
	}
	if (copy_to_user(cmd, kcmd, sizeof(struct lsmod_cmd)) != 0)
		ret = -EFAULT;

return_fail:
	/* free memory */
	kfree(work->data);
copy_fail:
	kfree(kcmd);
	kfree(work);

	return ret;
}
