MODULE_TOPDIR = ../../..

PGM = i.ortho.transform

EXTRA_CFLAGS = -I../lib

LIBES = $(IMAGERYLIB) $(IORTHOLIB) $(GISLIB) $(MATHLIB)
DEPENDENCIES = $(IMAGERYDEP) $(IORTHODEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
