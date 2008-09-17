MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Dir.make

SRCFILES := $(wildcard *.*)
DSTFILES := $(patsubst %,$(ETC)/gui/scripts/%,$(SRCFILES))

default:
	if [ ! -d $(ETC)/gui ] ; then $(MKDIR) $(ETC)/gui ; fi
	if [ ! -d $(ETC)/gui/scripts ] ; then $(MKDIR) $(ETC)/gui/scripts ; fi
	$(MAKE) $(DSTFILES)

$(ETC)/gui/scripts/%: %
	$(INSTALL) $< $@
