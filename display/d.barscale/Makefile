MODULE_TOPDIR = ../..

PGM = d.barscale

LIBES = $(DISPLAYLIB) $(SYMBLIB) $(GISLIB)
DEPENDENCIES = $(DISPLAYDEP) $(SYMBDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd

# insert thumbnail previews
$(PGM).tmp.html: $(BIN)/$(PGM)$(EXE)
	$(call htmldesc,$<,$@)
	sed 's!^<dd><b>\([a-z0-9._]*\)</b>:!<dd><img width="80" height="12" src="$(ETC)/gui/images/barscales/\1.png"> <b>\1</b>:!' "$@" > "$@.tmp"
	mv -f "$@.tmp" "$@"

.INTERMEDIATE: $(PGM).tmp.html
