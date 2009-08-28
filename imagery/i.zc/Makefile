MODULE_TOPDIR = ../..

PGM = i.zc

LIBES = $(GMATHLIB) $(RASTERLIB) $(GISLIB)
DEPENDENCIES = $(GMATHDEP) $(RASTERDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(strip $(FFTWLIB)),)
default: cmd
endif
