MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Dir.make

default:
	if [ ! -d $(GISBASE)/etc/gui ] ; then $(MKDIR) $(GISBASE)/etc/gui ; fi
	if [ ! -d $(GISBASE)/etc/gui/scripts ] ; then $(MKDIR) $(GISBASE)/etc/gui/scripts ; fi
	for file in *.* ; do $(INSTALL) $$file $(GISBASE)/etc/gui/scripts/ ; done
