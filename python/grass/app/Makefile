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

default: $(PYFILES) $(PYCFILES) $(DSTDIR)/resource_paths.py

$(DSTDIR)/resource_paths.py: resource_paths.py.in
	rm -f $@
	sed \
	-e 's#@GRASS_PREFIX@#$(RUN_GISBASE)#' \
	-e 's#@GISBASE@##' \
	-e 's#@GRASS_SHAREDIR@#share#' \
	-e 's#@GRASS_LOCALEDIR@#locale#' \
	-e 's#@GRASS_PYDIR@#etc/python#' \
	-e 's#@GRASS_GUIWXDIR@#gui/wxpython#' \
	-e 's#@GRASS_GUISCRIPTDIR@#gui/scripts#' \
	-e 's#@GRASS_GUIRESDIR@#gui#' \
	-e 's#@GRASS_FONTSDIR@#fonts#' \
	-e 's#@GRASS_ETCDIR@#etc#' \
	$< > $@
	chmod +x $@

$(DSTDIR):
	$(MKDIR) $@

$(DSTDIR)/%: % | $(DSTDIR)
	$(INSTALL_DATA) $< $@
