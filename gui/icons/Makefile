MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Dir.make

default: $(ICONS)
	if [ ! -d $(ETC)/gui ] ; then $(MKDIR) $(ETC)/gui ; fi
	if [ ! -d $(ETC)/gui/icons ] ; then $(MKDIR) $(ETC)/gui/icons ; fi
	if [ ! -d $(ETC)/gui/icons/grass ] ; then $(MKDIR) $(ETC)/gui/icons/grass ; fi
	if [ ! -d $(ETC)/gui/icons/grass/vdigit ] ; then $(MKDIR) $(ETC)/gui/icons/grass/vdigit ; fi
	if [ ! -d $(ETC)/gui/icons/silk ] ; then $(MKDIR) $(ETC)/gui/icons/silk ; fi
	for icon in grass/*.gif ; do $(INSTALL_DATA) $$icon $(ETC)/gui/icons/grass ; done
	for icon in grass/vdigit/*.gif ; do $(INSTALL_DATA) $$icon $(ETC)/gui/icons/grass/vdigit ; done
	for icon in grass/vdigit/*.xbm ; do $(INSTALL_DATA) $$icon $(ETC)/gui/icons/grass/vdigit ; done
	for icon in silk/*.png ; do $(INSTALL_DATA) $$icon $(ETC)/gui/icons/silk ; done
	for icon in *.ico ; do $(INSTALL_DATA) $$icon $(ETC)/gui/icons ; done
	if [ ! -d $(GISBASE)/docs ] ; then $(MKDIR) $(GISBASE)/docs ; fi
	if [ ! -d $(GISBASE)/docs/html ] ; then $(MKDIR) $(GISBASE)/docs/html ; fi
	if [ ! -d $(GISBASE)/docs/html/icons ] ; then $(MKDIR) $(GISBASE)/docs/html/icons ; fi
	if [ ! -d $(GISBASE)/docs/html/icons/grass ] ; then $(MKDIR) $(GISBASE)/docs/html/icons/grass ; fi
	if [ ! -d $(GISBASE)/docs/html/icons/grass/vdigit ] ; then $(MKDIR) $(GISBASE)/docs/html/icons/grass/vdigit ; fi
	if [ ! -d $(GISBASE)/docs/html/icons/silk ] ; then $(MKDIR) $(GISBASE)/docs/html/icons/silk ; fi
	for icon in grass/*.gif ; do $(INSTALL_DATA) $$icon $(GISBASE)/docs/html/icons/grass ; done
	for icon in grass/vdigit/*.gif ; do $(INSTALL_DATA) $$icon $(GISBASE)/docs/html/icons/grass/vdigit ; done
	for icon in grass/vdigit/*.xbm ; do $(INSTALL_DATA) $$icon $(GISBASE)/docs/html/icons/grass/vdigit ; done
	for icon in silk/*.png ; do $(INSTALL_DATA) $$icon $(GISBASE)/docs/html/icons/silk ; done

