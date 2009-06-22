MODULE_TOPDIR = ../..

PGM = i.atcorr

include $(MODULE_TOPDIR)/include/Make/Module.make

LIBES = $(GISLIB) $(RASTERLIB) 
DEPENDENCIES = $(GISDEP) $(RASTERDEP)

LINK = $(CXX)

ifneq ($(CXX),)
default: cmd
endif
