MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Python.make

DSTDIR = $(ETC)/python/grass/benchmark

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
MODULES = app plots results runners __main__
=======
MODULES = plots results runners
>>>>>>> da7f79c3f9 (libpython: Save and load benchmark results (#1711))
=======
MODULES = app plots results runners __main__
>>>>>>> ba3c0640fa (libpython: Support benchmarks of non-parallel runs better (#1733))
=======
MODULES = app plots results runners __main__
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

PYFILES := $(patsubst %,$(DSTDIR)/%.py,$(MODULES) __init__)
PYCFILES := $(patsubst %,$(DSTDIR)/%.pyc,$(MODULES) __init__)

default: $(PYFILES) $(PYCFILES)

$(DSTDIR):
	$(MKDIR) $@

$(DSTDIR)/%: % | $(DSTDIR)
	$(INSTALL_DATA) $< $@
