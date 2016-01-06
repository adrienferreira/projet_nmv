#ifndef MISHELL_H_
#define MISHELL_H_

#define PRINT_USAGE printf("usage : %s <command> [<args>]\n\
Type \"%s help\" for a list of commands\n", argv[0], argv[0])
#define PRINT_HELP printf("Command list:\n"				\
			  "  help                        print this help\n" \
			  "  wait <pid> [<pid> ...]      wait for one of these process\n" \
			  "  waitall <pid> [<pid> ...]   wait for all these processes\n" \
			  "  print <cmd>                 print system info, " \
			  "type \"print help\" to list commands\n"	\
			  "  lsmod                       list loaded modules\n")
#define PRINT_PRINT_HELP printf("Command list:\n"			\
				"  help                  print this help\n" \
				"  meminfo               print memory info\n"\
				"  cpuinfo               print CPU info\n")

enum commands {UNKNOWN, HELP, KILL, WAIT, WAITALL, PRINT, LSMOD};

enum print_commands {P_UNKNOWN, P_HELP, MEMINFO, CPUINFO};

enum commands get_cmd(char *string);

enum print_commands get_print_cmd(char *string);

int perform_kill(int argc, char **args, int fd);

int perform_lsmod(int argc, char **args, int fd);

int perform_print(int argc, char **args, int fd);

int perform_wait(int argc, char **args, int fd);

int perform_waitall(int argc, char **args, int fd);

#endif
