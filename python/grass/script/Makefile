MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Python.make

DSTDIR = $(ETC)/python/grass/script

MODULES = core db raster raster3d vector array setup task utils

PYFILES := $(patsubst %,$(DSTDIR)/%.py,$(MODULES) __init__)
PYCFILES := $(patsubst %,$(DSTDIR)/%.pyc,$(MODULES) __init__)

default: $(PYFILES) $(PYCFILES)

$(DSTDIR):
	$(MKDIR) $@

$(DSTDIR)/%: % | $(DSTDIR)
	$(INSTALL_DATA) $< $@

EXTRA_CLEAN_FILES = setup.py.tmp

$(DSTDIR)/setup.py: setup.py.tmp | $(DSTDIR)
	$(INSTALL_DATA) $< $@

setup.py.tmp: setup.py
	sed \
	-e 's#@LD_LIBRARY_PATH_VAR@#$(LD_LIBRARY_PATH_VAR)#' \
	$< > $@
