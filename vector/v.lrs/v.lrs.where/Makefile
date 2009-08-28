
MODULE_TOPDIR = ../../..

PGM = v.lrs.where

include $(MODULE_TOPDIR)/include/Make/Module.make

DEPENDENCIES = $(LRSDEP) $(VECTORDEP) $(DBMIDEP) $(GISDEP)
LIBES = $(LRSLIB) $(VECTORLIB) $(DBMILIB) $(GISLIB)

EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

default: cmd

