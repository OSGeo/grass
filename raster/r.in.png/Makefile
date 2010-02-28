MODULE_TOPDIR = ../..

PGM = r.in.png
EXTRA_CFLAGS=$(PNGINC)

LIBES = $(RASTERLIB) $(GISLIB) $(PNGLIB)
DEPENDENCIES = $(RASTERDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(strip $(PNGLIB)),)
default: cmd
endif
