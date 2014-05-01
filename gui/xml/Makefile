MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Dir.make

default: $(GUIDIR)/grass-interface.dtd 

$(GUIDIR)/grass-interface.dtd : grass-interface.dtd | $(GUIDIR) 
	$(INSTALL_DATA) $< $@

$(GUIDIR): %:
	$(MKDIR) $@
