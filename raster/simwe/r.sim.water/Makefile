MODULE_TOPDIR = ../../..

PGM=r.sim.water

EXTRA_CLEAN_DIRS=doxygenhtml

LIBES     = $(SIMLIB) $(BITMAPLIB) $(LINKMLIB) \
            $(SITESLIB) $(VECTLIB) $(VECTLIB_REAL) $(DBMILIB) $(GISLIB) $(DATETIMELIB) $(GMATHLIB)
DEPENDENCIES = $(SIMDEP) $(BITMAPDEP) $(LINKMDEP) \
            $(VECTDEP) $(DBMIDEP) $(GISDEP) $(DATETIMEDEP) $(GMATHDEP)
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = -I ../simlib $(VECT_CFLAGS) 

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd	

man:
	doxygen $(MODULE_TOPDIR)/Doxyfile
