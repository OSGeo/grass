MODULE_TOPDIR = ../../..

PGM=r.sim.water

EXTRA_CLEAN_DIRS=doxygenhtml

LIBES     = $(SIMLIB) $(GMATHLIB) $(GISLIB) $(OMPLIB)
DEPENDENCIES = $(SIMDEP) $(GMATHDEP) $(GISDEP)
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = -I ../simlib $(VECT_CFLAGS) $(OMPCFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd	

man:
	doxygen $(MODULE_TOPDIR)/Doxyfile
