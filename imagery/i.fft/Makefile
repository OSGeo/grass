MODULE_TOPDIR = ../..

PGM = i.fft

LIBES     = $(GMATHLIB) $(GISLIB) $(RASTERLIB) $(FFTWLIB) $(MATHLIB)
DEPENDENCIES= $(GMATHDEP) $(GISDEP) $(RASTERDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(strip $(FFTWLIB)),)
default: cmd
endif
