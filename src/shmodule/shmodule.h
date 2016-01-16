#ifndef SHMODULE_H_
#define SHMODULE_H_

struct pend_result {
	struct list_head list;
	unsigned long id_pend;
	void* data;
	unsigned int size;
	int ioctl_nr;
	bool done;
};

extern struct mutex mutex_pend_results;

extern wait_queue_head_t return_waitqueue;

long perform_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

struct pend_result* add_pending_result(void);

struct pend_result* get_result(long id);

#endif
