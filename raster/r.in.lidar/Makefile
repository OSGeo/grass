MODULE_TOPDIR = ../..

PGM = r.in.lidar

LIBES = $(RASTERLIB) $(GISLIB) $(MATHLIB) $(GPROJLIB) $(LASLIBS)
DEPENDENCIES = $(RASTERDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(USE_LIBLAS),)
default: cmd
endif

