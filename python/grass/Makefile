MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Dir.make
include $(MODULE_TOPDIR)/include/Make/Python.make

PYDIR = $(ETC)/python/grass

SUBDIRS = \
	app \
	bandref \
	benchmark \
	exceptions \
	grassdb \
	gunittest \
	imaging \
	jupyter \
	pydispatch \
	pygrass \
	script \
	temporal \
	utils

default: $(PYDIR)/__init__.py
	$(MAKE) subdirs

$(PYDIR):
	$(MKDIR) $@

$(PYDIR)/__init__.py: __init__.py | $(PYDIR)
	$(INSTALL_DATA) $< $@
