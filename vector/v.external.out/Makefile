MODULE_TOPDIR = ../..

PGM=v.external.out

LIBES = $(VECTORLIB) $(GISLIB) $(GDALLIBS)
DEPENDENCIES = $(VECTORDEP) $(GISDEP)

EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(USE_OGR),)
default: cmd
endif
