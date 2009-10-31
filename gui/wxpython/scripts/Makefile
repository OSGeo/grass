MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Dir.make

ETCDIR = $(ETC)/gui/scripts

default: install_scripts

install_scripts:
	$(MAKE) $(ETCDIR)/d.rast3d

$(ETCDIR)/%: % | $(ETC)/gui
	$(INSTALL) $< $@

$(ETC)/gui:
	$(MKDIR) $@

clean: cleansubdirs
