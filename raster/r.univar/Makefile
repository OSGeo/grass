
MODULE_TOPDIR = ../..

LIBES2 = $(RASTERLIB) $(GISLIB) $(MATHLIB) $(OPENMP_LIBPATH) $(OPENMP_LIB) $(PARSONLIB)
LIBES3 = $(RASTER3DLIB) $(RASTERLIB) $(GISLIB) $(MATHLIB) $(OPENMP_LIBPATH) $(OPENMP_LIB) $(PARSONLIB)
DEPENDENCIES = $(RASTER3DDEP) $(GISDEP) $(RASTERDEP)
EXTRA_CFLAGS = $(OPENMP_CFLAGS)
EXTRA_INC = $(OPENMP_INCPATH)

PROGRAMS = r.univar r3.univar

r_univar_OBJS = r.univar_main.o sort.o stats.o
r3_univar_OBJS = r3.univar_main.o sort.o stats.o

include $(MODULE_TOPDIR)/include/Make/Multi.make

default: multi

$(BIN)/r.univar$(EXE): LIBES = $(LIBES2)
$(BIN)/r3.univar$(EXE): LIBES = $(LIBES3)
