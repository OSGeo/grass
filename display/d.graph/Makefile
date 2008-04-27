MODULE_TOPDIR = ../..

PGM = d.graph

LIBES     = $(SYMBLIB) $(DISPLAYLIB) $(RASTERLIB) $(GISLIB)
DEPENDENCIES= $(SYMBDEP) $(DISPLAYDEP) $(RASTERDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
	$(INSTALL_DATA) grass_logo.txt $(GISBASE)/docs/html
