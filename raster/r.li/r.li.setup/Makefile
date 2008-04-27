TARGET= $(ETC)/r.li.setup

MODULE_TOPDIR = ../../..

PGM = r.li.setup

include $(MODULE_TOPDIR)/include/Make/Script.make

default: script
	$(MKDIR) $(TARGET)
	$(INSTALL) area_query $(TARGET)/area_query
	$(INSTALL) masked_area_selection $(TARGET)/masked_area_selection
	$(INSTALL) r.li.setup.main $(TARGET)/r.li.setup.main
	$(INSTALL) r.li.setup.procedures.tcl $(TARGET)/r.li.setup.procedures.tcl
	$(INSTALL) r.li.windows.tcl $(TARGET)/r.li.windows.tcl
	$(INSTALL) sample_area_vector.sh $(TARGET)/sample_area_vector.sh
	$(INSTALL) square_mouse_selection.sh $(TARGET)/square_mouse_selection.sh
	$(INSTALL) square_query $(TARGET)/square_query
	$(INSTALL_DATA) circle.txt $(TARGET)/circle.txt
	$(INSTALL_DATA) polygon.txt $(TARGET)/polygon.txt


