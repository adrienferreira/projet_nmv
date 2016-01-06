#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "shellghoumi.h"
#include "structs.h"

int main(int argc, char **argv)
{
	int f, ret;
	enum commands cmd;

	if (argc < 2) {
		PRINT_USAGE;
		return EXIT_SUCCESS;
	}

	cmd = get_cmd(argv[1]);

	if (cmd == HELP) {
		PRINT_USAGE;
		printf("\n");
		PRINT_HELP;
		return EXIT_SUCCESS;
	}
	if (cmd == UNKNOWN) {
		printf("Unknown command.\nTry \"%s help\".\n", argv[0]);
		return EXIT_FAILURE;
	}

	f = open("/dev/"CHRDEV_NAME, S_IWUSR);

	switch (cmd) {
	case KILL:
		ret = perform_kill(argc - 1, argv + 1, f);
		break;
	case WAIT:
		ret = perform_wait(argc - 1, argv + 1, f);
		break;
	case WAITALL:
		ret = perform_waitall(argc - 1, argv + 1, f);
		break;
	case PRINT:
		ret = perform_print(argc - 1, argv + 1, f);
		break;
	case LSMOD:
		ret = perform_lsmod(argc - 1, argv + 1, f);
		break;
	}		

	close(f);

	return ret;
}

enum commands get_cmd(char *string)
{
	if (strcmp(string, "kill") == 0)
		return KILL;
	if (strcmp(string, "wait") == 0)
		return WAIT;
	if (strcmp(string, "waitall") == 0)
		return WAITALL;
	if (strcmp(string, "print") == 0)
		return PRINT;
	if (strcmp(string, "lsmod") == 0)
		return LSMOD;
	if (strcmp(string, "help") == 0)
		return HELP;
	return UNKNOWN;
}

int perform_kill(int argc, char **argv, int fd)
{
	struct kill_struct ks;
	int ret;

	if (argc != 2)
		return -EINVAL;

	ks.pid = atoi(argv[1]);
	ks.sig = atoi(argv[2]);

	ret = ioctl(fd, KILL_IOCTL, &ks);

	return ret;
}

int perform_lsmod(int argc, char **argv, int fd)
{
	int ret;
	struct lsmod_struct *res, *cur;
	
	if (argc != 1)
		return -EINVAL;

	/* TODO: PAGE_SIZE may not be enough and uses a syscall */
	res = malloc(getpagesize());
	cur = res;

	ret = ioctl(fd, LSMOD_IOCTL, res);

	if (ret == 0) {
		printf("Module\t\t\tSize  Used by\n");
		while (cur->size != 0) {
			printf("%-24s%-5u %u\n", cur->name, cur->size, cur->ref);
			cur++;
		}
	}
	
	free(res);

	return ret;
}

int perform_print(int argc, char **argv, int fd)
{
	int ret;
	enum print_commands cmd;

	if (argc != 2)
		return -EINVAL;

	cmd = get_print_cmd(argv[1]);

	if (cmd == P_HELP) {
		PRINT_PRINT_HELP;
		return 0;
	}
	if (cmd == P_UNKNOWN) {
		printf("Unknown command.\nTry \"%s help\".\n", argv[0]);
		return -EINVAL;
	}
	switch (cmd) {
	case MEMINFO:
		ret = perform_print_meminfo(fd);
		break;
	case CPUINFO:
		ret = perform_print_cpuinfo(fd);
		break;
	}

	return ret;
}

enum print_commands get_print_cmd(char *string)
{
	if (strcmp(string, "meminfo") == 0)
		return MEMINFO;
	if (strcmp(string, "cpuinfo") == 0)
		return CPUINFO;
	if (strcmp(string, "help") == 0)
		return P_HELP;
	return P_UNKNOWN;
}

int perform_print_meminfo(int fd)
{
	struct sysinfo meminfo;
	unsigned int unit;
	int ret;

	ret = ioctl(fd, MEMINFO_IOCTL, &meminfo);

	if (ret == 0) {
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
	}

	return ret;
}

int perform_print_cpuinfo(int fd)
{
	printf("Work in progress\n");
	return 0;
}

int perform_wait(int argc, char **argv, int fd)
{
	return 0;
}

int perform_waitall(int argc, char **argv, int fd)
{
	return 0;
}
