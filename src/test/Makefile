TESTS = test_kill test_lsmod
CFLAGS = -I../shmodule


all: $(TESTS)

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $<

test_kill: test_kill.o
	gcc -o $@ $<

test_lsmod: test_lsmod.o
	gcc -o $@ $<

clean:
	rm -rf *~ *.o $(TESTS)

.PHONY: all clean
