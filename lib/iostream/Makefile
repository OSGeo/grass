MODULE_TOPDIR = ../..

LIB_NAME = $(IOSTREAM_LIBNAME)

include $(MODULE_TOPDIR)/include/Make/Lib.make

ifneq ($(USE_LARGEFILES),)
	EXTRA_CFLAGS = -D_FILE_OFFSET_BITS=64
endif

ifneq ($(strip $(CXX)),)
default: stlib
else
default:
endif
