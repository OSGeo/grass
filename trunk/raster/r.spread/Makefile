MODULE_TOPDIR = ../..

PGM = r.spread

include $(MODULE_TOPDIR)/include/Make/Vars.make

MOD_OBJS := $(filter-out display.o,$(AUTO_OBJS))

# LIBES = $(DISPLAYLIB) $(GISLIB) $(RASTERLIB)
# DEPENDENCIES = $(DISPLAYDEP) $(GISDEP) $(RASTERDEP)
LIBES = $(RASTERLIB) $(GISLIB) $(MATHLIB)
DEPENDENCIES = $(RASTERDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
