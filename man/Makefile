MODULE_TOPDIR = ..

include $(MODULE_TOPDIR)/include/Make/Other.make

MANPAGES := $(patsubst $(HTMLDIR)/%.html,$(MANDIR)/%.$(MANSECT),$(wildcard $(HTMLDIR)/*.html))

DSTFILES := \
	$(HTMLDIR)/grassdocs.css \
	$(HTMLDIR)/grass_logo.png \
	$(HTMLDIR)/grass_icon.png \
	$(HTMLDIR)/jquery.fixedheadertable.min.js \
	$(HTMLDIR)/parser_standard_options.css \
	$(HTMLDIR)/parser_standard_options.js

categories = \
	d:display \
	db:database \
	g:general \
	i:imagery \
	m:miscellaneous \
	ps:postscript \
	r:raster \
	r3:raster3d \
	t:temporal \
	v:vector

IDXCATS := $(foreach cat,$(categories),$(lastword $(subst :, ,$(cat))))

IDXSRC = full_index index topics keywords graphical_index manual_gallery class_graphical parser_standard_options $(IDXCATS)

INDICES := $(patsubst %,$(HTMLDIR)/%.html,$(IDXSRC))

ALL_HTML := $(wildcard $(HTMLDIR)/*.*.html)

ifneq (@(type sphinx-build2 > /dev/null),)
SPHINXBUILD   = sphinx-build2
endif
ifneq (@(type sphinx-build > /dev/null),)
SPHINXBUILD   = sphinx-build
endif

default: $(DSTFILES)
	@echo "Generating HTML manual pages index (help system)..."
	$(MAKE) $(INDICES)
	$(call build,check)
	$(MAKE) manpages

# This must be a separate target so that evaluation of $(MANPAGES)
# is delayed until the indices have been generated
left := (
right := )
manpages:
	$(MAKE) $(subst $(left),\$(left),$(subst $(right),\$(right),$(MANPAGES)))

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

define build_graphical_index
GISBASE="$(RUN_GISBASE)" ARCH="$(ARCH)" ARCH_DISTDIR="$(ARCH_DISTDIR)" \
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) \
	$(PYTHON) ./build_graphical_index.py $(HTMLDIR)
endef

define build_manual_gallery
GISBASE="$(RUN_GISBASE)" ARCH="$(ARCH)" ARCH_DISTDIR="$(ARCH_DISTDIR)" \
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) \
	$(PYTHON) ./build_manual_gallery.py $(HTMLDIR)
endef

define build_pso
GISBASE="$(RUN_GISBASE)" ARCH="$(ARCH)" ARCH_DISTDIR="$(ARCH_DISTDIR)" \
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) \
	$(PYTHON) ./parser_standard_options.py -t "$(GRASS_HOME)/lib/gis/parser_standard_options.c" \
	-f "grass" -o "$(HTMLDIR)/parser_standard_options.html" -p 'id="opts_table" class="scroolTable"'
endef

$(HTMLDIR)/topics.html: $(ALL_HTML)

define build_class_graphical
GISBASE="$(RUN_GISBASE)" ARCH="$(ARCH)" ARCH_DISTDIR="$(ARCH_DISTDIR)" \
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) \
	$(PYTHON) ./build_class_graphical.py $(HTMLDIR)
endef

$(HTMLDIR)/topics.html: $(ALL_HTML) build_topics.py
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


$(HTMLDIR)/graphical_index.html: $(ALL_HTML)
	$(call build_graphical_index)
	touch $@

$(HTMLDIR)/manual_gallery.html: $(ALL_HTML)
	$(call build_manual_gallery)

$(HTMLDIR)/parser_standard_options.html: $(ALL_HTML)
	$(call build_pso)
	touch $@

# TODO: this should be done in the same way as category_rule
$(HTMLDIR)/class_graphical.html: $(ALL_HTML)
	$(call build_class_graphical)
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

$(HTMLDIR)/jquery.fixedheadertable.min.js: jquery.fixedheadertable.min.js
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/parser_standard_options.js: parser_standard_options.js
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/parser_standard_options.css: parser_standard_options.css
	$(INSTALL_DATA) $< $@
