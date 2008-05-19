MODULE_TOPDIR = ../../..

EXTRA_LIBS=$(GISLIB) $(RTREELIB) $(MATHLIB)

LIB_NAME = $(DIG2_LIBNAME)

LIB_OBJS = \
    allocation.o  \
    angle.o  \
    box.o  \
    cindex.o  \
    cindex_rw.o  \
    file.o \
    frmt.o \
    head.o \
    inside.o \
    linecros.o \
    line_dist.o \
    list.o \
    plus.o \
    plus_area.o  \
    plus_line.o \
    plus_node.o \
    plus_struct.o \
    poly.o \
    port_init.o \
    portable.o \
    prune.o \
    spindex.o  \
    spindex_rw.o  \
    struct_alloc.o \
    type.o \
    update.o


include $(MODULE_TOPDIR)/include/Make/Lib.make

EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

TEST = $(OBJDIR)/test

default: lib
	$(MAKE) $(TEST)

#  Test portable read/write functions
$(OBJDIR)/test: test.c $(DEPENDENCIES) $(ARCH_LIB_OBJS)
	echo "==============TEST============="
	$(CC) $(LDFLAGS) $(COMPILE_FLAGS) $(VECT_CFLAGS) $(INC) $(VECT_INC) -o $@ test.c $(DIG2LIB) $(RTREELIB) \
				$(GISLIB) $(DATETIMELIB) $(MATHLIB) $(XDRLIB) 
	cd $(OBJDIR); $(LD_LIBRARY_PATH_VAR)="$($(LD_LIBRARY_PATH_VAR)):$(GISBASE)/lib" ./test; diff ./test.tmp ../test.ok

