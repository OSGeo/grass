MODULE_TOPDIR = ../..

PGM=v.external.out

LIBES = $(GISLIB) $(GDALLIBS)
DEPENDENCIES = $(GISDEP)

EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(filter 1,$(USE_OGR) $(USE_POSTGRES)),)
default: cmd
endif
