MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Dir.make

default:
	$(MAKE) $(ETC)/gui/images/startup_banner.png

$(ETC)/gui/images/%: % | $(ETC)/gui/images
	$(INSTALL_DATA) $< $@

$(ETC)/gui/images:
	$(MKDIR) $@
