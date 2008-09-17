
MODULE_TOPDIR = ../..

PGM = g.mapsets
GUI = $(ETC)/g.mapsets.tcl

LIBES     = $(GISLIB)
DEPENDENCIES= $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd $(GUI)

$(ETC)/%: %
	if [ ! -d $(ETC) ]; then $(MKDIR) $(ETC); fi
	$(INSTALL) $< $@
