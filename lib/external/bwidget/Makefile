MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make

default:
	if [ ! -d $(GISBASE)/bwidget ]; then $(MKDIR) $(GISBASE)/bwidget; fi
	if [ ! -d $(GISBASE)/bwidget/images ]; then $(MKDIR) $(GISBASE)/bwidget/images; fi
	if [ ! -d $(GISBASE)/bwidget/lang ]; then $(MKDIR) $(GISBASE)/bwidget/lang; fi
	for file in images/*.* ; do $(INSTALL_DATA) $$file $(GISBASE)/bwidget/images/ ; done
	for file in lang/*.* ; do $(INSTALL_DATA) $$file $(GISBASE)/bwidget/lang/ ; done
	for file in *.tcl *.txt README.grass ; do $(INSTALL_DATA) $$file $(GISBASE)/bwidget/ ; done

clean:
