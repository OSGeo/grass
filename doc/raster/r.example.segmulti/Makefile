# to use this file, make this relative to GRASS include/ directory
# or set -DMODULE_TOPDIR=... in make command line
# or (when everything fails) use absolute path to the GRASS source code
MODULE_TOPDIR = ../../..

PGM = r.example.segmulti

LIBES = $(GISLIB) $(RASTERLIB) $(SEGMENTLIB)
DEPENDENCIES = $(GISDEP) $(RASTERDEP) $(SEGMENTDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
