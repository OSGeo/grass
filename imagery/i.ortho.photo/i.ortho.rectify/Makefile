MODULE_TOPDIR = ../../..

PGM = i.ortho.rectify

EXTRA_CFLAGS = -I../lib

LIBES     = $(IMAGERYLIB) $(RASTERLIB) $(GISLIB) $(IORTHOLIB) $(GMATHLIB)
DEPENDENCIES= $(IMAGERYDEP) $(RASTERDEP) $(IORTHODEP) $(GISDEP) $(GMATHDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd

