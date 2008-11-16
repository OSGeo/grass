MODULE_TOPDIR = ../..

PGM = r.out.tiff

include $(MODULE_TOPDIR)/include/Make/Module.make

EXTRA_CFLAGS = $(TIFFINCPATH)

LIBES = $(GISLIB) $(TIFFLIBPATH) $(TIFFLIB)
DEPENDENCIES = $(GISDEP)

ifneq ($(strip $(TIFFLIB)),)
default: cmd
endif
