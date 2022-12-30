MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Dir.make
include $(MODULE_TOPDIR)/include/Make/Python.make

PYDIR = $(ETC)/python/grass

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
SUBDIRS = \
	app \
	benchmark \
	exceptions \
<<<<<<< HEAD
	experimental \
=======
SUBDIRS = \
	app \
	benchmark \
	exceptions \
>>>>>>> 6d691c8cd4 (wxGUI/splashscreen: use standard one, try to show it before main app, increase timeout (#1718))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
	grassdb \
	gunittest \
	imaging \
	jupyter \
	pydispatch \
	pygrass \
	script \
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
	semantic_label \
	temporal \
	utils
=======
SUBDIRS = app exceptions script ctypes grassdb temporal pygrass pydispatch imaging gunittest bandref jupyter
>>>>>>> 584e61d06d (wxGUI/datacatalog: fix setting output vector/raster format (#1596))
=======
SUBDIRS = app exceptions script ctypes grassdb temporal pygrass pydispatch imaging gunittest bandref jupyter
>>>>>>> 847046a031 (Add session initiations functions for Jupyter Notebooks (#1629))
=======
SUBDIRS = app benchmark exceptions script ctypes grassdb temporal pygrass pydispatch imaging gunittest bandref jupyter
>>>>>>> da7f79c3f9 (libpython: Save and load benchmark results (#1711))
=======
	temporal \
	utils
>>>>>>> 6d691c8cd4 (wxGUI/splashscreen: use standard one, try to show it before main app, increase timeout (#1718))
=======
	semantic_label \
	temporal \
	utils
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

default: $(PYDIR)/__init__.py
	$(MAKE) subdirs

$(PYDIR):
	$(MKDIR) $@

$(PYDIR)/__init__.py: __init__.py | $(PYDIR)
	$(INSTALL_DATA) $< $@
