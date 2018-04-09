MODULE_TOPDIR = ../..

PGM = g.proj

EXTRA_INC = $(PROJINC) $(GDALCFLAGS)
LIBES = $(GPROJLIB) $(GISLIB) $(GDALLIBS) $(PROJLIB)
DEPENDENCIES = $(GPROJDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
