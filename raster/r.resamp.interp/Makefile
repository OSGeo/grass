MODULE_TOPDIR = ../..

PGM = r.resamp.interp

LIBES = $(RASTERLIB) $(GISLIB) $(MATHLIB) $(OMPLIB)
DEPENDENCIES = $(RASTERDEP) $(GISDEP)
EXTRACFLAGS = $(OMPCLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
