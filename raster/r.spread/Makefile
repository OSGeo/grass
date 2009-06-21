MODULE_TOPDIR = ../..

PGM = r.spread

include $(MODULE_TOPDIR)/include/Make/Vars.make

MOD_OBJS := $(filter-out display.o,$(AUTO_OBJS))

# LIBES = $(DISPLAYLIB) $(GISLIB) $(RASTRLIB)
# DEPENDENCIES = $(DISPLAYDEP) $(GISDEP) $(RASTRDEP)
LIBES = $(GISLIB) $(RASTRLIB)
DEPENDENCIES = $(GISDEP) $(RASTRDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
