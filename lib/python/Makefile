MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

PYDIR = $(ETC)/python
GDIR = $(PYDIR)/grass
DSTDIR = $(GDIR)/script

MODULES = core db raster vector

PYFILES := $(patsubst %,$(DSTDIR)/%.py,$(MODULES) __init__)

default: $(DSTDIR)
	$(MAKE) $(PYFILES) $(DSTDIR)/README.txt

$(PYDIR):
	test -d $@ || $(MKDIR) -p $@

$(GDIR): $(PYDIR)
	test -d $@ || $(MKDIR) -p $@

$(DSTDIR): $(GDIR)
	test -d $@ || $(MKDIR) -p $@
	test -f __init__.py || touch $(GDIR)/__init__.py
	@echo "import script" >> $(GDIR)/__init__.py

$(DSTDIR)/%: %
	$(INSTALL_DATA) $< $@

#doxygen:
DOXNAME=grasspython
