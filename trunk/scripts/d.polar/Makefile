MODULE_TOPDIR = ../..

PGM = d.polar

include $(MODULE_TOPDIR)/include/Make/Script.make

default: script $(ETC)/d.polar/ps_defs.eps

$(ETC)/d.polar/%: % | $(ETC)/d.polar
	$(INSTALL_DATA) $< $@

$(ETC)/d.polar:
	$(MKDIR) $@
