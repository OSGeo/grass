MODULE_TOPDIR = ../..

PGM = r.colors

LIBES = $(RASTERLIB) $(GISLIB) $(MATHLIB) $(G3DLIB)
DEPENDENCIES = $(RASTERDEP) $(GISDEP) $(G3DDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd

# Insert thumbnail previews
r.colors.tmp.html: $(HTMLSRC) thumbnails.py
	$(call htmldesc,$<,$@)
	sed 's!^<DD><b>\([a-z0-9.]*\)</b>:!<DD><img width="80" height="12" src="Colortable_\1.png"> <b>\1</b>:!' "$@" > "$@.tmp"
	mv -f "$@.tmp" "$@"
	$(MAKE) thumbnails

thumbnails: $(BIN)/r.mapcalc$(EXE)
	-$(call run_grass, ./thumbnails.py)

$(BIN)/r.mapcalc$(EXE):
	$(MAKE) -C ../r.mapcalc

.PHONY: thumbnails

.INTERMEDIATE: r.colors.tmp.html
