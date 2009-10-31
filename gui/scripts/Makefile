MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Dir.make

SRCFILES := $(wildcard *.*)
DSTFILES := $(patsubst %,$(ETC)/gui/scripts/%,$(SRCFILES))

default: $(DSTFILES)

$(ETC)/gui/scripts/%: % | $(ETC)/gui/scripts
	$(INSTALL) $< $@

$(ETC)/gui/scripts:
	$(MKDIR) $@
