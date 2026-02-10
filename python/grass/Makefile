MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Dir.make
include $(MODULE_TOPDIR)/include/Make/Python.make

PYDIR = $(ETC)/python/grass

SUBDIRS = \
	app \
	benchmark \
	exceptions \
	experimental \
	grassdb \
	gunittest \
	imaging \
	jupyter \
	pydispatch \
	pygrass \
	script \
	semantic_label \
	temporal \
	tools \
	utils

default: $(PYDIR)/__init__.py
	$(MAKE) subdirs

$(PYDIR):
	$(MKDIR) $@

$(PYDIR)/__init__.py: __init__.py | $(PYDIR)
	$(INSTALL_DATA) $< $@
