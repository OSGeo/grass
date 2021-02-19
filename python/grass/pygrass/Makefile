MODULE_TOPDIR=../../..

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Python.make

PYDIR = $(ETC)/python
GDIR = $(PYDIR)/grass
DSTDIR = $(GDIR)/pygrass

MODULES = errors utils orderdict

CLEAN_SUBDIRS = messages modules raster vector gis shell tests rpc

PYFILES := $(patsubst %,$(DSTDIR)/%.py,$(MODULES) __init__)
PYCFILES := $(patsubst %,$(DSTDIR)/%.pyc,$(MODULES) __init__)

default: $(PYFILES) $(PYCFILES) $(GDIR)/__init__.py $(GDIR)/__init__.pyc
	-$(MAKE) -C messages || echo $(CURDIR)/messages >> $(ERRORLOG)
	-$(MAKE) -C modules || echo $(CURDIR)/modules >> $(ERRORLOG)
	-$(MAKE) -C raster || echo $(CURDIR)/raster >> $(ERRORLOG)
	-$(MAKE) -C vector || echo $(CURDIR)/vector >> $(ERRORLOG)
	-$(MAKE) -C gis || echo $(CURDIR)/gis >> $(ERRORLOG)
	-$(MAKE) -C shell || echo $(CURDIR)/shell >> $(ERRORLOG)
	-$(MAKE) -C tests || echo $(CURDIR)/tests >> $(ERRORLOG)
	-$(MAKE) -C rpc || echo $(CURDIR)/gis >> $(ERRORLOG)

$(PYDIR):
	$(MKDIR) $@

$(GDIR): | $(PYDIR)
	$(MKDIR) $@

$(DSTDIR): | $(GDIR)
	$(MKDIR) $@

$(DSTDIR)/%: % | $(DSTDIR)
	$(INSTALL_DATA) $< $@
