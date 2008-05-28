MODULE_TOPDIR = ../../..

EXTRA_LIBS=$(GISLIB) 

LIB_NAME = grass_rli

DEPENDENCIES = $(GISDEP) 

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: lib
	$(INSTALL_DATA) description.html $(GISBASE)/docs/html/r.li.daemon.html
