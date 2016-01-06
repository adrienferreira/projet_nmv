all: mishell

%.o: %.c
	gcc -I../shmodule -c -o $@ $<

mishell: mishell.o
	gcc -o $@ $^

clean:
	rm -rf *.o *~ mishell

.PHONY: all clean
