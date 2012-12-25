MODULE_TOPDIR = ../..

SUBDIRS = docs animation mapswipe gmodeler rlisetup psmap dbmgr vdigit iclass
EXTRA_CLEAN_FILES = menustrings.py build_ext.pyc

include $(MODULE_TOPDIR)/include/Make/Dir.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make
include $(MODULE_TOPDIR)/include/Make/Python.make

ETCDIR = $(ETC)/gui/wxpython

SRCFILES := $(wildcard icons/*.* scripts/* xml/*) \
	$(wildcard animation/* core/* dbmgr/* gcp/* gmodeler/* gui_core/* iclass/* lmgr/* location_wizard/* \
	mapdisp/* modules/* nviz/* psmap/* mapswipe/* vdigit/* wxplot/* ogc_services/* rlisetup/*) \
	gis_set.py gis_set_error.py wxgui.py README
DSTFILES := $(patsubst %,$(ETCDIR)/%,$(SRCFILES)) $(patsubst %.py,$(ETCDIR)/%.pyc,$(filter %.py,$(SRCFILES)))

PYDSTDIRS := $(patsubst %,$(ETCDIR)/%,animation core dbmgr gcp gmodeler gui_core iclass lmgr location_wizard \
	mapdisp modules nviz psmap mapswipe vdigit wxplot ogc_services rlisetup)
DSTDIRS := $(patsubst %,$(ETCDIR)/%,icons scripts xml)

default: $(DSTFILES)
	-$(MAKE) menustrings.py
	$(MAKE) parsubdirs

$(ETCDIR)/%: % | $(PYDSTDIRS) $(DSTDIRS)
	$(INSTALL_DATA) $< $@

menustrings.py: core/menudata.py $(ETCDIR)/xml/menudata.xml $(ETCDIR)/xml/menudata_modeler.xml 
	$(call run_grass,$(PYTHON) $< > $@)
	$(call run_grass,$(PYTHON) $< "modeler" >> $@)

$(PYDSTDIRS): %: | $(ETCDIR)
	$(MKDIR) $@
	$(call run_grass,$(PYTHON) create__init__.py $@)

$(DSTDIRS): %: | $(ETCDIR)
	$(MKDIR) $@

$(ETCDIR):
	$(MKDIR) $@

#doxygen:
DOXNAME=wxpython
