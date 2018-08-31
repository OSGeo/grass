MODULE_TOPDIR = ../../..

PGM = i.ortho.elev

LIBES     = $(IMAGERYLIB) $(GISLIB) $(IORTHOLIB) $(GMATHLIB)
DEPENDENCIES= $(IMAGERYDEP) $(IORTHODEP) $(GISDEP) $(GMATHDEP)

EXTRA_CFLAGS = -I../lib

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
