MODULE_TOPDIR = ../..

PGM  = r.external

LIBES = $(GPROJLIB) $(RASTERLIB) $(GISLIB) $(GDALLIBS) $(MATHLIB)
DEPENDENCIES = $(GPROJDEP) $(RASTERDEP) $(GISDEP)
EXTRA_INC = $(PROJINC) $(GDALCFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(USE_GDAL),)
default: cmd
endif
