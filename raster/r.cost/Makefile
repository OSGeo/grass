MODULE_TOPDIR = ../..

PGM = r.cost

LIBES = $(SEGMENTLIB) $(RASTERLIB) $(VECTORLIB) $(GISLIB) $(MATHLIB)
DEPENDENCIES = $(SEGMENTDEP) $(RASTERDEP) $(VECTORDEP) $(GISDEP)
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
