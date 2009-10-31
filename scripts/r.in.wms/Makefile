MODULE_TOPDIR = ../..

PGM=r.in.wms

include $(MODULE_TOPDIR)/include/Make/Script.make

SRCFILES = wms_parse.py wms_request.py wms_download.py gdalwarp.py r.in.gdalwarp.py wms_gdal.py
DSTFILES := $(patsubst %,$(ETC)/r.in.wms/%,$(SRCFILES))

default: script $(DSTFILES)

$(ETC)/r.in.wms/%: % | $(ETC)/r.in.wms
	$(INSTALL) $< $@

$(ETC)/r.in.wms:
	$(MKDIR) $@
