MODULE_TOPDIR = ../..

PGM=r3.mkdspf

include $(MODULE_TOPDIR)/include/Make/Module.make

LDIR = ../../lib/dspf
EXTRA_CFLAGS    =       -I$(LDIR)
# -I$(INCLUDE_DIR)

LIBES = $(DSPFLIB) $(RASTER3DLIB) $(GISLIB) $(MATHLIB)
DEPENDENCIES = $(DSPFDEP) $(RASTER3DDEP) $(GISDEP)

default: cmd
