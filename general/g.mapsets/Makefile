
MODULE_TOPDIR = ../..

PGM = g.mapsets
GUI = $(ETC)/gui/scripts/g.mapsets_picker.py

LIBES = $(GISLIB)
DEPENDENCIES = $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd $(GUI)

$(ETC)/gui/scripts/%: % | $(ETC)/gui/scripts
	$(INSTALL) $< $@

$(ETC)/gui/scripts:
	$(MKDIR) $@
