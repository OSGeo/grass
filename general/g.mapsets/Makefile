
MODULE_TOPDIR = ../..

PGM = g.mapsets
GUI = $(GUIDIR)/scripts/g.mapsets_picker.py

LIBES = $(GISLIB)
DEPENDENCIES = $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd $(GUI)

$(GUIDIR)/scripts/%: % | $(GUIDIR)/scripts
	$(INSTALL) $< $@

$(GUIDIR)/scripts:
	$(MKDIR) $@
