MODULE_TOPDIR = ../..

PGM = v.colors

LIBES = $(VECTORLIB) $(DBMILIB) $(GISLIB) $(RASTERLIB) $(RASTER3DLIB)
DEPENDENCIES = $(VECTORDEP) $(DBMIDEP) $(GISDEP) $(RASTERDEP) $(RASTER3DDEP)
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd

# Insert thumbnail previews
v.colors.tmp.html: $(BIN)/v.colors$(EXE) thumbnails
	$(call htmldesc,$<,$@)
	sed 's!^<dd><b>\([a-z0-9._]*\)</b>:!<dd><img width="80" height="12" src="Colortable_\1.png"> <b>\1</b>:!' "$@" > "$@.tmp"
	mv -f "$@.tmp" "$@"
	$(MAKE) thumbnails

thumbnails: $(BIN)/r.mapcalc$(EXE)
	-$(call run_grass, $(GRASS_HOME)/tools/thumbnails.py)

.PHONY: thumbnails

.INTERMEDIATE: v.colors.tmp.html
