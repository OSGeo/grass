MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Python.make

DSTDIR = $(ETC)/python/grass/jupyter

<<<<<<< HEAD
<<<<<<< HEAD
MODULES = \
	setup \
	map \
	interactivemap \
	region \
	map3d \
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

PYFILES := $(patsubst %,$(DSTDIR)/%.py,$(MODULES) __init__)
PYCFILES := $(patsubst %,$(DSTDIR)/%.pyc,$(MODULES) __init__)

default: $(PYFILES) $(PYCFILES)

$(DSTDIR):
	$(MKDIR) $@

$(DSTDIR)/%: % | $(DSTDIR)
	$(INSTALL_DATA) $< $@
