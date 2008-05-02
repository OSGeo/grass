MODULE_TOPDIR = ../..

SUBDIRS = docs

include $(MODULE_TOPDIR)/include/Make/Platform.make

ifneq ($(USE_WXWIDGETS),)
  ifneq ($(USE_PYTHON),)
    ifneq ($(strip $(CXX)),)
       SUBDIRS += vdigit
    endif
  endif
endif

include $(MODULE_TOPDIR)/include/Make/Dir.make

ETCDIR = $(ETC)/wxpython

default: install_scripts
	$(MAKE) parsubdirs

install_scripts:
	$(MKDIR) $(ETCDIR) $(ETCDIR)/compat $(ETCDIR)/gui_modules $(ETCDIR)/icons $(ETCDIR)/icons/silk $(ETCDIR)/images $(ETCDIR)/scripts $(ETCDIR)/vdigit $(ETCDIR)/xml
	$(INSTALL_DATA) compat/* $(ETCDIR)/compat/
	$(INSTALL_DATA) gui_modules/* $(ETCDIR)/gui_modules/
	$(INSTALL_DATA) icons/*.* $(ETCDIR)/icons/
	$(INSTALL_DATA) icons/silk/* $(ETCDIR)/icons/silk/
	$(INSTALL_DATA) images/* $(ETCDIR)/images/
	$(INSTALL_DATA) xml/* $(ETCDIR)/xml/	
	$(INSTALL_DATA) gis_set.py wxgui.py README $(ETCDIR)/
	$(INSTALL) scripts/* $(ETCDIR)/scripts/	
	$(INSTALL) wxgui $(ETCDIR)/scripts/

clean: cleansubdirs
