MODULE_TOPDIR = ../..

PGM = i.ifft

LIBES = $(GMATHLIB) $(RASTERLIB) $(GISLIB)
DEPENDENCIES = $(GMATHDEP) $(RASTERDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(strip $(FFTWLIB)),)
default: cmd
endif
