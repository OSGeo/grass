MODULE_TOPDIR = ../..

PGM = r.surf.fractal

LIBES = $(GMATHLIB) $(GISLIB) $(RASTRLIB) $(FFTWLIB)
DEPENDENCIES = $(GMATHDEP) $(GISDEP) $(RASTRDEP)
EXTRA_INC = $(FFTWINC)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(strip $(FFTWLIB)),)
default: cmd
endif
