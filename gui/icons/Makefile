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
	$(MKDIR) $(ARCH_DISTDIR)/share/appdata
	$(INSTALL_DATA) grass.appdata.xml $(ARCH_DISTDIR)/share/appdata

$(GUIDIR)/icons/%: % | $(DSTDIRS)
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/icons/%: grass/% | $(HTMLDIRS)
	$(INSTALL_DATA) $< $@

$(DSTDIRS) $(HTMLDIRS): %:
	$(MKDIR) $@
