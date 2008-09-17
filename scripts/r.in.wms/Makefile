MODULE_TOPDIR = ../..

PGM=r.in.wms

include $(MODULE_TOPDIR)/include/Make/Script.make

SRCFILES = r.in.gdalwarp wms.request wms.download
DSTFILES := $(patsubst %,$(ETC)/r.in.wms/%,$(SRCFILES))

default: script
	$(MKDIR) $(ETC)/r.in.wms/
	$(MAKE) $(DSTFILES)

$(ETC)/r.in.wms/%: %
	$(INSTALL) $< $@
