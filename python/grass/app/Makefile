MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Python.make

DSTDIR = $(ETC)/python/grass/app

MODULES = \
	__main__ \
	cli \
	data \
	runtime

PYFILES := $(patsubst %,$(DSTDIR)/%.py,$(MODULES) __init__)
PYCFILES := $(patsubst %,$(DSTDIR)/%.pyc,$(MODULES) __init__)

PYFILES := $(filter-out resource_paths.py,$(PYFILES))
PYCFILES := $(filter-out resource_paths.pyc,$(PYCFILES))

default: $(PYFILES) $(PYCFILES) $(DSTDIR)/resource_paths.py $(ARCH_DISTDIR)/resource_paths.py

$(DSTDIR)/resource_paths.py: resource_paths.py
	rm -f $@
	sed \
	-e 's#@GRASS_PREFIX@#$(RUN_GISBASE)#' \
	-e 's#@GISBASE_INSTALL_PATH@##' \
	$< > $@

$(ARCH_DISTDIR)/resource_paths.py:
	cp resource_paths.py $@

$(DSTDIR):
	$(MKDIR) $@

$(DSTDIR)/%: % | $(DSTDIR)
	$(INSTALL_DATA) $< $@
