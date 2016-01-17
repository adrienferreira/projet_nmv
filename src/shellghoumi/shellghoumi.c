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

#include "structs.h"
#include "shellghoumi.h"

int main(int argc, char **argv)
{
	int f, ret, async = 0;
	enum commands cmd;

	if (argc < 2) {
		PRINT_USAGE;
		return EXIT_SUCCESS;
	}

	if (strcmp(argv[1], "-b") == 0)
		async = 1;

	cmd = get_cmd(argv[async + 1]);

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
	if (f == -1) {
		perror("open(/dev/"CHRDEV_NAME") failed\n");
		return EXIT_FAILURE;
	}

	switch (cmd) {
	case KILL:
		ret = perform_kill(argc - 1 - async, argv + 1 + async, f, async);
		break;
	case WAIT:
		ret = perform_wait(argc - 1 - async, argv + 1 + async, f, async);
		break;
	case WAITALL:
		ret = perform_waitall(argc - 1 - async, argv + 1 + async, f, async);
		break;
	case PRINT:
		ret = perform_print(argc - 1 - async, argv + 1 + async, f, async);
		break;
	case LSMOD:
		ret = perform_lsmod(argc - 1 - async, argv + 1 + async, f, async);
		break;
	case RETURN:
		ret = perform_return(argc - 1 - async, argv + 1 + async, f);
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
	if (strcmp(string, "return") == 0)
		return RETURN;
	if (strcmp(string, "help") == 0)
		return HELP;
	return UNKNOWN;
}

int perform_kill(int argc, char **argv, int fd, int async)
{
	struct kill_struct ks;
	int ret;

	if (argc != 3){
		printf("Usage : ./prog <pid> <num_sig>\n");
		exit(EXIT_FAILURE);
	}

	ks.pid = atoi(argv[1]);
	ks.sig = atoi(argv[2]);
	ks.async = async;
	ks.id_pend = -1;

	ret = ioctl(fd, KILL_IOCTL, &ks);
	printf("Ticket : %lu \n", ks.id_pend);

	return ret;
}

int perform_lsmod(int argc, char **argv, int fd, int async)
{
	int ret, i;
	struct lsmod_struct *res = NULL;
	struct lsmod_cmd cmd = {.data = res,
				.size = 10,
				.async = async};
	
	if (argc != 1)
		return -EINVAL;

	if (!async) {
		cmd.data = malloc(cmd.size * sizeof(struct lsmod_struct));
		if (cmd.data == NULL) {
			perror("lsmod malloc() failed\n");
			return errno;
		}
	}
	do {
		ret = ioctl(fd, LSMOD_IOCTL, &cmd);
		if (ret != 0 || cmd.async)
			break;
		if (!cmd.done) {
			cmd.size += 10;
			free(cmd.data);
			cmd.data = malloc(cmd.size * sizeof(struct lsmod_struct));
		}
	} while (!cmd.done);

	if (!async) {
		print_modules(cmd.data, cmd.size);
		free(cmd.data);
	} else {
		printf("Async call got number %ld \n", cmd.id_pend);
		printf("Reclaim result with 'return %ld %lu' command\n",
		       cmd.id_pend, cmd.size * sizeof(struct lsmod_struct));
	}

	return ret;
}

int perform_print(int argc, char **argv, int fd, int async)
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
		ret = perform_print_meminfo(fd, async);
		break;
	case CPUINFO:
		ret = perform_print_cpuinfo(fd, async);
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

int perform_print_meminfo(int fd, int async)
{
	struct sysinfo meminfo;
	struct meminfo_cmd cmd = {.data = &meminfo,
				  .async = async};
	int ret = 0;

	ret = ioctl(fd, MEMINFO_IOCTL, &cmd);

	if (!async)
		print_meminfo(&meminfo);
	else {
		printf("Async call got number %ld \n", cmd.id_pend);
		printf("Reclaim result with 'return %ld %lu' command\n",
		       cmd.id_pend, sizeof(struct sysinfo));
	}

	return ret;
}

int perform_print_cpuinfo(int fd, int async)
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

int perform_wait(int argc, char **argv, int fd, int async)
{
	struct gen_wait_usr_struct gwus; 
	int ret;
	
	if(argc == 1)
	{
		printf("Usage : ./prog <pid1>, ..., <pidN>\n");
		exit(EXIT_FAILURE);
	}
	
	gwus.async = async;
	gwus.id_pend = -1;
	wait_build_struct(argc, argv, &gwus);
	ret= ioctl(fd, WAIT_IOCTL, &gwus);
	printf("Ticket : %d \n", ret);
	return ret;
}

int perform_waitall(int argc, char **argv, int fd, int async)
{
	struct gen_wait_usr_struct gwus; 
	int ret;
	
	if(argc == 1)
	{
		printf("Usage : ./prog <pid1>, ..., <pidN>\n");
		exit(EXIT_FAILURE);
	}
	
	gwus.async = async;
	gwus.id_pend = -1;
	wait_build_struct(argc, argv, &gwus);
	ret= ioctl(fd, WAITALL_IOCTL, &gwus);
	printf("Ticket : %d \n", ret);
	return ret;
}

int perform_return(int argc, char **argv, int fd)
{
	long ret = 0;
	struct return_cmd cmd;

	if (argc != 3) {
		printf("Usage: %s return <id> <size>\n", argv[-1]);
		exit(EXIT_FAILURE);
	}

	cmd.id_pend = strtol(argv[1], NULL, 10);
	cmd.size = (unsigned int) strtoul(argv[2], NULL, 10);
	cmd.data = malloc(cmd.size);

	ret = ioctl(fd, RETURN_IOCTL, &cmd);
	if (ret != 0)
		goto ret_free;
	switch (cmd.ioctl_nr) {
	case LSMOD_IOCTL:
		print_modules((struct lsmod_struct*)cmd.data, cmd.size / sizeof(struct lsmod_struct));
		break;
	case KILL_IOCTL:
		printf("Return kill : %ld\n", *((long*)cmd.data));
		break;
	case WAIT_IOCTL:
	case WAITALL_IOCTL:
		printf("Return wait : %d\n", *((int*)cmd.data));
		break;
	case MEMINFO_IOCTL:
		print_meminfo((struct sysinfo *)cmd.data);
		break;
	}

ret_free:
	free(cmd.data);

	return ret;
}

void print_modules(struct lsmod_struct *data, unsigned int size)
{
	int i;

	printf("Module\t\t\tSize  Used by\n");
	for (i = 0; i < size; i++) {
		printf("%-24s%-5u %u\n", data[i].name, data[i].size, data[i].ref);
	}
}

void print_meminfo(struct sysinfo *meminfo)
{
	unsigned int unit;

	unit = meminfo->mem_unit / 1024;
	printf("MemTotal:     %8lu kB\n"
	       "MemFree:      %8lu kB\n"
	       "MemAvailable: %8lu kB\n"      /* complicated, see fs/proc/meminfo.c */
	       "Buffers:      %8lu kB\n"
	       "SwapTotal:    %8lu kB\n"
	       "SwapFree:     %8lu kB\n"
	       "Shmem:        %8lu kB\n",
	       meminfo->totalram * unit,
	       meminfo->freeram  * unit,
	       (long unsigned int) 0,
	       meminfo->bufferram * unit,
	       meminfo->totalswap * unit,
	       meminfo->freeswap * unit,
	       meminfo->sharedram * unit);
}
