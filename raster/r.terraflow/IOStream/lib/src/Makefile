MODULE_TOPDIR = ../../../../..

LIB_OBJS = mm.o mm_utils.o ami_stream.o rtimer.o
LIB_NAME = iostream

include $(MODULE_TOPDIR)/include/Make/Lib.make

TPIEINC = ../../include
EXTRA_INC = -I$(TPIEINC)
ifneq ($(USE_LARGEFILES),)
	EXTRA_CFLAGS = -D_FILE_OFFSET_BITS=64
endif

default: stlib

