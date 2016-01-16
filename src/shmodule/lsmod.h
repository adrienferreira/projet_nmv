#ifndef LSMOD_H_
#define LSMOD_H_

struct lsmod_work {
	struct lsmod_struct *data;
	unsigned int size;
	struct list_head *head;
	struct work_struct work;
	bool cond;
	bool done;
	int async;
	struct pend_result *pend_res;
};

long perform_lsmod(unsigned long arg);

#endif
