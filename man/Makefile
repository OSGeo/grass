MODULE_TOPDIR = ..

include $(MODULE_TOPDIR)/include/Make/Other.make

MANPAGES = $(patsubst $(HTMLDIR)/%.html,$(MANDIR)/%.$(MANSECT),$(wildcard $(HTMLDIR)/*.html))

GEM_PNG := $(wildcard ../gem/docs/GEM-Manual/*.png)
GEM_HTML := $(wildcard ../gem/docs/GEM-Manual/*.html)
GEM_DST := $(patsubst ../gem/docs/GEM-Manual/%,$(HTMLDIR)/gem/%,$(GEM_PNG) $(GEM_HTML))

DSTFILES := \
	$(HTMLDIR)/grassdocs.css \
	$(HTMLDIR)/grass_logo.png \
	$(HTMLDIR)/nviz/grassdocs.css \
	$(HTMLDIR)/nviz/grass_logo.png \
	$(HTMLDIR)/nviz/grass_icon.png \
	$(GEM_DST)

IDXSRC = full_index index \
	display database general imagery misc postscript raster raster3D vector
INDICES := $(patsubst %,$(HTMLDIR)/%.html,$(IDXSRC))

default: $(DSTFILES)
	@echo "Generating HTML manual pages index (help system)..."
	$(MAKE) $(INDICES)
	$(call build,check)
	$(MAKE) manpages

# This must be a separate target so that evaluation of $(MANPAGES)
# is delayed until the indices have been generated
manpages:
	$(MAKE) $(MANPAGES)

.PHONY: manpages

define build
GISBASE="$(GISBASE)" ARCH="$(ARCH)" ARCH_DISTDIR="$(ARCH_DISTDIR)" \
	$(PYTHON) ./build_$(1).py $(2)
endef

$(HTMLDIR)/full_index.html: $(wildcard $(HTMLDIR)/*.*.html)
	$(call build,full_index)

$(HTMLDIR)/index.html:
	$(call build,index)

$(HTMLDIR)/display.html: $(wildcard $(HTMLDIR)/d.*.html)
	$(call build,class,d display)

$(HTMLDIR)/database.html: $(wildcard $(HTMLDIR)/db.*.html)
	$(call build,class,db database)

$(HTMLDIR)/general.html: $(wildcard $(HTMLDIR)/g.*.html)
	$(call build,class,g general)

$(HTMLDIR)/imagery.html: $(wildcard $(HTMLDIR)/i.*.html)
	$(call build,class,i imagery)

$(HTMLDIR)/misc.html: $(wildcard $(HTMLDIR)/m.*.html)
	$(call build,class,m misc)

$(HTMLDIR)/postscript.html: $(wildcard $(HTMLDIR)/ps.*.html)
	$(call build,class,ps postscript)

$(HTMLDIR)/raster.html: $(wildcard $(HTMLDIR)/r.*.html)
	$(call build,class,r raster)

$(HTMLDIR)/raster3D.html: $(wildcard $(HTMLDIR)/r3.*.html)
	$(call build,class,r3 raster3D)

$(HTMLDIR)/vector.html: $(wildcard $(HTMLDIR)/v.*.html)
	$(call build,class,v vector)

$(HTMLDIR)/grassdocs.css: grassdocs.css
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/nviz/grassdocs.css: grassdocs.css | $(HTMLDIR)/nviz
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/grass_logo.png: grass_logo.png
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/nviz/grass_logo.png: grass_logo.png | $(HTMLDIR)/nviz
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/nviz/grass_icon.png: grass_icon.png | $(HTMLDIR)/nviz
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/gem/%.png: ../gem/docs/GEM-Manual/%.png | $(HTMLDIR)/gem
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/gem/%.html: ../gem/docs/GEM-Manual/%.html | $(HTMLDIR)/gem
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/nviz:
	$(MKDIR) $@

$(HTMLDIR)/gem:
	$(MKDIR) $@
