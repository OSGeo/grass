MODULE_TOPDIR = ../../../..

PGM=test.rtree.lib

LIBES = $(VECTORLIB) $(GISLIB) $(RTREELIB)
DEPENDENCIES = $(VECTORDEP) $(GISDEP) $(RTREEDEP)

EXTRA_CFLAGS = $(VECT_CFLAGS)
EXTRA_INC = $(VECT_INC)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
