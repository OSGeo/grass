MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Dir.make

SRCICO := $(wildcard *.ico *.png)
SRCICONS := $(wildcard grass/*.png)
SRCFLAGS := $(wildcard flags/*.png)
ETCICONS := $(patsubst %,$(GUIDIR)/icons/%,$(SRCICONS) $(SRCICO) $(SRCFLAGS))
HTMLICONS := $(patsubst grass/%,$(HTMLDIR)/icons/%,$(SRCICONS))

DSTDIRS = \
	$(GUIDIR) \
	$(GUIDIR)/icons \
	$(GUIDIR)/icons/grass \
	$(GUIDIR)/icons/flags

HTMLDIRS = \
	$(HTMLDIR)/icons

default: $(ETCICONS) $(HTMLICONS)
	$(MKDIR) $(ARCH_DISTDIR)/share/applications
	$(INSTALL_DATA) grass.desktop $(ARCH_DISTDIR)/share/applications
	$(MKDIR) $(ARCH_DISTDIR)/share/icons/hicolor/48x48/apps
	$(INSTALL_DATA) grass-48x48.png $(ARCH_DISTDIR)/share/icons/hicolor/48x48/apps/grass70.png
	$(MKDIR) $(ARCH_DISTDIR)/share/icons/hicolor/64x64/apps
	$(INSTALL_DATA) grass-64x64.png $(ARCH_DISTDIR)/share/icons/hicolor/64x64/apps/grass70.png
	$(MKDIR) $(ARCH_DISTDIR)/share/icons/hicolor/scalable/apps
	$(INSTALL_DATA) grass.svg $(ARCH_DISTDIR)/share/icons/hicolor/scalable/apps/grass70.svg
	$(MKDIR) $(ARCH_DISTDIR)/share/appdata
	$(INSTALL_DATA) grass.appdata.xml $(ARCH_DISTDIR)/share/appdata
ifeq ($(filter x86_64,$(ARCH)),)
	$(INSTALL_DATA) x86_64/grass.ico $(GUIDIR)/icons
	$(INSTALL_DATA) x86_64/grass_osgeo.ico $(GUIDIR)/icons
endif

$(GUIDIR)/icons/%: % | $(DSTDIRS)
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/icons/%: grass/% | $(HTMLDIRS)
	$(INSTALL_DATA) $< $@

$(DSTDIRS) $(HTMLDIRS): %:
	$(MKDIR) $@
