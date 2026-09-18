MODULE_TOPDIR = ../..

PGM = d.barscale

LIBES = $(DISPLAYLIB) $(SYMBLIB) $(GISLIB)
DEPENDENCIES = $(DISPLAYDEP) $(SYMBDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

# Thumbnail previews. The copy in $(GUIDIR)/images is used by the GUI at
# runtime (the barscale selection widget); the documentation trees have
# their own copies.
IMGSRC := $(wildcard thumbnails/*.png)
IMGDST := $(patsubst thumbnails/%,$(HTMLDIR)/barscales/%,$(IMGSRC))
MDIMGDST := $(patsubst thumbnails/%,$(MDDIR)/source/barscales/%,$(IMGSRC))
GUIIMGDST := $(patsubst thumbnails/%,$(GUIDIR)/images/barscales/%,$(IMGSRC))

default: cmd $(IMGDST) $(MDIMGDST) $(GUIIMGDST)

$(HTMLDIR)/barscales/%.png: thumbnails/%.png | $(HTMLDIR)/barscales
	$(INSTALL_DATA) $< $@

$(MDDIR)/source/barscales/%.png: thumbnails/%.png | $(MDDIR)/source/barscales
	$(INSTALL_DATA) $< $@

$(GUIDIR)/images/barscales/%.png: thumbnails/%.png | $(GUIDIR)/images/barscales
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/barscales: $(HTMLDIR)
	$(MKDIR) $@

$(MDDIR)/source/barscales: $(MDDIR)
	$(MKDIR) $@

$(GUIDIR)/images/barscales:
	$(MKDIR) $@
