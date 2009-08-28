MODULE_TOPDIR = ../..

PGM = i.eb.eta

LIBES = $(RASTERLIB) $(GISLIB)
DEPENDENCIES = $(RASTERDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(USE_LARGEFILES),)
	EXTRA_CFLAGS = -D_FILE_OFFSET_BITS=64
endif

default: cmd
