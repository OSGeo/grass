MODULE_TOPDIR = ../..

PGM  = r.external.out

LIBES = $(GISLIB) $(GDALLIBS)
DEPENDENCIES = $(GISDEP)
EXTRA_INC = $(PROJINC) $(GDALCFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(USE_GDAL),)
default: cmd
endif
