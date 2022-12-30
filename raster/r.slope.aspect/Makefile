MODULE_TOPDIR = ../..

PGM = r.slope.aspect

<<<<<<< HEAD
LIBES = $(RASTERLIB) $(GISLIB) $(MATHLIB)
EXTRA_LIBS = $(OPENMP_LIBPATH) $(OPENMP_LIB)
DEPENDENCIES = $(RASTERDEP) $(GISDEP)
EXTRA_CFLAGS = $(OPENMP_CFLAGS)
EXTRA_INC = $(OPENMP_INCPATH)
=======
LIBES = $(RASTERLIB) $(GISLIB) $(MATHLIB) $(OMPLIB)
DEPENDENCIES = $(RASTERDEP) $(GISDEP)
EXTRA_CFLAGS = $(OMPCFLAGS)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
