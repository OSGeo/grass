
MODULE_TOPDIR = ../../..

LIB_NAME = $(TRANS_LIBNAME)

include $(MODULE_TOPDIR)/include/Make/Lib.make

#compile if LFS (Large File Support) present:
ifneq ($(USE_LARGEFILES),)
	EXTRA_CFLAGS += -D_FILE_OFFSET_BITS=64
endif

default: lib




