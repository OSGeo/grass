MODULE_TOPDIR = ../..

LIBES = $(RASTER3DLIB) $(RASTERLIB) $(GISLIB)
DEPENDENCIES = $(RASTER3DDEP) $(GISDEP) $(RASTERDEP)

PROGRAMS = r.colors r3.colors

r_colors_OBJS = raster_main.o edit_colors.o rules.o stats.o
r3_colors_OBJS = raster3d_main.o edit_colors.o rules.o stats.o

include $(MODULE_TOPDIR)/include/Make/Multi.make

default: multi thumbnails

# The copy in $(GUIDIR)/images is used by the GUI at runtime (the color
# table selection widget); the documentation trees have their own copies.
thumbnails: $(BIN)/r.mapcalc$(EXE) $(BIN)/r.colors$(EXE) $(HTMLDIR)/colortables $(MDDIR)/source/colortables $(GUIDIR)/images/colortables
ifndef CROSS_COMPILING
	$(call run_grass, $(GRASS_HOME)/utils/thumbnails.py $(MDDIR)/source/colortables)
	$(call run_grass, $(GRASS_HOME)/utils/thumbnails.py $(HTMLDIR)/colortables)
	$(call run_grass, $(GRASS_HOME)/utils/thumbnails.py $(GUIDIR)/images/colortables)
endif

$(HTMLDIR)/colortables: $(HTMLDIR)
	$(MKDIR) $@

$(MDDIR)/source/colortables: $(MDDIR)
	$(MKDIR) $@

$(GUIDIR)/images/colortables:
	$(MKDIR) $@

.PHONY: thumbnails
