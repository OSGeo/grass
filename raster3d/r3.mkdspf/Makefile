MODULE_TOPDIR = ../..

PGM=r3.mkdspf

include $(MODULE_TOPDIR)/include/Make/Module.make

LDIR = ../../lib/dspf
EXTRA_CFLAGS    =       -I$(LDIR)
# -I$(INCLUDE_DIR)

LIBES = $(DSPFLIB) $(RASTER3DLIB) $(RASTERLIB) $(GISLIB) $(MATHLIB)
DEPENDENCIES = $(DSPFDEP) $(RASTER3DDEP) $(RASTERDEP) $(GISDEP)

default: cmd
