MODULE_TOPDIR = ../..

PGM = v.fill.holes

LIBES = $(VECTORLIB) $(DBMILIB) $(GISLIB)
DEPENDENCIES = $(VECTORDEP) $(DBMIDEP) $(GISDEP)
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
