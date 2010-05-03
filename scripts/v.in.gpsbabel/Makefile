MODULE_TOPDIR = ../..

PGM = v.in.gps

include $(MODULE_TOPDIR)/include/Make/Script.make

default: script
	$(MAKE) $(ETC)/grass_write_ascii.style

$(ETC)/%: %
	$(INSTALL_DATA) $< $@
