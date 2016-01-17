#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

#include "shmodule.h"
#include "structs.h"
#include "kill.h"

long perform_kill(unsigned long arg)
{
	int ret;
	struct kill_struct usr_struct;
	struct delayed_kill *dk;
	struct pend_result *pr;

	ret = 0;
	dk = NULL;
	pr = NULL;

	if (copy_from_user(&usr_struct, (void*)arg, sizeof(usr_struct)))
	{
		pr_warn("Cannot retrieve kill_struct from user space\n");
		ret = -EFAULT;
		goto copy_usr_struct_fail;
	}

	if(!usr_struct.async)
	{
		return _perform_kill(usr_struct.pid, usr_struct.sig);
	}
	else
	{
		dk = (struct delayed_kill*) kmalloc(sizeof(struct delayed_kill), GFP_KERNEL);
	
		if(dk == NULL)
		{
			ret = -ENOMEM;
			goto delayed_killl_alloc_fail;
		}

		pr = add_pending_result();

		if(pr == NULL){
			ret = -ENOMEM;
			goto add_pend_res_fail;
		}

		pr->ioctl_nr = KILL_IOCTL;
		pr->size = sizeof(long);
		pr->data = (long*)kmalloc((size_t)pr->size, GFP_KERNEL);
		
		if(pr->data == NULL){
			ret = -ENOMEM;
			goto pr_data_alloc_fail;
		}
		
		dk->pr = pr;
		dk->pid = usr_struct.pid;
		dk->sig = usr_struct.sig;

		if (copy_to_user(&(((struct kill_struct*)arg)->id_pend), &(pr->id_pend), sizeof(unsigned long)))
		{
			pr_warn("Cannot transfert id_pend to user space\n");
			ret = -EFAULT;
			goto copy_id_pend_fail;
		}

		INIT_WORK(&(dk->ws), kill_work);
		schedule_work(&(dk->ws));	
	}

	final_return:
		return ret;

	copy_id_pend_fail:
	pr_data_alloc_fail:
		kfree(dk);
	add_pend_res_fail:
	delayed_killl_alloc_fail:
	copy_usr_struct_fail:
	goto final_return;
}

void kill_work(struct work_struct* pws)
{
	struct delayed_kill *dk;

	dk = container_of(pws, struct delayed_kill, ws);
	*((long*)(dk->pr->data)) = _perform_kill(dk->pid, dk->sig);
	dk->pr->done = true;
	wake_up(&return_waitqueue);
	kfree(dk);
}

int _perform_kill(pid_t pid, unsigned int sig)
{
	int ret;
	struct pid*dest_pid;

	dest_pid = find_get_pid(pid);

	if (!dest_pid){
		pr_warn("No process with the given PID\n");
		return -ESRCH;
	}

	ret = kill_pid(dest_pid, sig, 1);
	put_pid(dest_pid);
	return ret;
}

