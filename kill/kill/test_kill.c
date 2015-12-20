#include <sys/ioctl.h>
#include "kill_header.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

int main(void)
{
	int f;
	struct kill_struct ks;
	
	f=open("/dev/"CHRDEV_NAME,S_IWUSR);

	f.pid = 3;
	f.sig = SIGUSR1;

	if(f==-1){
		perror("Open");
		return 1;
	}

	ioctl(f, KILL_IOCTL, &ks);

	return 0;
}
