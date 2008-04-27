
MODULE_TOPDIR = ../..

LIB_NAME = $(BTREE_LIBNAME)

LIB_OBJS = \
	create.o\
	find.o\
	free.o\
	next.o\
	rewind.o\
	update.o

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: lib

try: $(OBJDIR)/try.o $(BTREELIB)
	$(CC) $(LDFLAGS) $(OBJDIR)/try.o $(BTREELIB) -o $(OBJDIR)/try
