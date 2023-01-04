MODULE_TOPDIR = ../..

PGM = r.resamp.filter

LIBES = $(RASTERLIB) $(GISLIB) $(MATHLIB) $(OMPLIB)
DEPENDENCIES = $(RASTERDEP) $(GISDEP)
EXTRA_CFLAGS = $(OMPCFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
