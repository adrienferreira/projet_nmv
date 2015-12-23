#include <sys/ioctl.h>
#include "kill_header.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

int main(int argv, char*argc[])
{
	int f;
	struct kill_struct ks;
	
	if(argv!=3){
		printf("Usage : ./prog <pid> <sig>\n")
		return 1;
	}

	f=open("/dev/"CHRDEV_NAME,S_IWUSR);

	if(f == -1){
		perror("Open");
		return 1;
	}

	ks.pid = atoi(argc[1]);
	ks.sig = atoi(argc[2]);

	ioctl(f, KILL_IOCTL, &ks);

	return 0;
}
