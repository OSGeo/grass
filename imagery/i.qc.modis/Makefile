MODULE_TOPDIR = ../..

PGM = i.qc.modis

LIBES = $(GISLIB)
DEPENDENCIES = $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(USE_LARGEFILES),)
	EXTRA_CFLAGS = -D_FILE_OFFSET_BITS=64
endif

default: cmd
