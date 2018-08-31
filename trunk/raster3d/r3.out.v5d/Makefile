MODULE_TOPDIR = ../..

PGM=r3.out.v5d

#for Linux/x86 et al. (needs fix for MacOSX)
EXTRA_CFLAGS = -DLITTLE

LIBES = $(RASTER3DLIB) $(GISLIB) $(MATHLIB)
DEPENDENCIES = $(RASTER3DDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
