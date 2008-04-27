MODULE_TOPDIR = ../..

PGM=r.in.wms

include $(MODULE_TOPDIR)/include/Make/Script.make

default: script
	$(MKDIR) $(GISBASE)/etc/r.in.wms/
	for file in r.in.gdalwarp wms.request wms.download ; do \
		$(INSTALL) $$file $(GISBASE)/etc/r.in.wms/ ; \
	done
