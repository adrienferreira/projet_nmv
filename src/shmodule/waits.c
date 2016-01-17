#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

#include "shmodule.h"
#include "structs.h"
#include "waits.h"

long perform_gen_wait(unsigned long arg, int is_waitall)
{
	int ret;
	struct gen_wait_usr_struct usr_struct;
	unsigned int icp;//ind cur pid
	pid_t *usr_pids_ptr;
	struct gen_wait_struct *gws;
	struct pend_result *pr;
	int nb_proc_finished;

	ret = 0;
	gws = NULL;
	pr = NULL;

	if(copy_from_user(&usr_struct, (void*)arg, sizeof(struct gen_wait_usr_struct))){
		pr_warn("Impossible to retrieve struct to kernel space\n");
		ret = -EFAULT;
		goto copy_waitall_struct_fail;
	}

	if(usr_struct.nb_pid == 0){
		pr_warn("No PID given.\n");
		ret = -EFAULT;
		goto pid_size_zero;
	}

	usr_pids_ptr = usr_struct.pids;//do not override user pointer
	usr_struct.pids = (pid_t*)kmalloc((usr_struct.nb_pid * sizeof(pid_t)), GFP_KERNEL);

	if(!(usr_struct.pids)){
		pr_warn("Impossible to allocate space for PIDs in kernel space\n");
		ret = -ENOMEM;
		goto alloc_pids_fail;
	}

	if(copy_from_user(usr_struct.pids, (void*)usr_pids_ptr, (usr_struct.nb_pid * sizeof(pid_t)))){
		pr_warn("Impossible to retrieve PIDs to kernel space\n");
		ret = -EFAULT;
		goto copy_pids_fail;
	}

	gws = (struct gen_wait_struct*)kmalloc(sizeof(struct gen_wait_struct), GFP_KERNEL);
	if(!gws){
		pr_warn("Impossible to allocate space for gen_wait_struct in kernel space\n");
		ret = -ENOMEM;
		goto alloc_gws_fail;
	}

	gws->tasks = (struct task_struct**)kmalloc((usr_struct.nb_pid * sizeof(struct task_struct*)), GFP_KERNEL);
	if(!(gws->tasks)){
		pr_warn("Impossible to allocate space for PIDs in kernel space\n");
		ret = -ENOMEM;
		goto alloc_gws_fail;
	}

	for(icp=0; icp<(usr_struct.nb_pid); icp++)
	{
		gws->tasks[icp] = get_pid_task(find_vpid(usr_struct.pids[icp]), PIDTYPE_PID);
		if(!(gws->tasks[icp]))
		{
			pr_warn("No process with the given PID (%d)\n",(int)usr_struct.pids[icp]);
			ret = -ESRCH;
			goto pid_not_found_fail;
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

	if(!usr_struct.async){
		while((nb_proc_finished =  atomic_read(&(gws->nb_finished))) < gws->nb_to_wait)
			schedule();

		ret = nb_proc_finished;
	}
	else{
		pr = add_pending_result();
		pr_warn("Ticket : %ld \n", pr->id_pend);

		if(pr == NULL){
			ret = -ENOMEM;
			goto add_pend_res_fail;
		}

		pr->ioctl_nr=is_waitall?WAITALL_IOCTL:WAIT_IOCTL;
		pr->size = sizeof(int);
		pr->data = (int*)kmalloc((size_t)pr->size, GFP_KERNEL);
		
		if(pr->data == NULL){
			ret = -ENOMEM;
			goto pr_data_alloc_fail;
		}

		gws->pr = pr;
		ret = pr->id_pend;
	}

	final_return:
	return ret;

	pr_data_alloc_fail:
	add_pend_res_fail:
	pid_not_found_fail:
	//TODO ???
	//for(; icp>=0 && (gws.tasks[icp]); icp--)
	//	put_task_struct(gws.tasks[icp]);
	kfree(gws->tasks);
	alloc_gws_fail:
	copy_pids_fail:
	kfree(usr_struct.pids);
	alloc_pids_fail:
	pid_size_zero:
	copy_waitall_struct_fail:
	goto final_return;
}


void gen_wait_work(struct work_struct*param_gws)
{//TODO put pid 
	struct gen_wait_struct *gws;
	int cur_fin, cur_nb_wait;
	int icp;

	gws = container_of(param_gws, struct gen_wait_struct, dws.work);
	cur_nb_wait=gws->nb_to_wait;
	cur_fin=0;

	for (icp = 0; icp <(gws->nb_pid); icp++)
		if(! (pid_alive(gws->tasks[icp])) )
			cur_fin++;

	pr_warn("Process finished %d/%d\n", cur_fin, cur_nb_wait);
	atomic_set(&(gws->nb_finished), cur_fin);

	if(cur_fin < cur_nb_wait)
		schedule_delayed_work(&(gws->dws), gws->check_freq);

	if(gws->pr){
		*((int*)gws->pr->data) = cur_fin;
		gws->pr->done = (cur_fin >= cur_nb_wait);
	}

	wake_up(&return_waitqueue);
}
