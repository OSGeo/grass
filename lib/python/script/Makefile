MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Python.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

DSTDIR = $(ETC)/python/grass/script

MODULES = core db raster raster3d vector array setup task 

PYFILES := $(patsubst %,$(DSTDIR)/%.py,$(MODULES) __init__)
PYCFILES := $(patsubst %,$(DSTDIR)/%.pyc,$(MODULES) __init__)

EXTRA_CLEAN_FILES = setup.py

default: $(PYFILES) $(PYCFILES)

$(DSTDIR):
	$(MKDIR) $@

$(DSTDIR)/%: % | $(DSTDIR)
	$(INSTALL_DATA) $< $@

$(DSTDIR)/setup.py: setup.py | $(DSTDIR)
	$(INSTALL_DATA) $< $@

setup.py: setup.py.sed
	sed \
	-e 's#@LD_LIBRARY_PATH_VAR@#$(LD_LIBRARY_PATH_VAR)#' \
	$< > $@
