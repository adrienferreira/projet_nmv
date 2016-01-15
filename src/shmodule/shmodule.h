#ifndef SHMODULE_H_
#define SHMODULE_H_

struct pend_result {
	struct list_head list;
	unsigned long id_pend;
	void* data;
	bool done;
};


static long perform_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

static struct pend_result* add_pending_result(void);

static struct pend_result* get_result(long id);

#endif
