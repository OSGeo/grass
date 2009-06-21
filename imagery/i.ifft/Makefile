MODULE_TOPDIR = ../..

PGM = i.ifft

LIBES     = $(GMATHLIB) $(GISLIB) $(RASTRLIB) $(FFTWLIB) $(MATHLIB)
DEPENDENCIES= $(GMATHDEP) $(GISDEP) $(RASTRDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(strip $(FFTWLIB)),)
default: cmd
endif
