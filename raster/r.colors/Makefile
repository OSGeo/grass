MODULE_TOPDIR = ../..

PGM = r.colors

LIBES = $(RASTERLIB) $(GISLIB) $(MATHLIB)
DEPENDENCIES = $(RASTERDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd

# Insert thumbnail previews
#  Try overriding the default rule with:
#r.colors.tmp.html: $(HTMLSRC)
#	$(call htmldesc,$<,$@)
#	sed 's!^<DD><b>\([a-z0-9.]*\)</b>:!<DD><img width="80" height="12" \
#		src="Colortable_\1_i.png"><b>\1</b>:!' $@ > $@.tmp
#	mv -f $@.tmp $@
