MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Dir.make

ETCDIR = $(ETC)/gui/scripts

default: install_scripts

install_scripts:
	if [ ! -d $(ETC)/gui ] ; then $(MKDIR) $(ETC)/gui ; fi
	if [ ! -d $(ETCDIR) ] ; then $(MKDIR) $(ETCDIR) ; fi
	$(MAKE) $(ETCDIR)/d.rast3d

$(ETCDIR)/%: %
	$(INSTALL) $< $@

clean: cleansubdirs
