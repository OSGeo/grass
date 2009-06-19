MODULE_TOPDIR = ../..

PGM = d.graph

LIBES     = $(SYMBLIB) $(DISPLAYLIB) $(GISLIB)
DEPENDENCIES= $(SYMBDEP) $(DISPLAYDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd $(HTMLDIR)/grass_logo.txt

$(HTMLDIR)/%.txt: %.txt
	$(INSTALL_DATA) $< $@
