all: $(SHNAME)

%.o: %.c
	gcc -I../shmodule -c -o $@ $<

$(SHNAME): $(SHNAME).o
	gcc -o $@ $^

clean:
	rm -rf *.o *~ $(SHNAME)

.PHONY: all clean
