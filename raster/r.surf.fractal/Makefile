MODULE_TOPDIR = ../..

PGM = r.surf.fractal

LIBES = $(GMATHLIB) $(GISLIB) $(RASTERLIB) $(FFTWLIB)
DEPENDENCIES = $(GMATHDEP) $(GISDEP) $(RASTERDEP)
EXTRA_INC = $(FFTWINC)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(strip $(FFTWLIB)),)
default: cmd
endif
