MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Dir.make

SRCICO := $(wildcard *.ico *.png)
SRCICONS := $(wildcard grass/*.png)
SRCFLAGS := $(wildcard flags/*.png)
ETCICONS := $(patsubst %,$(GUIDIR)/icons/%,$(SRCICONS) $(SRCICO) $(SRCFLAGS))
HTMLICONS := $(patsubst grass/%,$(HTMLDIR)/icons/%,$(SRCICONS))
MDDIRICONS := $(patsubst grass/%,$(MDDIR)/source/icons/%,$(SRCICONS))

DSTDIRS = \
	$(GUIDIR) \
	$(GUIDIR)/icons \
	$(GUIDIR)/icons/grass \
	$(GUIDIR)/icons/flags

HTMLDIRS = \
	$(HTMLDIR)/icons

MDDIRS = \
	$(MDDIR)/source/icons

default: $(ETCICONS) $(HTMLICONS) $(MDDIRICONS)
	$(MKDIR) $(ARCH_DISTDIR)/share/applications
	$(INSTALL_DATA) grass.desktop $(ARCH_DISTDIR)/share/applications
	$(MKDIR) $(ARCH_DISTDIR)/share/icons/hicolor/8x8/apps
	$(INSTALL_DATA) grass-8x8.png $(ARCH_DISTDIR)/share/icons/hicolor/8x8/apps/grass.png
	$(MKDIR) $(ARCH_DISTDIR)/share/icons/hicolor/16x16/apps
	$(INSTALL_DATA) grass-16x16.png $(ARCH_DISTDIR)/share/icons/hicolor/16x16/apps/grass.png
	$(MKDIR) $(ARCH_DISTDIR)/share/icons/hicolor/22x22/apps
	$(INSTALL_DATA) grass-22x22.png $(ARCH_DISTDIR)/share/icons/hicolor/22x22/apps/grass.png
	$(MKDIR) $(ARCH_DISTDIR)/share/icons/hicolor/24x24/apps
	$(INSTALL_DATA) grass-24x24.png $(ARCH_DISTDIR)/share/icons/hicolor/24x24/apps/grass.png
	$(MKDIR) $(ARCH_DISTDIR)/share/icons/hicolor/32x32/apps
	$(INSTALL_DATA) grass-32x32.png $(ARCH_DISTDIR)/share/icons/hicolor/32x32/apps/grass.png
	$(MKDIR) $(ARCH_DISTDIR)/share/icons/hicolor/36x36/apps
	$(INSTALL_DATA) grass-36x36.png $(ARCH_DISTDIR)/share/icons/hicolor/36x36/apps/grass.png
	$(MKDIR) $(ARCH_DISTDIR)/share/icons/hicolor/40x40/apps
	$(INSTALL_DATA) grass-40x40.png $(ARCH_DISTDIR)/share/icons/hicolor/40x40/apps/grass.png
	$(MKDIR) $(ARCH_DISTDIR)/share/icons/hicolor/42x42/apps
	$(INSTALL_DATA) grass-42x42.png $(ARCH_DISTDIR)/share/icons/hicolor/42x42/apps/grass.png
	$(MKDIR) $(ARCH_DISTDIR)/share/icons/hicolor/48x48/apps
	$(INSTALL_DATA) grass-48x48.png $(ARCH_DISTDIR)/share/icons/hicolor/48x48/apps/grass.png
	$(MKDIR) $(ARCH_DISTDIR)/share/icons/hicolor/64x64/apps
	$(INSTALL_DATA) grass-64x64.png $(ARCH_DISTDIR)/share/icons/hicolor/64x64/apps/grass.png
	$(MKDIR) $(ARCH_DISTDIR)/share/icons/hicolor/72x72/apps
	$(INSTALL_DATA) grass-72x72.png $(ARCH_DISTDIR)/share/icons/hicolor/72x72/apps/grass.png
	$(MKDIR) $(ARCH_DISTDIR)/share/icons/hicolor/80x80/apps
	$(INSTALL_DATA) grass-80x80.png $(ARCH_DISTDIR)/share/icons/hicolor/80x80/apps/grass.png
	$(MKDIR) $(ARCH_DISTDIR)/share/icons/hicolor/96x96/apps
	$(INSTALL_DATA) grass-96x96.png $(ARCH_DISTDIR)/share/icons/hicolor/96x96/apps/grass.png
	$(MKDIR) $(ARCH_DISTDIR)/share/icons/hicolor/128x128/apps
	$(INSTALL_DATA) grass-128x128.png $(ARCH_DISTDIR)/share/icons/hicolor/128x128/apps/grass.png
	$(MKDIR) $(ARCH_DISTDIR)/share/icons/hicolor/192x192/apps
	$(INSTALL_DATA) grass-192x192.png $(ARCH_DISTDIR)/share/icons/hicolor/192x192/apps/grass.png
	$(MKDIR) $(ARCH_DISTDIR)/share/icons/hicolor/scalable/apps
	$(INSTALL_DATA) grass.svg $(ARCH_DISTDIR)/share/icons/hicolor/scalable/apps/grass.svg
	$(MKDIR) $(ARCH_DISTDIR)/share/metainfo
	$(INSTALL_DATA) grass.appdata.xml $(ARCH_DISTDIR)/share/metainfo/org.osgeo.grass.appdata.xml
### disable special 64bit icons (using different icons can be confusing for the user)
# ifeq ($(findstring x86_64,$(ARCH)),x86_64)
# 	$(INSTALL_DATA) x86_64/grass.ico $(GUIDIR)/icons
# 	$(INSTALL_DATA) x86_64/grass_osgeo.ico $(GUIDIR)/icons
# endif

$(GUIDIR)/icons/%: % | $(DSTDIRS)
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/icons/%: grass/% | $(HTMLDIRS)
	$(INSTALL_DATA) $< $@

$(MDDIR)/source/icons/%: grass/% | $(MDDIRS)
	$(INSTALL_DATA) $< $@

$(DSTDIRS) $(HTMLDIRS) $(MDDIRS): %:
	$(MKDIR) $@
