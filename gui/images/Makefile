MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Dir.make

default:
	if [ ! -d $(ETC)/gui ] ; then $(MKDIR) $(ETC)/gui ; fi
	if [ ! -d $(ETC)/gui/images ] ; then $(MKDIR) $(ETC)/gui/images ; fi
	$(INSTALL_DATA) gintro.gif $(ETC)/gui/images/gintro.gif

