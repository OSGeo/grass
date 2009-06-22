MODULE_TOPDIR = ../..

PGM = r.out.png
EXTRA_CFLAGS=$(PNGINC)

LIBES = $(GISLIB) $(RASTERLIB) $(PNGLIB)
DEPENDENCIES = $(GISDEP) $(RASTERDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(strip $(PNGLIB)),)
default: cmd
endif
