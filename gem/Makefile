CC=gcc
CFLAGS=-g -Wall -pedantic -Werror-implicit-function-declaration -fno-common
CLIBS=

SRCS =  at_exit_funcs.c \
	error.c \
	tools.c \
	reg_deps.c \
	reg_entries.c \
	reg_html.c \
	actions.c \
	main.c

HDRS =	globals.h \
	at_exit_funcs.h \
	error.h \
	tools.h \
	reg_deps.h \
	reg_entries.h \
	reg_html.h \
	actions.h

OBJS =	at_exit_funcs.o \
	error.o \
	tools.o \
	reg_deps.o \
	reg_entries.o \
	reg_html.o \
	actions.o \
	main.o

default: $(SRCS) $(HDRS) $(OBJS)
	$(CC) $(OBJS) -o gem6 $(CFLAGS) $(CLIBS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

at_exit_funcs.o: at_exit_funcs.h globals.h

error.o: error.h globals.h

tools.o: tools.h globals.h

reg_deps.o: reg_deps.h globals.h

reg_entries.o: reg_entries.h globals.h

reg_html.o: reg_html.h globals.h

actions.o: actions.h globals.h

main.o:	globals.h

install: gem6
	strip gem6;
	cp gem6 /usr/local/bin

clean:
	-rm -f *.o *.bak core *~ gem6

