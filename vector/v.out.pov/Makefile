
MODULE_TOPDIR = ../..

PGM=v.out.pov

OBJS = main.o

LIBES     = $(VECTLIB) $(GISLIB) $(GDALLIBS)
DEPENDENCIES = $(VECTDEP) $(GISDEP)
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)
 
include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd

