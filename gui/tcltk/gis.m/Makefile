MODULE_TOPDIR = ../../..

PGM=gis.m

include $(MODULE_TOPDIR)/include/Make/Script.make

default: script install_scripts myhtml

install_scripts:
	if [ ! -d $(GISBASE)/etc/gm ] ; then $(MKDIR) $(GISBASE)/etc/gm; fi
	#$(INSTALL) description.html $(GISBASE)/docs/html/gis.m.html
	for file in *.tcl ; do $(INSTALL) $$file $(GISBASE)/etc/gm/ ; done
	for file in *.gif ; do $(INSTALL_DATA) $$file $(GISBASE)/etc/gm/ ; done
	for file in docs/gm_*.html ; do $(INSTALL_DATA) $$file $(GISBASE)/docs/html/ ; done
	for file in docs/gm_*.png ; do $(INSTALL_DATA) $$file $(GISBASE)/docs/html/ ; done
	for file in docs/gm_*.jpg ; do $(INSTALL_DATA) $$file $(GISBASE)/docs/html/ ; done

myhtml:
	$(MAKE) htmlscript PGM=gis.m


$(BIN)/$(PGM).bat: $(PGM).bat
	$(INSTALL_DATA) $< $@
