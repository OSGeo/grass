MODULE_TOPDIR = ../..

PGM = d.rast.edit

include $(MODULE_TOPDIR)/include/Make/Script.make

default: script $(ETC)/d.rast.edit.tcl

$(ETC)/d.rast.edit.tcl: d.rast.edit.tcl
	$(INSTALL_DATA) $^ $@
