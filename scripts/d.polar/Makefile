MODULE_TOPDIR = ../..

PGM = d.polar

include $(MODULE_TOPDIR)/include/Make/Script.make

default: script
	if [ ! -d $(ETC)/d.polar ] ; then $(MKDIR) $(ETC)/d.polar ; fi
	$(MAKE) $(ETC)/d.polar/ps_defs.eps

$(ETC)/d.polar/%: %
	$(INSTALL_DATA) $< $@

