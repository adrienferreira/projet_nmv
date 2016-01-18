#ifndef KILL_H_
#define KILL_H_

long perform_kill(unsigned long arg);
void kill_work(struct work_struct *pws);
int _perform_kill(pid_t pid, unsigned int sig);

struct delayed_kill {
	pid_t pid;
	unsigned int sig;
	struct work_struct ws;
	struct pend_result *pr;
};

#endif
