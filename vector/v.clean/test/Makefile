
MODULE_TOPDIR = ../../..
PGM = v.topo.check
RPOLY = $(GISBASE)/scripts/v.rand.poly
CTEST = $(GISBASE)/scripts/v.clean.test

LIBES = $(VECTLIB) $(GISLIB)
DEPENDENCIES = $(VECTDEP) $(GISDEP)
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd $(RPOLY) $(CTEST)

$(RPOLY): v.rand.poly
	if [ ! -d $(GISBASE)/scripts ]; then $(MKDIR) $(GISBASE)/scripts; fi
	$(INSTALL) v.rand.poly $(RPOLY)

$(CTEST): v.clean.test
	if [ ! -d $(GISBASE)/scripts ]; then $(MKDIR) $(GISBASE)/scripts; fi
	$(INSTALL) v.clean.test $(CTEST)

