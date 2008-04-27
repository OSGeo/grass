
MODULE_TOPDIR = ../..

PGM = v.to.points

include $(MODULE_TOPDIR)/include/Make/Module.make

DEPENDENCIES = $(VECTDEP) $(DBMIDEP) $(GISDEP)

LIBES        = $(VECTLIB) $(DBMILIB) $(GISLIB)

EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

default: cmd

