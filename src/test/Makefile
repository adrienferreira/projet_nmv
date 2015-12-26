TESTS = test_kill
CFLAGS = -I../shmodule


all: $(TESTS)

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $<

test_kill: test_kill.o
	gcc -o $@ $<

clean:
	rm -rf *~ *.o $(TESTS)

.PHONY: all clean
