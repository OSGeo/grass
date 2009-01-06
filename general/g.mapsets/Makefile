
MODULE_TOPDIR = ../..

PGM = g.mapsets
GUI = $(ETC)/gui/g.mapsets.py

LIBES     = $(GISLIB)
DEPENDENCIES= $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd $(GUI)

$(ETC)/gui/%: %
	if [ ! -d $(ETC) ]; then $(MKDIR) $(ETC); fi
	if [ ! -d $(ETC)/gui ]; then $(MKDIR) $(ETC)/gui; fi
	$(INSTALL) $< $@
