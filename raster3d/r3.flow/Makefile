MODULE_TOPDIR = ../..

PROGRAMS=r3.flow test.r3flow 

LIBES = $(RASTER3DLIB) $(RASTERLIB) $(GISLIB) $(VECTORLIB) $(DBMILIB)
DEPENDENCIES = $(RASTER3DDEP) $(RASTERDEP) $(GISDEP) $(VECTORDEP) $(DBMIDEP)
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

r3_flow_OBJS = main.o flowline.o integrate.o interpolate.o voxel_traversal.o
test_r3flow_OBJS = test_main.o flowline.o integrate.o interpolate.o voxel_traversal.o

include $(MODULE_TOPDIR)/include/Make/Multi.make

default: multi
