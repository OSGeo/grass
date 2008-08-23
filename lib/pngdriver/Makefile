MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Vars.make

ifeq ($(USE_PNG),)
  MOD_OBJS := $(filter-out read_png.o write_png.o,$(AUTO_OBJS))
endif

LIB_NAME = $(PNGDRIVER_LIBNAME)
PGM = pngdriver
include $(MODULE_TOPDIR)/include/Make/Lib.make

EXTRA_CFLAGS=$(GETHOSTNAME) $(ZLIBINCPATH) $(PNGINC) -I../driver

EXTRA_LIBS=$(DRIVERLIB) $(GISLIB) $(PNGLIB)

default: lib
