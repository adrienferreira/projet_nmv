#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sysinfo.h>

#include "structs.h"

int main(int argc, char **argv)
{
	int f;
	struct sysinfo meminfo;
	unsigned int unit;

	if (argc != 1) {
		printf("Usage : %s\n", argv[0]);
		return 1;
	}

	f = open("/dev/"CHRDEV_NAME, S_IWUSR);

	if (f == -1) {
		perror("Open");
		return 1;
	}

	ioctl(f, MEMINFO_IOCTL, &meminfo);

	unit = meminfo.mem_unit / 1024;
	printf("MemTotal:     %8lu kB\n"
	       "MemFree:      %8lu kB\n"
	       "MemAvailable: %8lu kB\n"      /* complicated, see fs/proc/meminfo.c */
	       "Buffers:      %8lu kB\n"
	       "SwapTotal:    %8lu kB\n"
	       "SwapFree:     %8lu kB\n"
	       "Shmem:        %8lu kB\n",
	       meminfo.totalram * unit,
	       meminfo.freeram  * unit,
	       (long unsigned int) 0,
	       meminfo.bufferram * unit,
	       meminfo.totalswap * unit,
	       meminfo.freeswap * unit,
	       meminfo.sharedram * unit);

	close(f);

	return 0;
}
