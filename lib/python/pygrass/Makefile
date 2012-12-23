MODULE_TOPDIR=../../..

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Python.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

PYDIR = $(ETC)/python
GDIR = $(PYDIR)/grass
DSTDIR = $(GDIR)/pygrass

MODULES = errors functions orderdict

CLEAN_SUBDIRS = modules raster vector gis tests

PYFILES := $(patsubst %,$(DSTDIR)/%.py,$(MODULES) __init__)
PYCFILES := $(patsubst %,$(DSTDIR)/%.pyc,$(MODULES) __init__)

default: $(PYFILES) $(PYCFILES) $(GDIR)/__init__.py $(GDIR)/__init__.pyc
	-$(MAKE) -C modules || echo $(CURDIR)/modules >> $(ERRORLOG)
	-$(MAKE) -C raster || echo $(CURDIR)/raster >> $(ERRORLOG)
	-$(MAKE) -C vector || echo $(CURDIR)/vector >> $(ERRORLOG)
	-$(MAKE) -C gis || echo $(CURDIR)/gis >> $(ERRORLOG)
	-$(MAKE) -C tests || echo $(CURDIR)/tests >> $(ERRORLOG)

$(PYDIR):
	$(MKDIR) $@

$(GDIR): | $(PYDIR)
	$(MKDIR) $@

$(DSTDIR): | $(GDIR)
	$(MKDIR) $@

$(DSTDIR)/%: % | $(DSTDIR)
	$(INSTALL_DATA) $< $@

#doxygen:
DOXNAME = pythonpygrass
