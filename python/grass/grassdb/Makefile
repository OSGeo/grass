MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Python.make

DSTDIR = $(ETC)/python/grass/grassdb

MODULES = checks create data manage

PYFILES := $(patsubst %,$(DSTDIR)/%.py,$(MODULES) __init__)
PYCFILES := $(patsubst %,$(DSTDIR)/%.pyc,$(MODULES) __init__)

default: $(PYFILES) $(PYCFILES)

$(DSTDIR):
	$(MKDIR) $@

$(DSTDIR)/%: % | $(DSTDIR)
	$(INSTALL_DATA) $< $@
