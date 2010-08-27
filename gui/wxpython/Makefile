MODULE_TOPDIR = ../..

SUBDIRS = docs vdigit
EXTRA_CLEAN_FILES = menustrings.py build_ext.pyc
CLEAN_SUBDIRS = vdigit

include $(MODULE_TOPDIR)/include/Make/Dir.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make
include $(MODULE_TOPDIR)/include/Make/Python.make

ETCDIR = $(ETC)/gui/wxpython

SRCFILES := $(wildcard scripts/* compat/* gui_modules/* icons/*.* icons/silk/* images/* xml/*) gis_set.py gis_set_error.py wxgui.py README
DSTFILES := $(patsubst %,$(ETCDIR)/%,$(SRCFILES)) $(patsubst %.py,$(ETCDIR)/%.pyc,$(filter %.py,$(SRCFILES)))

DSTDIRS := $(patsubst %,$(ETCDIR)/%,compat gui_modules icons icons/silk images scripts vdigit xml)

default: $(DSTFILES) menustrings.py
	$(MAKE) parsubdirs

$(ETCDIR)/%: % | $(DSTDIRS)
	$(INSTALL_DATA) $< $@

menustrings.py: gui_modules/menudata.py $(ETCDIR)/xml/menudata.xml $(ETCDIR)/xml/menudata_modeler.xml 
	$(call run_grass,$(PYTHON) $< > $@)
	$(call run_grass,$(PYTHON) $< "modeler" >> $@)

$(DSTDIRS): %: | $(ETCDIR)
	$(MKDIR) $@

$(ETCDIR):
	$(MKDIR) $@

#doxygen:
DOXNAME=wxpython
