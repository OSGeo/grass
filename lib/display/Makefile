MODULE_TOPDIR = ../..

EXTRA_INC = -I../driver

include $(MODULE_TOPDIR)/include/Make/Vars.make
ifneq ($(USE_CAIRO),)
EXTRA_CFLAGS += -DUSE_CAIRO
endif

LIB = DISPLAY

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

default: lib

#doxygen:
DOXNAME=display
