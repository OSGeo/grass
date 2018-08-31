MODULE_TOPDIR = ../..

PGM = wxpyimgview

include $(MODULE_TOPDIR)/include/Make/Script.make

default: script $(ETC)/wxpyimgview_gui.py

$(ETC)/%: %
	$(INSTALL) $< $@
