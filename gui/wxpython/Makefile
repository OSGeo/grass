MODULE_TOPDIR = ../..

SUBDIRS = docs scripts vdigit nviz
EXTRA_CLEAN_FILES = menustrings.py
CLEAN_SUBDIRS = vdigit nviz

include $(MODULE_TOPDIR)/include/Make/Dir.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make
include $(MODULE_TOPDIR)/include/Make/Python.make

ETCDIR = $(ETC)/wxpython

SRCFILES := $(wildcard scripts/p.* compat/* gui_modules/* icons/*.* icons/silk/* images/* xml/*) gis_set.py wxgui.py README
DSTFILES := $(patsubst %,$(ETCDIR)/%,$(SRCFILES)) $(patsubst %.py,$(ETCDIR)/%.pyc,$(filter %.py,$(SRCFILES)))

DSTDIRS := $(patsubst %,$(ETCDIR)/%,compat gui_modules icons icons/silk images scripts vdigit xml nviz)

default: $(DSTFILES) menustrings.py
	$(MAKE) parsubdirs

$(ETCDIR)/%: % | $(DSTDIRS)
	$(INSTALL_DATA) $< $@

menustrings.py: gui_modules/menudata.py xml/menudata.xml
	GISBASE="$(GISBASE)" \
	$(PYTHON) $< > $@

$(DSTDIRS): %: | $(ETCDIR)
	$(MKDIR) $@

$(ETCDIR):
	$(MKDIR) $@

#doxygen:
DOXNAME=wxpython
