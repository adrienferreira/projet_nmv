#include <linux/mm.h>
#include <linux/swap.h>
#include <asm/uaccess.h>

#include "shmodule.h"
#include "structs.h"
#include "meminfo.h"

long perform_meminfo(unsigned long arg)
{
	struct sysinfo kmeminfo;

	si_meminfo(&kmeminfo);
	si_swapinfo(&kmeminfo);

	if (copy_to_user((struct sysinfo *) arg, &kmeminfo, sizeof(struct sysinfo)) != 0)
		return -EFAULT;

	return 0;
}
