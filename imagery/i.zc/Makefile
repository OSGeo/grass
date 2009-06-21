MODULE_TOPDIR = ../..

PGM = i.zc

LIBES     = $(IMAGERYLIB) $(GMATHLIB) $(GISLIB) $(RASTRLIB) $(FFTWLIB)
DEPENDENCIES= $(IMAGERYDEP) $(GMATHDEP) $(GISDEP) $(RASTRDEP) $(GISDEP) $(RASTRDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(strip $(FFTWLIB)),)
default: cmd
endif
