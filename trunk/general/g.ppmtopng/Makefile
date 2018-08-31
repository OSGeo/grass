
MODULE_TOPDIR = ../..

PGM = g.ppmtopng
LIBES = $(GISLIB) $(PNGLIB)
DEPENDENCIES = $(GISDEP)

EXTRA_CFLAGS=$(ZLIBINCPATH) $(PNGINC)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(USE_PNG),)
default: cmd
endif

