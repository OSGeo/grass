MODULE_TOPDIR = ../..

PGM = r.patch

LIBES = $(RASTERLIB) $(GISLIB) $(OMPLIB)
DEPENDENCIES = $(RASTERDEP) $(GISDEP)
EXTRA_CFLAGS = $(OMPCFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
