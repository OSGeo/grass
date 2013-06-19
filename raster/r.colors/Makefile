MODULE_TOPDIR = ../..

LIBES = $(RASTER3DLIB) $(RASTERLIB) $(GISLIB)
DEPENDENCIES = $(RASTER3DDEP) $(GISDEP) $(RASTERDEP)

PROGRAMS = r.colors r3.colors

r_colors_OBJS = raster_main.o edit_colors.o rules.o stats.o
r3_colors_OBJS = raster3d_main.o edit_colors.o rules.o stats.o

include $(MODULE_TOPDIR)/include/Make/Multi.make

default: multi

# Insert thumbnail previews
r.colors.tmp.html: $(BIN)/r.colors$(EXE) thumbnails
	$(call htmldesc,$<,$@)
	sed 's!^<dd><b>\([a-z0-9._]*\)</b>:!<dd><img width="80" height="12" src="Colortable_\1.png"> <b>\1</b>:!' "$@" > "$@.tmp"
	mv -f "$@.tmp" "$@"
	$(MAKE) thumbnails

r3.colors.tmp.html: $(BIN)/r3.colors$(EXE) thumbnails
	$(call htmldesc,$<,$@)
	sed 's!^<dd><b>\([a-z0-9._]*\)</b>:!<dd><img width="80" height="12" src="Colortable_\1.png"> <b>\1</b>:!' "$@" > "$@.tmp"
	mv -f "$@.tmp" "$@"
	$(MAKE) thumbnails

thumbnails: $(BIN)/r.mapcalc$(EXE)
	-$(call run_grass, $(GRASS_HOME)/tools/thumbnails.py)

$(BIN)/r.mapcalc$(EXE):
	$(MAKE) -C ../r.mapcalc

.PHONY: thumbnails

.INTERMEDIATE: r.colors.tmp.html r3.colors.tmp.html


