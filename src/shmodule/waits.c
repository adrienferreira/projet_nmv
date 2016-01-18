#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/uaccess.h>

#include "shmodule.h"
#include "structs.h"
#include "waits.h"

long perform_gen_wait(unsigned long arg, int is_waitall)
{
	int ret;
	struct gen_wait_usr_struct usr_struct;
	unsigned int icp, j;/* ind cur pid */
	pid_t *usr_pids_ptr;
	struct gen_wait_struct *gws;
	struct pend_result *pr;
	int nb_proc_finished;

	ret = 0;
	gws = NULL;
	pr = NULL;

	if (copy_from_user(&usr_struct,
			   (void *)arg,
			   sizeof(struct gen_wait_usr_struct))) {
		pr_warn("Impossible to retrieve struct to kernel space\n");
		ret = -EFAULT;
		goto final_return;
	}

	if (usr_struct.nb_pid == 0) {
		pr_warn("No PID given.\n");
		ret = -EFAULT;
		goto final_return;
	}

	usr_pids_ptr = usr_struct.pids;/* do not override user pointer */
	usr_struct.pids = kmalloc_array(usr_struct.nb_pid,
					sizeof(pid_t),
					GFP_KERNEL);

	if (!(usr_struct.pids)) {
		pr_warn("Impossible to allocate space for PIDs in kernel space\n");
		ret = -ENOMEM;
		goto final_return;
	}

	if (copy_from_user(usr_struct.pids,
			   (void *)usr_pids_ptr,
			   (usr_struct.nb_pid * sizeof(pid_t)))) {
		pr_warn("Impossible to retrieve PIDs to kernel space\n");
		ret = -EFAULT;
		goto free_pids;
	}

	gws = kmalloc(sizeof(struct gen_wait_struct), GFP_KERNEL);
	if (!gws) {
		ret = -ENOMEM;
		goto free_pids;
	}

	gws->tasks = kmalloc_array(usr_struct.nb_pid,
				   sizeof(struct task_struct *),
				   GFP_KERNEL);
	if (!(gws->tasks)) {
		pr_warn("Impossible to allocate space for PIDs in kernel space\n");
		ret = -ENOMEM;
		goto free_pids;
	}

	for (icp = 0; icp < (usr_struct.nb_pid); icp++) {
		gws->tasks[icp] = get_pid_task(find_vpid(usr_struct.pids[icp]),
					       PIDTYPE_PID);
		if (!(gws->tasks[icp])) {
			pr_warn("No process with the given PID (%d)\n",
				(int)usr_struct.pids[icp]);
			ret = -ESRCH;
			goto free_all;
		}
	}

	atomic_set(&(gws->nb_finished), 0);
	gws->nb_to_wait = 0;
	gws->nb_pid = usr_struct.nb_pid;
	gws->check_freq = CHECK_FREQUENCY;
	gws->nb_to_wait = is_waitall ? gws->nb_pid : 1;
	gws->pr = NULL;

	INIT_DELAYED_WORK(&(gws->dws), gen_wait_work);
	schedule_delayed_work(&(gws->dws), gws->check_freq);

	if (!usr_struct.async) {
		nb_proc_finished = atomic_read(&(gws->nb_finished));
		while (nb_proc_finished < gws->nb_to_wait) {
			schedule();
			nb_proc_finished = atomic_read(&(gws->nb_finished));
		}

		ret = nb_proc_finished;
		goto free_all;
	} else {
		pr = add_pending_result();

		if (pr == NULL) {
			ret = -ENOMEM;
			goto free_all;
		}

		pr->ioctl_nr = is_waitall?WAITALL_IOCTL:WAIT_IOCTL;
		pr->size = sizeof(int);
		pr->data = kmalloc((size_t)pr->size, GFP_KERNEL);

		if (pr->data == NULL) {
			ret = -ENOMEM;
			goto free_all;
		}

		if (copy_to_user(&((struct gen_wait_usr_struct *)arg)->id_pend,
				 &(pr->id_pend),
				 sizeof(unsigned long))) {
			pr_warn("Cannot transfert id_pend to user space\n");
			ret = -EFAULT;
			goto free_all;
		}

		gws->pr = pr;
		kfree(usr_struct.pids);
	}

final_return:
	return ret;

free_all:
	for (j = 0; j < icp; j++)
		if (gws->tasks[j])
			put_task_struct(gws->tasks[j]);
	kfree(gws->tasks);

free_pids:
	kfree(usr_struct.pids);
}


void gen_wait_work(struct work_struct *param_gws)
{
	struct gen_wait_struct *gws;
	int cur_fin, cur_nb_wait;
	int icp;

	gws = container_of(param_gws, struct gen_wait_struct, dws.work);
	cur_nb_wait = gws->nb_to_wait;
	cur_fin = 0;

	for (icp = 0; icp < (gws->nb_pid); icp++)
		if (!(pid_alive(gws->tasks[icp])))
			cur_fin++;

	pr_warn("Process finished %d/%d\n", cur_fin, cur_nb_wait);
	atomic_set(&(gws->nb_finished), cur_fin);

	if (cur_fin < cur_nb_wait) {
		schedule_delayed_work(&(gws->dws), gws->check_freq);
	} else	{
		if (gws->pr) {
			for (icp = 0; icp < (gws->nb_pid); icp++)
				put_task_struct(gws->tasks[icp]);

			kfree(gws->tasks);
			*((int *)gws->pr->data) = cur_fin;
			gws->pr->done = 1;
		}
	}

	wake_up(&return_waitqueue);
}
