#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

#include "shmodule.h"
#include "structs.h"
#include "return.h"

long perform_return(unsigned long arg)
{
	long ret = 0;
	struct return_cmd *cmd = NULL, *kcmd = NULL;
	struct pend_result *res = NULL;

        /* Retrieve command from user */
	cmd = (struct return_cmd*) arg;
	kcmd = kmalloc(sizeof(struct return_cmd), GFP_KERNEL);
	if (kcmd == NULL)
		return -ENOMEM;
	if (copy_from_user(kcmd, cmd, sizeof(struct return_cmd)) != 0) {
		ret = -EFAULT;
		goto free1;
	}
	pr_warn("%s:%d: start return(%lu, %u)\n", __FILE__, __LINE__,
		kcmd->id_pend, kcmd->size);

	/* Retrieve pending result */
	mutex_lock(&mutex_pend_results);
	res = get_result(kcmd->id_pend);
	if (res == NULL) {
		ret = -EINVAL;
		goto rls_mutex;
	}

	pr_warn("%s:%d: wait\n", __FILE__, __LINE__);
	/* Wait for result if not available yet */
	if (!res->done)
		wait_event(return_waitqueue, res->done);
	
	/* Copy result back to user */
	if (copy_to_user(kcmd->data, res->data, min(kcmd->size, res->size)) != 0) {
		ret = -EINVAL;
		goto rls_mutex;
	}
	pr_warn("%s:%d: copy_back\n", __FILE__, __LINE__);
	kcmd->ioctl_nr = res->ioctl_nr;
	kcmd->size = res->size;
	if (copy_to_user(cmd, kcmd, sizeof(struct return_cmd)) != 0) {
		ret = -EINVAL;
		goto rls_mutex;
	}

	/* Free pending result from kernel */
	list_del(&(res->list));
	kfree(res->data);
	pr_warn("%s:%d: return result freed from kernel\n", 
		__FILE__, __LINE__);
rls_mutex:
	mutex_unlock(&mutex_pend_results);
	kfree(res);
	pr_warn("%s:%d: rls_mutex\n", __FILE__, __LINE__);

free1:
	kfree(kcmd);
	pr_warn("%s:%d: end return()\n", __FILE__, __LINE__);
	
	return ret;
}
