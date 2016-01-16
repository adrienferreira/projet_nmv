#include <linux/module.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/pid.h>
#include <linux/sched.h>

#include "shmodule.h"
#include "structs.h"
#include "kill.h"


long perform_kill(unsigned long arg)
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

