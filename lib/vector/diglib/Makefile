MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Vars.make

MOD_OBJS := $(filter-out test.o port_test.o,$(AUTO_OBJS))

LIB_NAME = $(DIG2_LIBNAME)

include $(MODULE_TOPDIR)/include/Make/Lib.make

EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)
LIBES = $(GISLIB) $(RTREELIB) $(MATHLIB)

TEST = $(OBJDIR)/test

default: lib
	$(MAKE) $(TEST)

#  Test portable read/write functions
$(OBJDIR)/test: test.c $(DIG2DEP)
	echo "==============TEST============="
	$(CC) $(LDFLAGS) $(COMPILE_FLAGS) $(VECT_CFLAGS) $(INC) $(VECT_INC) -o $@ test.c $(DIG2LIB) $(RTREELIB) \
				$(GISLIB) $(DATETIMELIB) $(MATHLIB) $(XDRLIB) 
	cd $(OBJDIR); $(LD_LIBRARY_PATH_VAR)="$($(LD_LIBRARY_PATH_VAR)):$(GISBASE)/lib" ./test; diff ./test.tmp ../test.ok

