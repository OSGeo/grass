MODULE_TOPDIR = ../../..

PGM = i.ortho.init

EXTRA_CFLAGS = -I../lib

LIBES     = $(IORTHOLIB) $(IMAGERYLIB) $(RASTERLIB) $(GISLIB) $(GMATHLIB)
DEPENDENCIES= $(IMAGERYDEP) $(RASTERDEP) $(IORTHODEP) $(GISDEP) $(GMATHDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd

