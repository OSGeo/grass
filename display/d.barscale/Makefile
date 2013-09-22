MODULE_TOPDIR = ../..

PGM = d.barscale

LIBES = $(DISPLAYLIB) $(SYMBLIB) $(GISLIB)
DEPENDENCIES = $(DISPLAYDEP) $(SYMBDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

# thumbnail previews
IMGSRC := $(wildcard thumbnails/*.png)
IMGDST := $(patsubst thumbnails/%,$(HTMLDIR)/barscales/%,$(IMGSRC))

default: cmd $(IMGDST)

$(HTMLDIR)/barscales/%.png: thumbnails/%.png | $(HTMLDIR)/barscales
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/barscales: $(HTMLDIR)
	$(MKDIR) $@

# insert thumbnail previews
$(PGM).tmp.html: $(BIN)/$(PGM)$(EXE)
	$(call htmldesc,$<,$@)
	sed 's!^<dd><b>\([a-z0-9._]*\)</b>:!<dd><img width="80" height="12" src="$(HTMLDIR)/barscales/\1.png"> <b>\1</b>:!' "$@" > "$@.tmp"
	mv -f "$@.tmp" "$@"

.INTERMEDIATE: $(PGM).tmp.html
