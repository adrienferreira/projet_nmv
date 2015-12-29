TESTS = test_kill test_lsmod test_meminfo test_wait test_waitall
CFLAGS = -I../shmodule
WAITS_SRC=test_waits.c

all: $(TESTS)

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $<

test_kill: test_kill.o
	gcc -o $@ $<

test_lsmod: test_lsmod.o
	gcc -o $@ $<

test_meminfo: test_meminfo.o
	gcc -o $@ $<

test_wait: $(WAITS_SRC)
	gcc $(CFLAGS) -o $@ $<

test_waitall: $(WAITS_SRC)
	gcc $(CFLAGS) -DWAITALL_CMD -o $@ $<

clean:
	rm -rf *~ *.o $(TESTS)

.PHONY: all clean
