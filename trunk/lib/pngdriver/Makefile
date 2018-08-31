MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Vars.make

ifeq ($(USE_PNG),)
  MOD_OBJS := $(filter-out read_png.o write_png.o,$(AUTO_OBJS))
endif

LIB = PNGDRIVER
PGM = pngdriver
include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

EXTRA_CFLAGS=$(ZLIBINCPATH) $(PNGINC) -I../driver

default: lib

DOXNAME = pngdriver
