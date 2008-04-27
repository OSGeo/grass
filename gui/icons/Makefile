MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Dir.make

default:
	if [ ! -d $(GISBASE)/etc/gui ] ; then $(MKDIR) $(GISBASE)/etc/gui ; fi
	if [ ! -d $(GISBASE)/etc/gui/icons ] ; then $(MKDIR) $(GISBASE)/etc/gui/icons ; fi
	@# if [ ! -d $(GISBASE)/etc/gui/icons/silk ] ; then $(MKDIR) $(GISBASE)/etc/gui/icons/silk ; fi
	for icon in *.gif ; do $(INSTALL_DATA) $$icon $(GISBASE)/etc/gui/icons ; done
	@# for icon in silk/*.png ; do $(INSTALL_DATA) $$icon $(GISBASE)/etc/gui/icons/silk ; done
	if [ ! -d $(GISBASE)/docs ] ; then $(MKDIR) $(GISBASE)/docs ; fi
	if [ ! -d $(GISBASE)/docs/html ] ; then $(MKDIR) $(GISBASE)/docs/html ; fi
	if [ ! -d $(GISBASE)/docs/html/icons ] ; then $(MKDIR) $(GISBASE)/docs/html/icons ; fi
	if [ ! -d $(GISBASE)/docs/html/icons/silk ] ; then $(MKDIR) $(GISBASE)/docs/html/icons/silk ; fi
	for icon in *.gif ; do $(INSTALL_DATA) $$icon $(GISBASE)/docs/html/icons ; done
	for icon in silk/*.png ; do $(INSTALL_DATA) $$icon $(GISBASE)/docs/html/icons/silk ; done
