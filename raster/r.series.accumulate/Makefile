MODULE_TOPDIR = ../..

PGM = r.series.accumulate

LIBES = $(RASTERLIB) $(GISLIB)
DEPENDENCIES = $(RASTERDEP) $(GISDEP)

EXTRA_LIBS = $(OMPLIB)
EXTRA_CFLAGS = $(OMPCFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
