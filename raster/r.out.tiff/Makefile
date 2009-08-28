MODULE_TOPDIR = ../..

PGM = r.out.tiff

include $(MODULE_TOPDIR)/include/Make/Module.make

EXTRA_CFLAGS = $(TIFFINCPATH)

LIBES = $(RASTERLIB) $(GISLIB) $(TIFFLIBPATH) $(TIFFLIB)
DEPENDENCIES = $(RASTERDEP) $(GISDEP)

ifneq ($(strip $(TIFFLIBPATH) $(TIFFLIB)),)
default: cmd
endif
