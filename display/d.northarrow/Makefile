MODULE_TOPDIR = ../..

PGM = d.northarrow

LIBES = $(DISPLAYLIB) $(SYMBLIB) $(GISLIB)
DEPENDENCIES = $(DISPLAYDEP) $(SYMBDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

# Thumbnail previews. The copy in $(GUIDIR)/images is used by the GUI at
# runtime (the north arrow selection widget); the documentation trees have
# their own copies.
IMGSRC := $(wildcard thumbnails/*.png)
IMGDST := $(patsubst thumbnails/%,$(HTMLDIR)/northarrows/%,$(IMGSRC))
MDIMGDST := $(patsubst thumbnails/%,$(MDDIR)/source/northarrows/%,$(IMGSRC))
GUIIMGDST := $(patsubst thumbnails/%,$(GUIDIR)/images/northarrows/%,$(IMGSRC))

default: cmd $(IMGDST) $(MDIMGDST) $(GUIIMGDST)

$(HTMLDIR)/northarrows/%.png: thumbnails/%.png | $(HTMLDIR)/northarrows
	$(INSTALL_DATA) $< $@

$(MDDIR)/source/northarrows/%.png: thumbnails/%.png | $(MDDIR)/source/northarrows
	$(INSTALL_DATA) $< $@

$(GUIDIR)/images/northarrows/%.png: thumbnails/%.png | $(GUIDIR)/images/northarrows
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/northarrows: $(HTMLDIR)
	$(MKDIR) $@

$(MDDIR)/source/northarrows: $(MDDIR)
	$(MKDIR) $@

$(GUIDIR)/images/northarrows:
	$(MKDIR) $@
