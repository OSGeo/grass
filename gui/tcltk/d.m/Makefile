MODULE_TOPDIR = ../../..

PGM=d.m
PGM2=d.m.tcl

include $(MODULE_TOPDIR)/include/Make/Script.make

PROG1 =  $(SCRIPTDIR)/$(PGM)
PROG2 =  $(ETC)/dm/$(PGM2)

default: $(PROG1) $(PROG2) script

TCL = cmd.tcl grassabout.tcl group.tcl labels.tcl menu.tcl print.tcl raster.tcl tksys.tcl tool1.tcl tool2.tcl tree.tcl vector.tcl  barscale.tcl chart.tcl dtext.tcl frames.tcl fttext.tcl gridline.tcl legend.tcl rgbhis.tcl thematic.tcl

$(PROG2): *.gif *.tcl 
	if [ ! -d $(ETC)/dm ]; then $(MKDIR) $(ETC)/dm; fi
	$(INSTALL_DATA) *.gif $(ETC)/dm/
	for tcl in $(TCL); do \
	    $(INSTALL) $$tcl $(ETC)/dm/ ; \
	done
	$(INSTALL) $(PGM2) $(PROG2)
	$(MKDIR) $(GISBASE)/etc/dm/script
	$(INSTALL) script/*.* $(GISBASE)/etc/dm/script
