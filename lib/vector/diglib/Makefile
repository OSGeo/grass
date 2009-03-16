MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Vars.make

MOD_OBJS := $(filter-out test.o port_test.o,$(AUTO_OBJS))

LIB_NAME = $(DIG2_LIBNAME)

include $(MODULE_TOPDIR)/include/Make/Lib.make

EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)
LIBES = $(GISLIB) $(RTREELIB) $(MATHLIB)

#compile if LFS (Large File Support) present:
ifneq ($(USE_LARGEFILES),)
	EXTRA_CFLAGS += -D_FILE_OFFSET_BITS=64
	TESTFILE = test64.ok
else
	TESTFILE = test32.ok
endif

TEST = $(OBJDIR)/test

default: lib
	$(MAKE) $(TEST)

#  Test portable read/write functions
$(OBJDIR)/test: test.c $(DIG2DEP)
	echo "==============TEST============="
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(LDFLAGS) $(INC) $(VECT_INC) -o $@ test.c $(DIG2LIB) $(RTREELIB) \
				$(GISLIB) $(DATETIMELIB) $(MATHLIB) $(XDRLIB) 
	cd $(OBJDIR); $(LD_LIBRARY_PATH_VAR)="$($(LD_LIBRARY_PATH_VAR)):$(GISBASE)/lib" ./test; diff ./test.tmp ../$(TESTFILE)

