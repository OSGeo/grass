MODULE_TOPDIR = ../../..

PGM=r.sim.water

EXTRA_CLEAN_DIRS=doxygenhtml

LIBES     = $(SIMLIB) $(GMATHLIB) $(GISLIB) $(RASTERLIB) $(OPENMP_LIBPATH) $(OPENMP_LIB)
DEPENDENCIES = $(SIMDEP) $(GMATHDEP) $(GISDEP) $(RASTERDEP)
EXTRA_INC = $(VECT_INC) $(OPENMP_INCPATH)
EXTRA_CFLAGS = -I ../simlib $(VECT_CFLAGS) $(OPENMP_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd

man:
	doxygen $(MODULE_TOPDIR)/Doxyfile
