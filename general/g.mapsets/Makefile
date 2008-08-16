
MODULE_TOPDIR = ../..

PGM = g.mapsets
GUI = $(GISBASE)/etc/g.mapsets.tcl

LIBES     = $(GISLIB)
DEPENDENCIES= $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd $(GUI)

$(GUI): g.mapsets.tcl
	if [ ! -d $(GISBASE)/etc ]; then $(MKDIR) $(GISBASE)/etc; fi
	$(INSTALL) g.mapsets.tcl $(GUI)
