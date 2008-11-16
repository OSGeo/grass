MODULE_TOPDIR = ../..

EXTRA_CFLAGS=-I../driver $(CAIROINC)
EXTRA_LIBS=$(DRIVERLIB) $(GISLIB) $(CAIROLIB)

# added constant to /include/Make/Grass.make.in
LIB_NAME = $(CAIRODRIVER_LIBNAME)
PGM = cairodriver

include $(MODULE_TOPDIR)/include/Make/Lib.make

ifneq ($(USE_CAIRO),)
default: lib
endif

