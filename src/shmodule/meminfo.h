#ifndef MEMINFO_H_
#define MEMINFO_H_

struct meminfo_work {
	struct work_struct work;
	struct pend_result *pend_res;
	bool cond;
};

long perform_meminfo(unsigned long arg);

#endif
