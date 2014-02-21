MODULE_TOPDIR = ../..

PGM = v.colors

LIBES = $(VECTORLIB) $(DBMILIB) $(GISLIB) $(RASTERLIB) $(RASTER3DLIB)
DEPENDENCIES = $(VECTORDEP) $(DBMIDEP) $(GISDEP) $(RASTERDEP) $(RASTER3DDEP)
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd

thumbnails: $(BIN)/r.mapcalc$(EXE)
	-$(call run_grass, $(GRASS_HOME)/tools/thumbnails.py)

.PHONY: thumbnails

.INTERMEDIATE: v.colors.tmp.html
