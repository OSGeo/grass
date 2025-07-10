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

default: $(PYFILES) $(PYCFILES) $(DSTDIR)/resource_paths.py $(ARCH_DISTDIR)/resource_paths.py

$(DSTDIR)/resource_paths.py: resource_paths.py
	rm -f $@
	sed \
	-e 's#@CONFIG_PROJSHARE@#$(PROJSHARE)#' \
	-e 's#@GISBASE_INSTALL_PATH@##' \
	-e 's#@GRASS_PREFIX@#$(RUN_GISBASE)#' \
	-e 's#@GRASS_VERSION_GIT@#$(GRASS_VERSION_GIT)#' \
	-e 's#@GRASS_VERSION_MAJOR@#$(GRASS_VERSION_MAJOR)#' \
	-e 's#@GRASS_VERSION_MINOR@#$(GRASS_VERSION_MINOR)#' \
	-e 's#@GRASS_VERSION_NUMBER@#$(GRASS_VERSION_NUMBER)#' \
	-e 's#@LD_LIBRARY_PATH_VAR@#$(LD_LIBRARY_PATH_VAR)#' \
	-e 's#@START_UP@#$(GRASS_NAME)#' \
	-e 's#@GRASS_COLORSDIR@#etc/colors#' \
	-e 's#@GRASS_DOCDIR@#docs/html#' \
	-e 's#@GRASS_ETCBINDIR@#etc#' \
	-e 's#@GRASS_ETCDIR@#etc#' \
	-e 's#@GRASS_FONTSDIR@#fonts#' \
	-e 's#@GRASS_GRAPHICSDIR@#docs/html#' \
	-e 's#@GRASS_GUIRESDIR@#gui#' \
	-e 's#@GRASS_GUISCRIPTDIR@#gui/scripts#' \
	-e 's#@GRASS_GUIWXDIR@#gui/wxpython#' \
	-e 's#@GRASS_LOCALEDIR@#locale#' \
	-e 's#@GRASS_MISCDIR@##' \
	-e 's#@GRASS_MKDOCSDIR@#docs/mkdocs#' \
	$< > $@

$(ARCH_DISTDIR)/resource_paths.py:
	cp resource_paths.py $@

$(DSTDIR):
	$(MKDIR) $@

$(DSTDIR)/%: % | $(DSTDIR)
	$(INSTALL_DATA) $< $@
