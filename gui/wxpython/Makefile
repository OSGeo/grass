MODULE_TOPDIR = ../..

SUBDIRS = docs animation mapswipe gmodeler rlisetup psmap dbmgr vdigit iclass
EXTRA_CLEAN_FILES = menustrings.py build_ext.pyc xml/menudata.xml xml/module_tree_menudata.xml

include $(MODULE_TOPDIR)/include/Make/Dir.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make
include $(MODULE_TOPDIR)/include/Make/Python.make

ETCDIR = $(ETC)/gui/wxpython

SRCFILES := $(wildcard icons/*.py scripts/*.py xml/*) \
	$(wildcard animation/* core/*.py dbmgr/* gcp/*.py gmodeler/* \
	gui_core/*.py iclass/* lmgr/*.py location_wizard/*.py mapdisp/*.py \
	mapswipe/* modules/*.py nviz/*.py psmap/* rlisetup/* vdigit/* \
	vnet/*.py web_services/*.py wxplot/*.py) \
	gis_set.py gis_set_error.py wxgui.py README

DSTFILES := $(patsubst %,$(ETCDIR)/%,$(SRCFILES)) \
	$(patsubst %.py,$(ETCDIR)/%.pyc,$(filter %.py,$(SRCFILES)))

PYDSTDIRS := $(patsubst %,$(ETCDIR)/%,animation core dbmgr gcp gmodeler \
	gui_core iclass lmgr location_wizard mapdisp modules nviz psmap \
	mapswipe vdigit wxplot web_services rlisetup vnet)

DSTDIRS := $(patsubst %,$(ETCDIR)/%,icons scripts xml)

default: $(DSTFILES)
	-$(MAKE) $(ETCDIR)/xml/module_items.xml
	-$(MAKE) xml/menudata.xml
	-$(MAKE) xml/module_tree_menudata.xml
	-$(MAKE) menustrings.py
	$(MAKE) parsubdirs


$(ETCDIR)/%: % | $(PYDSTDIRS) $(DSTDIRS)
	$(INSTALL_DATA) $< $@

xml/menudata.xml: core/toolboxes.py
	$(call run_grass,$(PYTHON) $< > $@)

xml/module_tree_menudata.xml: core/toolboxes.py
	$(call run_grass,$(PYTHON) $< dummy_parameter > $@)

menustrings.py: core/menutree.py $(ETCDIR)/xml/menudata.xml $(ETCDIR)/xml/module_tree_menudata.xml $(ETCDIR)/xml/menudata_modeler.xml $(ETCDIR)/xml/menudata_psmap.xml
	@echo "# This is a generated file.\n" > $@
	$(call run_grass,$(PYTHON) $< >> $@)
	$(call run_grass,$(PYTHON) $< "modeler" >> $@)
	$(call run_grass,$(PYTHON) $< "psmap" >> $@)

$(ETCDIR)/xml/module_items.xml: tools/build_modules_xml.py
	@echo "Generating interface description for all modules..."
	$(call run_grass,$(PYTHON) $< > $@)
	
$(PYDSTDIRS): %: | $(ETCDIR)
	$(MKDIR) $@

$(DSTDIRS): %: | $(ETCDIR)
	$(MKDIR) $@

$(ETCDIR):
	$(MKDIR) $@

#doxygen:
DOXNAME = wxpython
