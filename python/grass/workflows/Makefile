MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Python.make

DSTDIR = $(ETC)/python/grass/workflows
TEMPLATE_FILES = template_notebooks/welcome.ipynb template_notebooks/new.ipynb
MODULES = server directory environment

PYFILES = $(patsubst %,$(DSTDIR)/%.py,$(MODULES) __init__)
PYCFILES = $(patsubst %,$(DSTDIR)/%.pyc,$(MODULES) __init__)
TEMPLATE_DST = $(patsubst %,$(DSTDIR)/%,$(TEMPLATE_FILES))

default: $(PYFILES) $(PYCFILES) $(TEMPLATE_DST)

$(DSTDIR):
	$(MKDIR) $@

$(DSTDIR)/%: % | $(DSTDIR)
	$(INSTALL_DATA) $< $@
