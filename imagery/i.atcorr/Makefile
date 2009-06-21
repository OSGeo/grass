MODULE_TOPDIR = ../..

PGM = i.atcorr

include $(MODULE_TOPDIR)/include/Make/Module.make

LIBES = $(GISLIB) $(RASTRLIB) 
DEPENDENCIES = $(GISDEP) $(RASTRDEP)

LINK = $(CXX)

ifneq ($(CXX),)
default: cmd
endif
