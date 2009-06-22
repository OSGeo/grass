MODULE_TOPDIR = ../..

PGM = r.grow.distance

LIBES = $(GISLIB) $(RASTERLIB) $(MATHLIB)
DEPENDENCIES = $(GISDEP) $(RASTERDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(USE_LARGEFILES),)
	EXTRA_CFLAGS = -D_FILE_OFFSET_BITS=64
endif

default: cmd
