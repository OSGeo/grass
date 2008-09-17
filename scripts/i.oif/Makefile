MODULE_TOPDIR = ../..

PGM=i.oif

include $(MODULE_TOPDIR)/include/Make/Script.make

SRCFILES = i.oifcalc m.cutmatrix
DSTFILES := $(patsubst %,$(ETC)/i.oif/%,$(SRCFILES))

default: script
	$(MKDIR) $(ETC)/i.oif/
	$(MAKE) $(DSTFILES)

$(ETC)/i.oif/%: %
	$(INSTALL) $< $@
