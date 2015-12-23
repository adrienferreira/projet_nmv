//TODO même header module et test sys/types.h
struct kill_struct{
	int sig;
	pid_t pid;
};

#define CHRDEV_NAME "kill"
#define KILL_IOCTL _IOR('N', 1, struct kill_struct*)


