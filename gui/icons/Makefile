MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Dir.make

SRCICO := $(wildcard *.ico *.png)
SRCICONS := $(wildcard grass/*.png)
ETCICONS := $(patsubst %,$(ETC)/gui/icons/%,$(SRCICONS) $(SRCICO))
HTMLICONS := $(patsubst %,$(HTMLDIR)/icons/%,$(SRCICONS))

ETCDIRS = \
	$(ETC)/gui \
	$(ETC)/gui/icons \
	$(ETC)/gui/icons/grass

HTMLDIRS = \
	$(HTMLDIR)/icons \
	$(HTMLDIR)/icons/grass

default: $(ICONS) $(ETCICONS) $(HTMLICONS)

$(ETC)/gui/icons/%: % | $(ETCDIRS)
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/icons/%: % | $(HTMLDIRS)
	$(INSTALL_DATA) $< $@

$(ETCDIRS) $(HTMLDIRS): %:
	$(MKDIR) $@
