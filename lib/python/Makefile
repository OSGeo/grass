MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

PYDIR = $(ETC)/python
GDIR = $(PYDIR)/grass
DSTDIR = $(GDIR)/script

MODULES = core db raster vector

PYFILES := $(patsubst %,$(DSTDIR)/%.py,$(MODULES) __init__)

default: $(PYFILES) $(GDIR)/__init__.py

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
