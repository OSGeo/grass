MODULE_TOPDIR = ../..

SUBDIRS = docs animation datacatalog mapswipe gmodeler rlisetup psmap dbmgr vdigit rdigit iclass gcp timeline tplot photo2image image2target
EXTRA_CLEAN_FILES = menustrings.py build_ext.pyc xml/menudata.xml xml/module_tree_menudata.xml */*.pyc

include $(MODULE_TOPDIR)/include/Make/Dir.make
include $(MODULE_TOPDIR)/include/Make/Python.make

DSTDIR = $(GUIDIR)/wxpython

SRCFILES := $(wildcard icons/*.py scripts/*.py xml/*) \
	$(wildcard animation/*.py core/*.py datacatalog/*.py dbmgr/*.py gcp/*.py gmodeler/*.py \
	gui_core/*.py iclass/*.py lmgr/*.py location_wizard/*.py mapwin/*.py mapdisp/*.py \
	mapswipe/*.py modules/*.py nviz/*.py psmap/*.py rdigit/*.py \
	rlisetup/*.py startup/*.py timeline/*.py vdigit/*.py \
	vnet/*.py web_services/*.py wxplot/*.py iscatt/*.py tplot/*.py photo2image/*.py image2target/*.py) \
	gis_set.py gis_set_error.py wxgui.py README

DSTFILES := $(patsubst %,$(DSTDIR)/%,$(SRCFILES)) \
	$(patsubst %.py,$(DSTDIR)/%.pyc,$(filter %.py,$(SRCFILES)))

PYDSTDIRS := $(patsubst %,$(DSTDIR)/%,animation core datacatalog dbmgr gcp gmodeler \
	gui_core iclass lmgr location_wizard mapwin mapdisp modules nviz psmap \
	mapswipe vdigit wxplot web_services rdigit rlisetup startup \
	vnet timeline iscatt tplot photo2image image2target)


DSTDIRS := $(patsubst %,$(DSTDIR)/%,icons scripts xml)

default: $(DSTFILES)
ifndef CROSS_COMPILING
	-$(MAKE) $(DSTDIR)/xml/module_items.xml
	-$(MAKE) xml/menudata.xml
	-$(MAKE) xml/module_tree_menudata.xml
	-$(MAKE) menustrings.py
endif
	$(MAKE) parsubdirs


$(DSTDIR)/%: % | $(PYDSTDIRS) $(DSTDIRS)
	$(INSTALL_DATA) $< $@

xml/menudata.xml: core/toolboxes.py
	$(call run_grass,$(PYTHON) $< > $@)
	$(call run_grass,$(PYTHON) $< "validate" $@)

xml/module_tree_menudata.xml: core/toolboxes.py
	$(call run_grass,$(PYTHON) $< "module_tree" > $@)
	$(call run_grass,$(PYTHON) $< "validate" $@)

menustrings.py: core/menutree.py $(DSTDIR)/xml/menudata.xml $(DSTDIR)/xml/module_tree_menudata.xml $(DSTDIR)/xml/menudata_modeler.xml $(DSTDIR)/xml/menudata_psmap.xml
	@echo "# This is a generated file.\n" > $@
	$(call run_grass,$(PYTHON) $< "manager" >> $@)
	$(call run_grass,$(PYTHON) $< "module_tree" >> $@)
	$(call run_grass,$(PYTHON) $< "modeler" >> $@)
	$(call run_grass,$(PYTHON) $< "psmap" >> $@)

$(DSTDIR)/xml/module_items.xml: tools/build_modules_xml.py
	@echo "Generating interface description for all modules..."
	$(call run_grass,$(PYTHON) $< > $@)

$(PYDSTDIRS): %: | $(DSTDIR)
	$(MKDIR) $@

$(DSTDIRS): %: | $(DSTDIR)
	$(MKDIR) $@

$(DSTDIR):
	$(MKDIR) $@
