
MODULE_TOPDIR = ../..

PGM=v.transform

include $(MODULE_TOPDIR)/include/Make/Module.make

DEPENDENCIES = $(VECTDEP) $(DBMIDEP) $(GISDEP)
LIBES     = $(VECTLIB) $(DBMILIB) $(TRANSLIB) $(EXTRA_LIB) $(GISLIB)
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

default: cmd

