MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Dir.make

SRCICO := $(wildcard *.ico *.png)
SRCICONS := $(wildcard grass/*.gif grass/vdigit/*.gif grass/vdigit/*.xbm silk/*.png grass2/*.png)
ETCICONS := $(patsubst %,$(ETC)/gui/icons/%,$(SRCICONS) $(SRCICO))
HTMLICONS := $(patsubst %,$(HTMLDIR)/icons/%,$(SRCICONS))

default: $(ICONS)
	if [ ! -d $(ETC)/gui ] ; then $(MKDIR) $(ETC)/gui ; fi
	if [ ! -d $(ETC)/gui/icons ] ; then $(MKDIR) $(ETC)/gui/icons ; fi
	if [ ! -d $(ETC)/gui/icons/grass ] ; then $(MKDIR) $(ETC)/gui/icons/grass ; fi
	if [ ! -d $(ETC)/gui/icons/grass/vdigit ] ; then $(MKDIR) $(ETC)/gui/icons/grass/vdigit ; fi
	if [ ! -d $(ETC)/gui/icons/silk ] ; then $(MKDIR) $(ETC)/gui/icons/silk ; fi
	if [ ! -d $(ETC)/gui/icons/grass2 ] ; then $(MKDIR) $(ETC)/gui/icons/grass2 ; fi
	if [ ! -d $(DOCSDIR) ] ; then $(MKDIR) $(DOCSDIR) ; fi
	if [ ! -d $(HTMLDIR) ] ; then $(MKDIR) $(HTMLDIR) ; fi
	if [ ! -d $(HTMLDIR)/icons ] ; then $(MKDIR) $(HTMLDIR)/icons ; fi
	if [ ! -d $(HTMLDIR)/icons/grass ] ; then $(MKDIR) $(HTMLDIR)/icons/grass ; fi
	if [ ! -d $(HTMLDIR)/icons/grass/vdigit ] ; then $(MKDIR) $(HTMLDIR)/icons/grass/vdigit ; fi
	if [ ! -d $(HTMLDIR)/icons/silk ] ; then $(MKDIR) $(HTMLDIR)/icons/silk ; fi
	if [ ! -d $(HTMLDIR)/icons/grass2 ] ; then $(MKDIR) $(HTMLDIR)/icons/grass2 ; fi
	$(MAKE) $(ETCICONS)
	$(MAKE) $(HTMLICONS)

$(ETC)/gui/icons/%: %
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/icons/%: %
	$(INSTALL_DATA) $< $@
