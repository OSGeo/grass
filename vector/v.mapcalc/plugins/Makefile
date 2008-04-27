CC = gcc
CFLAGS = -g -Wall -Werror
LDFLAGS = -shared

all: testmod

testmod: testmod.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

testmod.o: testmod.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *~ .*~ *.o testmod
