MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Python.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

PYDIR = $(ETC)/python
GDIR = $(PYDIR)/grass
DSTDIR = $(GDIR)/script

MODULES = core db raster raster3d vector array setup task 

PYFILES := $(patsubst %,$(DSTDIR)/%.py,$(MODULES) __init__)
PYCFILES := $(patsubst %,$(DSTDIR)/%.pyc,$(MODULES) __init__)

CLEAN_SUBDIRS = ctypes temporal
EXTRA_CLEAN_FILES = setup.py

default: $(PYFILES) $(PYCFILES) $(GDIR)/__init__.py $(GDIR)/__init__.pyc
	-$(MAKE) -C ctypes || echo $(CURDIR)/ctypes >> $(ERRORLOG)
	-$(MAKE) -C temporal || echo $(CURDIR)/temporal >> $(ERRORLOG)

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

$(DSTDIR)/setup.py: $(OBJDIR)/setup.py | $(DSTDIR)
	$(INSTALL_DATA) $< $@

$(OBJDIR)/setup.py: setup.py.sed | $(OBJDIR)
	sed \
	-e 's#@LD_LIBRARY_PATH_VAR@#$(LD_LIBRARY_PATH_VAR)#' \
	$< > $@

#doxygen:
DOXNAME = python
