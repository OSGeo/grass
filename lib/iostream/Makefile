MODULE_TOPDIR = ../..

LIB_OBJS = mm.o mm_utils.o ami_stream.o rtimer.o
LIB_NAME = $(IOSTREAM_LIBNAME)

include $(MODULE_TOPDIR)/include/Make/Lib.make

ifneq ($(USE_LARGEFILES),)
	EXTRA_CFLAGS = -D_FILE_OFFSET_BITS=64
endif

default: stlib
