#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/string.h>

DECLARE_WAIT_QUEUE_HEAD(lsmod_waitqueue);

static long perform_lsmod(unsigned long arg);

struct lsmod_work {
	struct lsmod_struct *data;
	unsigned int size;
	struct list_head *head;
	struct work_struct work;
	bool cond;
	bool done;
	int async;
};

static void gather_modules(struct work_struct *work)
{
	struct lsmod_work *work_args = container_of(work,
						    struct lsmod_work, work);
	struct module *curs = container_of(work_args->head,
					   struct module,
					   list);
	struct lsmod_struct *elt;
	unsigned int counter = 0;

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
	}
}

static long perform_lsmod(unsigned long arg)
{
	struct lsmod_cmd *cmd = (struct lsmod_cmd *) arg;
	struct lsmod_cmd *kcmd = kmalloc(sizeof(struct lsmod_cmd), GFP_KERNEL);
	struct lsmod_work *work = kmalloc(sizeof(struct lsmod_work),
					  GFP_KERNEL);
	struct lsmod_struct *buf;
	long ret = 0;

	if (copy_from_user(kcmd, cmd, sizeof(struct lsmod_cmd)) != 0) {
		ret = -EFAULT;
		goto copy_fail;
	}

	work->size = kcmd->size;
	if (work->size == 0)
		goto no_kmalloc;

	buf = kmalloc(work->size, sizeof(struct lsmod_struct), GFP_KERNEL);
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

	schedule_work(&(work->work));

	if (kcmd->async)
		return ret;

	wait_event(lsmod_waitqueue, work->cond);

	/* copy to user-space */
	kcmd->done = work->done;
	kcmd->size = work->size;
	if (kcmd->done) {
		if (copy_to_user(kcmd->data,
				 work->data,
				 work->size * sizeof(struct lsmod_struct)) != 0)
			ret = -EFAULT;
	}
	if (copy_to_user(cmd, kcmd, sizeof(struct lsmod_cmd)) != 0)
		ret = -EFAULT;

	/* free memory */
	kfree(work->data);
copy_fail:
	kfree(kcmd);
	kfree(work);

	return ret;
}
