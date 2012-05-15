MODULE_TOPDIR = ../..

PGM=r3.out.netcdf

LIBES = $(RASTER3DLIB) $(GISLIB) $(GPROJLIB)
DEPENDENCIES = $(RASTER3DDEP) $(GISDEP) $(GPROJDEP)

EXTRA_INC = $(PROJINC)

EXTRA_LIBS = -lnetcdf

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
