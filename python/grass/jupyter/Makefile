MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Python.make

DSTDIR = $(ETC)/python/grass/jupyter

<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
MODULES = \
	setup \
	map \
	interactivemap \
	region \
	map3d \
<<<<<<< HEAD
<<<<<<< HEAD
	reprojection_renderer \
	utils \
	timeseriesmap
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
	seriesmap \
	reprojection_renderer \
	utils \
	timeseriesmap
=======
MODULES = setup
>>>>>>> 584e61d06d (wxGUI/datacatalog: fix setting output vector/raster format (#1596))
=======
MODULES = setup
>>>>>>> 847046a031 (Add session initiations functions for Jupyter Notebooks (#1629))
=======
MODULES = setup display
>>>>>>> da7f79c3f9 (libpython: Save and load benchmark results (#1711))
=======
MODULES = \
	setup \
<<<<<<< HEAD
	display \
<<<<<<< HEAD
	interact_display
>>>>>>> ba3c0640fa (libpython: Support benchmarks of non-parallel runs better (#1733))
=======
	interact_display \
	utils
>>>>>>> 7896e1a53f (wxGUI/Single-Window: New change page event for AuiNotebook (#1780))
=======
	map \
	interactivemap \
	region \
	map3d \
	reprojection_renderer \
	utils \
	timeseriesmap
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
	reprojection_renderer \
	utils \
	timeseriesmap
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

PYFILES := $(patsubst %,$(DSTDIR)/%.py,$(MODULES) __init__)
PYCFILES := $(patsubst %,$(DSTDIR)/%.pyc,$(MODULES) __init__)

default: $(PYFILES) $(PYCFILES)

$(DSTDIR):
	$(MKDIR) $@

$(DSTDIR)/%: % | $(DSTDIR)
	$(INSTALL_DATA) $< $@
