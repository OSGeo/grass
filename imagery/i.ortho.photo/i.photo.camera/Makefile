MODULE_TOPDIR = ../../..

PGM = i.ortho.camera

EXTRA_CFLAGS = -I../lib

LIBES     = $(IORTHOLIB) $(IMAGERYLIB) $(GISLIB) $(GMATHLIB)
DEPENDENCIES= $(IMAGERYDEP) $(IORTHODEP) $(GISDEP) $(GMATHDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
