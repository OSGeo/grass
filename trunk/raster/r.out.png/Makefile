MODULE_TOPDIR = ../..

PGM = r.out.png
EXTRA_CFLAGS=$(PNGINC)

LIBES = $(RASTERLIB) $(GISLIB) $(PNGLIB)
DEPENDENCIES = $(RASTERDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(strip $(PNGLIB)),)
default: cmd
endif
