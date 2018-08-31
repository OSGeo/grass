# fix this relative to include/
# or use absolute path to the GRASS source code
MODULE_TOPDIR = ../..

PGM = r.lake

LIBES = $(RASTERLIB) $(GISLIB)
DEPENDENCIES = $(RASTERDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
