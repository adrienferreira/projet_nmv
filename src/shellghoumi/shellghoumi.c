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
	int ret, i;
	struct lsmod_struct *res, *cur;
	struct lsmod_cmd cmd = {.data = res,
				.size = 1,
				.async = 0};
	
	if (argc != 1)
		return -EINVAL;

	cmd.data = malloc(cmd.size * sizeof(struct lsmod_struct));
	do {
		ret = ioctl(fd, LSMOD_IOCTL, &cmd);
		if (ret != 0)
			break;
		if (!cmd.done) {
			cmd.size += 10;
			free(cmd.data);
			cmd.data = malloc(cmd.size * sizeof(struct lsmod_struct));
		}
	} while (!cmd.done);

	printf("Module\t\t\tSize  Used by\n");
	for (i = 0; i < cmd.size; i++) {
		printf("%-24s%-5u %u\n", cmd.data[i].name, cmd.data[i].size, cmd.data[i].ref);
	}
	
	free(cmd.data);

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

void wait_build_struct(int argc, char*argv[], struct gen_wait_usr_struct *gwus)
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

int perform_wait(int argc, char **argv, int fd)
{
	struct gen_wait_usr_struct gwus; 
	
	if(argc == 1)
	{
		printf("Usage : ./prog <pid1>, ..., <pidN>\n");
		exit(EXIT_FAILURE);
	}
	
	wait_build_struct(argc, argv, &gwus);
	return ioctl(fd, WAIT_IOCTL, &gwus);
}

int perform_waitall(int argc, char **argv, int fd)
{
	struct gen_wait_usr_struct gwus; 
	
	if(argc == 1)
	{
		printf("Usage : ./prog <pid1>, ..., <pidN>\n");
		exit(EXIT_FAILURE);
	}
	
	wait_build_struct(argc, argv, &gwus);
	return ioctl(fd, WAITALL_IOCTL, &gwus);
}


