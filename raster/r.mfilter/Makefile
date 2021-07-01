MODULE_TOPDIR = ../..

PGM = r.mfilter

LIBES = $(ROWIOLIB) $(RASTERLIB) $(GISLIB) $(OMPLIB)
DEPENDENCIES = $(ROWIODEP) $(RASTERDEP) $(GISDEP)
EXTRA_CFLAGS = $(OMPCFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
