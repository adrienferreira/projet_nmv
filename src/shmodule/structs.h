#ifndef STRUCT_H_
#define STRUCT_H_

#define CHRDEV_NAME "shmodule"

#define KILL_IOCTL _IOR('N', 1, struct kill_struct*)
#define WAITALL_IOCTL _IOR('N', 2, struct  gen_wait_usr_struct*)
#define WAIT_IOCTL _IOR('N', 3, struct  gen_wait_usr_struct*)
#define MEMINFO_IOCTL _IOR('N', 4, struct sysinfo*)
#define LSMOD_IOCTL _IOR('N', 5, struct lsmod_struct*)

struct kill_struct {
	unsigned int sig;
	pid_t pid;
};

struct gen_wait_usr_struct{
	unsigned int nb_pid;
	pid_t *pids;
};

#ifndef MODULE_NAME_LEN
/* MODULE_NAME_LEN, as defined in the kernel in linux/module.h */
#define MODULE_NAME_LEN (64 - sizeof(unsigned long))
#endif

struct lsmod_struct {
	char name[MODULE_NAME_LEN];
	unsigned int size;
	unsigned int ref;
};

#endif
