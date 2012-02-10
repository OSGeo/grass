
MODULE_TOPDIR = ../../..
PGM = v.topo.check

LIBES = $(VECTORLIB) $(GISLIB)
DEPENDENCIES = $(VECTORDEP) $(GISDEP)
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make
include $(MODULE_TOPDIR)/include/Make/ScriptRules.make

default: cmd
	$(MAKE) script PGM=v.rand.poly
	$(MAKE) script PGM=v.clean.test
