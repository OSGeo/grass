
MODULE_TOPDIR = ../..

PGM = v.label.sa

LIBES = $(DISPLAYLIB) $(RASTERLIB) $(VECTLIB) $(GISLIB) $(FTLIB)
DEPENDENCIES= $(DISPLAYDEP) $(RASTERDEP) $(VECTDEP) $(GISDEP)
EXTRA_INC = $(VECT_INC) $(FTINC)
EXTRA_CFLAGS = $(VECT_CFLAGS) -Wall -Wmissing-prototypes -std=c99

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd

