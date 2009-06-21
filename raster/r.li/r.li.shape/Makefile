MODULE_TOPDIR = ../../..

PGM = r.li.shape

EXTRA_CLEAN_DIRS=doxygenhtml

LIB_NAME      = grass_rli

RLI_LIB = -l$(LIB_NAME)

LIBES = $(GISLIB) $(RASTRLIB) $(RLI_LIB)

DEPENDENCIES = $(GISDEP) $(RASTRDEP) $(LIB_NAME )

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd

