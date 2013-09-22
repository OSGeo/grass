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
