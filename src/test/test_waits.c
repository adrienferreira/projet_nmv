#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

#include "structs.h"

#ifdef WAITALL_CMD
	#define IOCTL_CMD WAITALL_IOCTL
#else	
	#define IOCTL_CMD WAIT_IOCTL
#endif

void check_args(int argc); 
int open_chrdev(void);
void build_struct(int argc, char*argv[], struct gen_wait_usr_struct *gwus);

int main(int argc, char*argv[])
{
	int f;
	struct gen_wait_usr_struct gwus; 

	check_args(argc); 
	f=open_chrdev();
	build_struct(argc, argv, &gwus);

	ioctl(f, IOCTL_CMD, &gwus);

	close(f);
	free(gwus.pids);

	exit(EXIT_SUCCESS);
}

void check_args(int argc)
{
	if(argc == 1)
	{
		printf("Usage : ./prog <pid1>, ..., <pidN>\n");
		exit(EXIT_FAILURE);
	}
}

void build_struct(int argc, char*argv[], struct gen_wait_usr_struct *gwus)
{
	int i;

	gwus->nb_pid = (argc - 1);
	gwus->pids=(pid_t*)malloc(gwus->nb_pid * sizeof(pid_t));

	if(!(gwus->pids)){
		perror("PIDs allocation failed\n");
		exit(EXIT_FAILURE);
	}

	for(i=0; i < gwus->nb_pid; i++)
		gwus->pids[i] = (pid_t)atoi(argv[i+1]);
}

int open_chrdev(void)
{
	int fd;

	fd=open("/dev/"CHRDEV_NAME,S_IWUSR);

	if(fd == -1){
		perror("Open chrdev failed\n");
		exit(EXIT_FAILURE);
	}

	return fd;
}
