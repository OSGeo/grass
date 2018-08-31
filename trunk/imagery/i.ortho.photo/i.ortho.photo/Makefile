MODULE_TOPDIR = ../../..

PGM = i.ortho.photo

EXTRA_CFLAGS = -I../lib

LIBES     = $(IMAGERYLIB) $(GISLIB) $(IORTHOLIB)
DEPENDENCIES= $(IMAGERYDEP) $(IORTHODEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
