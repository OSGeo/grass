MODULE_TOPDIR = ../..

SUBDIRS = docs scripts vdigit nviz

include $(MODULE_TOPDIR)/include/Make/Dir.make

ETCDIR = $(ETC)/wxpython

SRCFILES := $(wildcard scripts/* compat/* gui_modules/* icons/*.* icons/silk/* images/* xml/*) gis_set.py wxgui.py README scripts/wxgui
DSTFILES := $(patsubst %,$(ETCDIR)/%,$(SRCFILES))

default: install_scripts
	$(MAKE) parsubdirs

install_scripts:
	-$(MKDIR) $(ETCDIR) $(ETCDIR)/compat $(ETCDIR)/gui_modules \
		$(ETCDIR)/icons $(ETCDIR)/icons/silk $(ETCDIR)/images \
		$(ETCDIR)/scripts $(ETCDIR)/vdigit $(ETCDIR)/xml $(ETCDIR)/nviz
	$(MAKE) $(DSTFILES)

$(ETCDIR)/scripts/wxgui: wxgui
	$(INSTALL) $< $@

$(ETCDIR)/scripts/%: scripts/%
	$(INSTALL) $< $@

$(ETCDIR)/%: %
	$(INSTALL_DATA) $< $@
