MODULE_TOPDIR = ../../../..

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Python.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

PYDIR = $(ETC)/python
GDIR = $(PYDIR)/grass
PGDIR = $(GDIR)/pygrass
DSTDIR= $(PGDIR)/messages

MODULES = 

PYFILES := $(patsubst %,$(DSTDIR)/%.py,$(MODULES) __init__)
PYCFILES := $(patsubst %,$(DSTDIR)/%.pyc,$(MODULES) __init__)

default: $(PYFILES) $(PYCFILES) $(GDIR)/__init__.py $(GDIR)/__init__.pyc

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
