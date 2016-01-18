#ifndef WAITS_H_
#define WAITS_H_

#define CHECK_FREQUENCY (1 * HZ)

long perform_gen_wait(unsigned long arg, int is_waitall);
void gen_wait_work(struct work_struct *param_gws);

struct gen_wait_struct {
	/* pids to wait */
	int nb_pid;
	struct task_struct **tasks;

	/* generic wait/waitall */
	atomic_t nb_finished;
	int nb_to_wait;

	/* re-schecdule autonomously */
	unsigned long check_freq;
	struct delayed_work dws;

	struct pend_result *pr;
};

#endif
