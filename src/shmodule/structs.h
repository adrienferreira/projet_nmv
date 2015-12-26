#ifndef STRUCT_H_
#define STRUCT_H_

#define CHRDEV_NAME "shmodule"
#define KILL_IOCTL _IOR('N', 1, struct kill_struct*)


//TODO même header module et test sys/types.h
struct kill_struct{
	int sig;
	pid_t pid;
};

#endif
