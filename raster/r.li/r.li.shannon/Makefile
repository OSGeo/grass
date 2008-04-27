MODULE_TOPDIR = ../../..

PGM = r.li.shannon

EXTRA_CLEAN_DIRS=doxygenhtml

LIB_NAME      = grass_rli

RLI_LIB = -l$(LIB_NAME)

LIBES = $(GISLIB) $(RLI_LIB)

DEPENDENCIES = $(GISDEP) $(LIB_NAME )

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd

