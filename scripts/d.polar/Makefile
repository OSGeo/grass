MODULE_TOPDIR = ../..

PGM = d.polar

include $(MODULE_TOPDIR)/include/Make/Script.make

default: script
	$(MKDIR) $(ETC)/d.polar/
	$(MAKE) $(ETC)/d.polar/ps_defs.eps

$(ETC)/d.polar/%: %
	$(INSTALL_DATA) $< $@

