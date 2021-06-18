MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Dir.make
include $(MODULE_TOPDIR)/include/Make/Python.make

PYDIR = $(ETC)/python/grass

<<<<<<< HEAD
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
	utils
=======
SUBDIRS = app exceptions script ctypes grassdb temporal pygrass pydispatch imaging gunittest bandref jupyter
>>>>>>> 584e61d06d (wxGUI/datacatalog: fix setting output vector/raster format (#1596))

default: $(PYDIR)/__init__.py
	$(MAKE) subdirs

$(PYDIR):
	$(MKDIR) $@

$(PYDIR)/__init__.py: __init__.py | $(PYDIR)
	$(INSTALL_DATA) $< $@
