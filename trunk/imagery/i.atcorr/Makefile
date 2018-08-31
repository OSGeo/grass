MODULE_TOPDIR = ../..

PGM = i.atcorr

include $(MODULE_TOPDIR)/include/Make/Module.make

LIBES = $(RASTERLIB) $(GISLIB) $(MATHLIB) $(BTREE2LIB) 
DEPENDENCIES = $(RASTERDEP) $(GISDEP)

LINK = $(CXX)

ifneq ($(CXX),)
default: cmd
endif

