MODULE_TOPDIR = ../..

PGM = r.out.tiff

include $(MODULE_TOPDIR)/include/Make/Module.make

EXTRA_CFLAGS = $(TIFFINCPATH)

LIBES = $(GISLIB) $(RASTERLIB) $(TIFFLIBPATH) $(TIFFLIB)
DEPENDENCIES = $(GISDEP) $(RASTERDEP)

ifneq ($(strip $(TIFFLIB)),)
default: cmd
endif
