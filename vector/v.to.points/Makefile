
MODULE_TOPDIR = ../..

PGM = v.to.points

include $(MODULE_TOPDIR)/include/Make/Module.make

DEPENDENCIES = $(VECTORDEP) $(DBMIDEP) $(GISDEP)

LIBES = $(VECTORLIB) $(DBMILIB) $(GISLIB) $(MATHLIB)

EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

default: cmd

