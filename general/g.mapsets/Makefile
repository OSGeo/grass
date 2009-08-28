
MODULE_TOPDIR = ../..

PGM = g.mapsets
GUI = $(ETC)/gui/scripts/g.mapsets_picker.py

LIBES = $(GISLIB)
DEPENDENCIES = $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd $(GUI)

$(ETC)/gui/scripts/%: %
	if [ ! -d $(ETC) ]; then $(MKDIR) $(ETC); fi
	if [ ! -d $(ETC)/gui ]; then $(MKDIR) $(ETC)/gui; fi
	if [ ! -d $(ETC)/gui/scripts ]; then $(MKDIR) $(ETC)/gui/scripts; fi
	$(INSTALL) $< $@
