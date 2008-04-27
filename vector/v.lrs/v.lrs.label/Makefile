
MODULE_TOPDIR = ../../..

PGM = v.lrs.label

include $(MODULE_TOPDIR)/include/Make/Module.make

DEPENDENCIES = $(LRSDEP) $(VECTDEP) $(DBMIDEP) $(GISDEP)
LIBES        = $(LRSLIB) $(VECTLIB) $(DBMILIB) $(GISLIB)

EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

default: cmd

