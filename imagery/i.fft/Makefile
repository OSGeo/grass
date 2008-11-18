MODULE_TOPDIR = ../..

PGM = i.fft

LIBES     = $(GMATHLIB) $(GISLIB) $(FFTWLIB) $(MATHLIB)
DEPENDENCIES= $(GMATHDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(strip $(FFTWLIB)),)
default: cmd
endif
