MODULE_TOPDIR = ../..

LIBES = $(RASTER3DLIB) $(RASTERLIB) $(GISLIB)
DEPENDENCIES = $(RASTER3DDEP) $(GISDEP) $(RASTERDEP)

PROGRAMS = r.colors r3.colors

r_colors_OBJS = raster_main.o edit_colors.o rules.o stats.o
r3_colors_OBJS = raster3d_main.o edit_colors.o rules.o stats.o

include $(MODULE_TOPDIR)/include/Make/Multi.make

default: multi thumbnails

thumbnails: $(BIN)/r.mapcalc$(EXE) $(BIN)/r.colors$(EXE) $(HTMLDIR)/colortables
ifndef CROSS_COMPILING
	-$(call run_grass, $(GRASS_HOME)/utils/thumbnails.py)
endif

$(HTMLDIR)/colortables: $(HTMLDIR)
	$(MKDIR) $@

$(BIN)/r.mapcalc$(EXE):
	$(MAKE) -C ../r.mapcalc

.PHONY: thumbnails
