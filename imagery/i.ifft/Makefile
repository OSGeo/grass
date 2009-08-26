MODULE_TOPDIR = ../..

PGM = i.ifft

LIBES     = $(GMATHLIB) $(GISLIB) $(RASTERLIB) $(MATHLIB)
DEPENDENCIES= $(GMATHDEP) $(GISDEP) $(RASTERDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(strip $(FFTWLIB)),)
default: cmd
endif
