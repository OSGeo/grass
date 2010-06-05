MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Python.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

PYDIR = $(ETC)/python
GDIR = $(PYDIR)/grass
DSTDIR = $(GDIR)/script

MODULES = core db raster vector array

PYFILES := $(patsubst %,$(DSTDIR)/%.py,$(MODULES) __init__)
PYCFILES := $(patsubst %,$(DSTDIR)/%.pyc,$(MODULES) __init__)

CLEAN_SUBDIRS = ctypes

default: $(PYFILES) $(PYCFILES) $(GDIR)/__init__.py $(GDIR)/__init__.pyc
	-$(MAKE) -C ctypes || echo $(CURDIR)/ctypes >> $(ERRORLOG)

$(PYDIR):
	$(MKDIR) $@

$(GDIR): | $(PYDIR)
	$(MKDIR) $@

$(DSTDIR): | $(GDIR)
	$(MKDIR) $@

$(GDIR)/__init__.py: grass__init__.py | $(GDIR)
	$(INSTALL_DATA) $< $@

$(DSTDIR)/%: % | $(DSTDIR)
	$(INSTALL_DATA) $< $@

#doxygen:
DOXNAME=grasspython
