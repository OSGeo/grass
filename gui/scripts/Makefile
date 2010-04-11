MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Dir.make
include $(MODULE_TOPDIR)/include/Make/Python.make

ETCDIR = $(ETC)/gui/scripts

SRCFILES := $(wildcard *.py)
DSTFILES := $(patsubst %,$(ETCDIR)/%,$(SRCFILES)) $(patsubst %.py,$(ETCDIR)/%.pyc,$(filter %.py,$(SRCFILES)))

default: $(DSTFILES)

$(ETCDIR)/%: % | $(ETCDIR)
	$(INSTALL) $< $@

$(ETCDIR):
	$(MKDIR) $@
