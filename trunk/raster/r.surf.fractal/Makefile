MODULE_TOPDIR = ../..

PGM = r.surf.fractal

LIBES = $(GMATHLIB) $(RASTERLIB) $(GISLIB) $(MATHLIB)
DEPENDENCIES = $(GMATHDEP) $(RASTERDEP) $(GISDEP)
EXTRA_INC = $(FFTWINC)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(strip $(FFTWLIB)),)
default: cmd
endif
