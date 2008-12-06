MODULE_TOPDIR = ../..

PGM  = r.external.out

LIBES= $(GISLIB) $(GDALLIBS) $(GMATHLIB)
DEPENDENCIES = $(GISDEP) $(GMATHDEP)
EXTRA_INC = $(PROJINC) $(GDALCFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(USE_GDAL),)
default: cmd
endif
