
MODULE_TOPDIR = ../..

PGM = v.clean

LIBES = $(VECTORLIB) $(GISLIB)
DEPENDENCIES = $(VECTORDEP) $(GISDEP)
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)
CLEAN_SUBDIRS = test

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd 

ctest:
	$(MAKE) -C test

