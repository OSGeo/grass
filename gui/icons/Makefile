MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Dir.make

SRCICO := $(wildcard *.ico *.png)
SRCICONS := $(wildcard grass/*.png)
SRCFLAGS := $(wildcard flags/*.png)
ETCICONS := $(patsubst %,$(ETC)/gui/icons/%,$(SRCICONS) $(SRCICO) $(SRCFLAGS))
HTMLICONS := $(patsubst grass/%,$(HTMLDIR)/icons/%,$(SRCICONS))

ETCDIRS = \
	$(ETC)/gui \
	$(ETC)/gui/icons \
	$(ETC)/gui/icons/grass \
	$(ETC)/gui/icons/flags

HTMLDIRS = \
	$(HTMLDIR)/icons

default: $(ETCICONS) $(HTMLICONS)
	$(MKDIR) $(ARCH_DISTDIR)/share/applications
	$(INSTALL_DATA) grass.desktop $(ARCH_DISTDIR)/share/applications

$(ETC)/gui/icons/%: % | $(ETCDIRS)
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/icons/%: grass/% | $(HTMLDIRS)
	$(INSTALL_DATA) $< $@

$(ETCDIRS) $(HTMLDIRS): %:
	$(MKDIR) $@
