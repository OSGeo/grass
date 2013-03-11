MODULE_TOPDIR = ..

include $(MODULE_TOPDIR)/include/Make/Other.make

MANPAGES := $(patsubst $(HTMLDIR)/%.html,$(MANDIR)/%.$(MANSECT),$(wildcard $(HTMLDIR)/*.html))

DSTFILES := \
	$(HTMLDIR)/grassdocs.css \
	$(HTMLDIR)/grass_logo.png \
	$(HTMLDIR)/grass_icon.png

categories = \
	d:display \
	db:database \
	g:general \
	i:imagery \
	m:misc \
	ps:postscript \
	r:raster \
	r3:raster3D \
	t:temporal \
	v:vector

IDXCATS := $(foreach cat,$(categories),$(lastword $(subst :, ,$(cat))))

IDXSRC = full_index index topics keywords $(IDXCATS)

INDICES := $(patsubst %,$(HTMLDIR)/%.html,$(IDXSRC))

ALL_HTML := $(wildcard $(HTMLDIR)/*.*.html)

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
GISBASE="$(RUN_GISBASE)" ARCH="$(ARCH)" ARCH_DISTDIR="$(ARCH_DISTDIR)" \
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) \
	$(PYTHON) ./build_$(1).py $(2)
endef

define build_topics
GISBASE="$(RUN_GISBASE)" ARCH="$(ARCH)" ARCH_DISTDIR="$(ARCH_DISTDIR)" \
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) \
	$(PYTHON) ./build_topics.py $(HTMLDIR)
endef

define build_keywords
GISBASE="$(RUN_GISBASE)" ARCH="$(ARCH)" ARCH_DISTDIR="$(ARCH_DISTDIR)" \
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) \
	$(PYTHON) ./build_keywords.py $(HTMLDIR)
endef

$(HTMLDIR)/topics.html: $(ALL_HTML)
	$(call build_topics)
	touch $@

$(HTMLDIR)/full_index.html: $(ALL_HTML) build_full_index.py build_html.py
	$(call build,full_index)
	touch $@

$(HTMLDIR)/index.html: build_index.py build_html.py
	$(call build,index)
	touch $@

$(HTMLDIR)/keywords.html: $(ALL_HTML)
	$(call build_keywords)
	touch $@

define category_rule
$$(HTMLDIR)/$(2).html: $$(wildcard $$(HTMLDIR)/$(1).*.html) build_class.py build_html.py
	$$(call build,class,$(1) $(2))
	touch $$@
endef

$(foreach cat,$(categories),$(eval $(call category_rule,$(firstword $(subst :, ,$(cat))),$(lastword $(subst :, ,$(cat))))))

$(HTMLDIR)/grassdocs.css: grassdocs.css
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/grass_logo.png: grass_logo.png
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/grass_icon.png: grass_icon.png
	$(INSTALL_DATA) $< $@
