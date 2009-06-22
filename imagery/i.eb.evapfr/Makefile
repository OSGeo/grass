MODULE_TOPDIR = ../..

PGM = i.eb.evapfr

LIBES = $(GISLIB) $(RASTERLIB)
DEPENDENCIES = $(GISDEP) $(RASTERDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(USE_LARGEFILES),)
	EXTRA_CFLAGS = -D_FILE_OFFSET_BITS=64
endif

default: cmd
