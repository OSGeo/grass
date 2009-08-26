MODULE_TOPDIR = ../..

PGM = i.zc

LIBES     = $(IMAGERYLIB) $(GMATHLIB) $(GISLIB) $(RASTERLIB)
DEPENDENCIES= $(IMAGERYDEP) $(GMATHDEP) $(GISDEP) $(RASTERDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(strip $(FFTWLIB)),)
default: cmd
endif
