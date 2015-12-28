#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "structs.h"

int main(int argc, char **argv)
{
	int f;
	struct lsmod_struct *res, *cur;

	if (argc != 1) {
		printf("Usage : %s\n", argv[0]);
		return 1;
	}

	f = open("/dev/"CHRDEV_NAME, S_IWUSR);

	if (f == -1) {
		perror("Open");
		return 1;
	}
	
	/* TODO: PAGE_SIZE may not be enough and uses a syscall */
	res = malloc(getpagesize());
	cur = res;

	ioctl(f, LSMOD_IOCTL, res);

	printf("Name\t\t\tSize  Used by\n");
	int i = 0;
	while (cur->size != 0) {
		printf("%-24s%-5u %u\n", cur->name, cur->size, cur->ref);
		cur++;
		i++;
	}
	
	free(res);
	close(f);

	return 0;
}
