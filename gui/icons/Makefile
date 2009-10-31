MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Dir.make

SRCICO := $(wildcard *.ico *.png)
SRCICONS := $(wildcard grass/*.gif grass/vdigit/*.gif grass/vdigit/*.xbm silk/*.png grass2/*.png)
ETCICONS := $(patsubst %,$(ETC)/gui/icons/%,$(SRCICONS) $(SRCICO))
HTMLICONS := $(patsubst %,$(HTMLDIR)/icons/%,$(SRCICONS))

ETCDIRS = \
	$(ETC)/gui \
	$(ETC)/gui/icons \
	$(ETC)/gui/icons/grass \
	$(ETC)/gui/icons/grass/vdigit \
	$(ETC)/gui/icons/silk \
	$(ETC)/gui/icons/grass2

HTMLDIRS = \
	$(HTMLDIR)/icons \
	$(HTMLDIR)/icons/grass \
	$(HTMLDIR)/icons/grass/vdigit \
	$(HTMLDIR)/icons/silk \
	$(HTMLDIR)/icons/grass2

default: $(ICONS) $(ETCICONS) $(HTMLICONS)

$(ETC)/gui/icons/%: % | $(ETCDIRS)
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/icons/%: % | $(HTMLDIRS)
	$(INSTALL_DATA) $< $@

$(ETCDIRS) $(HTMLDIRS): %:
	$(MKDIR) $@
