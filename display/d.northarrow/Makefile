MODULE_TOPDIR = ../..

PGM = d.northarrow

LIBES = $(DISPLAYLIB) $(SYMBLIB) $(GISLIB)
DEPENDENCIES = $(DISPLAYDEP) $(SYMBDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

# thumbnail previews
IMGSRC := $(wildcard thumbnails/*.png)
IMGDST := $(patsubst thumbnails/%,$(HTMLDIR)/northarrows/%,$(IMGSRC))
MDIMGDST := $(patsubst thumbnails/%,$(MDDIR)/source/northarrows/%,$(IMGSRC))

default: cmd $(IMGDST) $(MDIMGDST)

$(HTMLDIR)/northarrows/%.png: thumbnails/%.png | $(HTMLDIR)/northarrows
	$(INSTALL_DATA) $< $@

$(MDDIR)/source/northarrows/%.png: thumbnails/%.png | $(MDDIR)/source/northarrows
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/northarrows: $(HTMLDIR)
	$(MKDIR) $@

$(MDDIR)/source/northarrows: $(MDDIR)
	$(MKDIR) $@
