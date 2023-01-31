
MODULE_TOPDIR = ../..

<<<<<<< HEAD
LIBES2 = $(RASTERLIB) $(GISLIB) $(MATHLIB) $(OMPLIB)
LIBES3 = $(RASTER3DLIB) $(RASTERLIB) $(GISLIB) $(MATHLIB) $(OMPLIB)
DEPENDENCIES = $(RASTER3DDEP) $(GISDEP) $(RASTERDEP)
EXTRA_CFLAGS = $(OMPCFLAGS)
=======
LIBES2 = $(RASTERLIB) $(GISLIB) $(MATHLIB) $(OPENMP_LIBPATH) $(OPENMP_LIB)
LIBES3 = $(RASTER3DLIB) $(RASTERLIB) $(GISLIB) $(MATHLIB) $(OPENMP_LIBPATH) $(OPENMP_LIB)
DEPENDENCIES = $(RASTER3DDEP) $(GISDEP) $(RASTERDEP)
EXTRA_CFLAGS = $(OPENMP_CFLAGS)
EXTRA_INC = $(OPENMP_INCPATH)
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))

PROGRAMS = r.univar r3.univar

r_univar_OBJS = r.univar_main.o sort.o stats.o
r3_univar_OBJS = r3.univar_main.o sort.o stats.o

include $(MODULE_TOPDIR)/include/Make/Multi.make

default: multi

$(BIN)/r.univar$(EXE): LIBES = $(LIBES2)
$(BIN)/r3.univar$(EXE): LIBES = $(LIBES3)
