MODULE_TOPDIR = ../..

PGM=r.in.wms

include $(MODULE_TOPDIR)/include/Make/Script.make

SRCFILES = wms_parse.py wms_request.py wms_download.py gdalwarp.py r.in.gdalwarp.py
DSTFILES := $(patsubst %,$(ETC)/r.in.wms/%,$(SRCFILES))

default: script
	if [ ! -d $(ETC)/r.in.wms ] ; then $(MKDIR) $(ETC)/r.in.wms ; fi
	$(MAKE) $(DSTFILES)

$(ETC)/r.in.wms/%: %
	$(INSTALL) $< $@
