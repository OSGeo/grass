MODULE_TOPDIR = ../../..

PGM = i.smap

EXTRA_CFLAGS = -I../bouman

LIBES     = $(ISMAPLIB) $(IMAGERYLIB) $(GISLIB) $(GMATHLIB)
DEPENDENCIES= $(ISMAPDEP) $(IMAGERYDEP) $(GISDEP) $(GMATHDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
