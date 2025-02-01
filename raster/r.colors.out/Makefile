MODULE_TOPDIR = ../..

LIBES2 = $(RASTERLIB) $(GISLIB)
LIBES3 = $(RASTER3DLIB) $(RASTERLIB) $(GISLIB)
DEPENDENCIES = $(RASTER3DDEP) $(GISDEP) $(RASTERDEP)

PROGRAMS = r.colors.out r3.colors.out

r_colors_out_OBJS = raster_main.o
r3_colors_out_OBJS = raster3d_main.o

include $(MODULE_TOPDIR)/include/Make/Multi.make

default: multi

$(BIN)/r.colors.out$(EXE): LIBES = $(LIBES2)
$(BIN)/r3.colors.out$(EXE):  LIBES = $(LIBES3)
