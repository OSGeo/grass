MODULE_TOPDIR = ../..

PGM = i.fft

LIBES     = $(IMAGERYLIB) $(GMATHLIB) $(GISLIB) $(FFTWLIB)
DEPENDENCIES= $(IMAGERYDEP) $(GMATHDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(strip $(FFTWLIB)),)
default: cmd
endif
