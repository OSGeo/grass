MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Platform.make

EXTRA_CFLAGS=$(FTINC)
EXTRA_LIBS=$(GISLIB) $(FTLIB) $(ICONVLIB)
LIB_NAME = $(DRIVER_LIBNAME)

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: lib

