
MODULE_TOPDIR = ../..

LIB_NAME = $(LINKM_LIBNAME)

LIB_OBJS = \
	init.o\
	new.o\
	next.o\
	oom.o\
	dispose.o

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: lib

try: try.o $(LINKMLIB)
	$(CC) $(LDFLAGS) try.o $(LINKMLIB) -o try

try2: try2.o $(LINKMLIB)
	$(CC) $(LDFLAGS) try2.o $(LINKMLIB) -o try2
