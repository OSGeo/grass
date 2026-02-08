MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Python.make

DSTDIR = $(ETC)/python/grass/workflows
TEMPLATE_DIR_NAME = template_notebooks
TEMPLATE_DIR = $(DSTDIR)/$(TEMPLATE_DIR_NAME)
TEMPLATE_FILES = $(TEMPLATE_DIR_NAME)/welcome.ipynb $(TEMPLATE_DIR_NAME)/new.ipynb
MODULES = server directory environment utils

PYFILES := $(patsubst %,$(DSTDIR)/%.py,$(MODULES) __init__)
PYCFILES := $(patsubst %,$(DSTDIR)/%.pyc,$(MODULES) __init__)
TEMPLATE_DST := $(patsubst %,$(DSTDIR)/%,$(TEMPLATE_FILES))

default: $(PYFILES) $(PYCFILES) $(TEMPLATE_DST)

$(TEMPLATE_DIR):
	$(MKDIR) $@

$(DSTDIR)/%: % | $(TEMPLATE_DIR)
	$(INSTALL_DATA) $< $@
