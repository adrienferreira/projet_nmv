#ifndef STRUCT_H_
#define STRUCT_H_

#define CHRDEV_NAME "shmodule"
#define KILL_IOCTL _IOR('N', 1, struct kill_struct*)
#define MEMINFO_IOCTL _IOR('N', 4, struct sysinfo*)
#define LSMOD_IOCTL _IOR('N', 5, struct lsmod_struct*)

//TODO même header module et test sys/types.h
struct kill_struct {
	int sig;
	pid_t pid;
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
