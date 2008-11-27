MODULE_TOPDIR = ../..

PGM=r.in.wms

include $(MODULE_TOPDIR)/include/Make/Script.make

SRCFILES = r.in.gdalwarp wms.request wms.download
DSTFILES := $(patsubst %,$(ETC)/r.in.wms/%,$(SRCFILES))

default: script
	if [ ! -d $(ETC)/r.in.wms ] ; then $(MKDIR) $(ETC)/r.in.wms ; fi
	$(MAKE) $(DSTFILES)

$(ETC)/r.in.wms/%: %
	$(INSTALL) $< $@
