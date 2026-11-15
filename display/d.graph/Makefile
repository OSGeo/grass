MODULE_TOPDIR = ../..

PGM = d.graph

LIBES = $(DISPLAYLIB) $(SYMBLIB) $(GISLIB)
DEPENDENCIES = $(DISPLAYDEP) $(SYMBDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd $(HTMLDIR)/grass_logo.txt $(MDDIR)/source/grass_logo.txt

$(HTMLDIR)/%.txt: %.txt
	$(INSTALL_DATA) $< $@

$(MDDIR)/source/%.txt: %.txt
	$(INSTALL_DATA) $< $@
