MODULE_TOPDIR = ..

include $(MODULE_TOPDIR)/include/Make/Other.make

MANPAGES := $(patsubst $(HTMLDIR)/%.html,$(MANDIR)/%.$(MANSECT),$(wildcard $(HTMLDIR)/*.html))

DSTFILES := \
	$(HTMLDIR)/grassdocs.css \
	$(HTMLDIR)/grass_logo.png \
	$(HTMLDIR)/hamburger_menu.svg \
	$(HTMLDIR)/hamburger_menu_close.svg \
	$(HTMLDIR)/grass_icon.png \
	$(HTMLDIR)/jquery.fixedheadertable.min.js \
	$(HTMLDIR)/parser_standard_options.css \
	$(HTMLDIR)/parser_standard_options.js \
	$(MDDIR)/mkdocs.yml \
	$(MDDIR)/source/grass_logo.svg \
	$(MDDIR)/source/grass_icon.png \
	$(MDDIR)/source/favicon.ico \
	$(MDDIR)/source/grassdocs.css \
	$(MDDIR)/scripts/hook_list_scripts.py \
	$(MDDIR)/source/index.md \
	$(MDDIR)/overrides/partials/footer.html \
	$(MDDIR)/overrides/partials/actions.html \
	$(MDDIR)/overrides/partials/source.html \
	$(MDDIR)/overrides/fragments/tags/default/listing.html \
	$(MDDIR)/source/keywords.md \
	$(MDDIR)/source/development_intro.md \
	$(MDDIR)/source/interfaces_overview.md \
	$(MDDIR)/source/jupyter_intro.md \
	$(MDDIR)/source/python_intro.md \
	$(MDDIR)/source/command_line_intro.md

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

EXTRA_CLEAN_DIRS := $(wildcard __pycache__)

IDXCATS := $(foreach cat,$(categories),$(lastword $(subst :, ,$(cat))))

IDXSRC = full_index index topics keywords graphical_index manual_gallery class_graphical parser_standard_options $(IDXCATS)

INDICES := $(patsubst %,$(HTMLDIR)/%.html,$(IDXSRC))

IDXSRC_MD = full_index index topics graphical_index manual_gallery class_graphical parser_standard_options $(IDXCATS)
INDICES_MD := $(patsubst %,$(MDDIR)/source/%.md,$(IDXSRC_MD))

ALL_HTML := $(wildcard $(HTMLDIR)/*.*.html)
ALL_MD := $(wildcard $(MDDIR)/source/*.*.md)

ifneq (@(type sphinx-build2 > /dev/null),)
SPHINXBUILD   = sphinx-build2
endif
ifneq (@(type sphinx-build > /dev/null),)
SPHINXBUILD   = sphinx-build
endif

default: $(DSTFILES)
	@echo "Generating manual pages index (help system)..."
	$(MAKE) $(INDICES)
	$(call build,check)
	$(MAKE) manpages
	$(MAKE) $(INDICES_MD)
# $(MAKE) build-mkdocs

# This must be a separate target so that evaluation of $(MANPAGES)
# is delayed until the indices have been generated
left := (
right := )
manpages:
	$(MAKE) $(subst $(left),\$(left),$(subst $(right),\$(right),$(MANPAGES)))

.PHONY: manpages

define build
GISBASE="$(RUN_GISBASE)" ARCH="$(ARCH)" ARCH_DISTDIR="$(ARCH_DISTDIR)" \
	MDDIR="${MDDIR}" \
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) \
	$(PYTHON) ./build_$(1).py $(2)
endef

define build_topics
GISBASE="$(RUN_GISBASE)" ARCH="$(ARCH)" ARCH_DISTDIR="$(ARCH_DISTDIR)" \
	MDDIR="${MDDIR}" \
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) \
	$(PYTHON) ./build_topics.py
endef

define build_keywords
GISBASE="$(RUN_GISBASE)" ARCH="$(ARCH)" ARCH_DISTDIR="$(ARCH_DISTDIR)" \
	MDDIR="${MDDIR}" \
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) \
	$(PYTHON) ./build_keywords.py
endef

define build_graphical_index
GISBASE="$(RUN_GISBASE)" ARCH="$(ARCH)" ARCH_DISTDIR="$(ARCH_DISTDIR)" \
	MDDIR="${MDDIR}" \
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) \
	$(PYTHON) ./build_graphical_index.py
endef

define build_manual_gallery
GISBASE="$(RUN_GISBASE)" ARCH="$(ARCH)" ARCH_DISTDIR="$(ARCH_DISTDIR)" \
	MDDIR="${MDDIR}" \
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) \
	$(PYTHON) ./build_manual_gallery.py
endef

define build_pso
GISBASE="$(RUN_GISBASE)" ARCH="$(ARCH)" ARCH_DISTDIR="$(ARCH_DISTDIR)" \
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) \
	$(PYTHON) ./parser_standard_options.py -t "$(GRASS_HOME)/lib/gis/parser_standard_options.c" \
	-f "grass" -o "$(HTMLDIR)/parser_standard_options.html" -p 'id="opts_table" class="scroolTable"'
endef

define build_pso_md
GISBASE="$(RUN_GISBASE)" ARCH="$(ARCH)" ARCH_DISTDIR="$(ARCH_DISTDIR)" \
	MDDIR="${MDDIR}" \
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) \
	$(PYTHON) ./parser_standard_options.py -t "$(GRASS_HOME)/lib/gis/parser_standard_options.c" \
	-f "grass" -o "$(MDDIR)/source/parser_standard_options.md"
endef

$(HTMLDIR)/topics.html: $(ALL_HTML)

$(MDDIR)/source/topics.md: $(ALL_MD)

define build_class_graphical
GISBASE="$(RUN_GISBASE)" ARCH="$(ARCH)" ARCH_DISTDIR="$(ARCH_DISTDIR)" \
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) \
	$(PYTHON) ./build_class_graphical.py html $(HTMLDIR)
endef

define build_class_graphical_md
GISBASE="$(RUN_GISBASE)" ARCH="$(ARCH)" ARCH_DISTDIR="$(ARCH_DISTDIR)" \
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) \
	$(PYTHON) ./build_class_graphical.py md $(MDDIR)/source
endef

$(HTMLDIR)/topics.html: $(ALL_HTML) build_topics.py
	$(call build_topics)
	touch $@

$(MDDIR)/source/topics.md: $(ALL_MD) build_topics.py
	$(call build_topics)
	touch $@

$(HTMLDIR)/full_index.html: $(ALL_HTML) build_full_index.py build_html.py
	$(call build,full_index)
	touch $@

$(MDDIR)/source/full_index.md: $(ALL_MD) build_full_index.py build_md.py
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

$(MDDIR)/source/graphical_index.md: $(ALL_MD)
	$(call build_graphical_index)
	touch $@

$(HTMLDIR)/manual_gallery.html: $(ALL_HTML)
	$(call build_manual_gallery)

$(MDDIR)/source/manual_gallery.md: $(ALL_MD)
	$(call build_manual_gallery)

$(HTMLDIR)/parser_standard_options.html: $(ALL_HTML)
	$(call build_pso)
	touch $@

$(MDDIR)/source/parser_standard_options.md: $(ALL_MD)
	$(call build_pso_md)
	touch $@

# TODO: this should be done in the same way as category_rule
$(HTMLDIR)/class_graphical.html: $(ALL_HTML)
	$(call build_class_graphical)
	touch $@

# TODO: this should be done in the same way as category_rule
$(MDDIR)/source/class_graphical.md: $(ALL_MD)
	$(call build_class_graphical_md)
	touch $@

define category_rule
$$(HTMLDIR)/$(2).html: $$(wildcard $$(HTMLDIR)/$(1).*.html) build_class.py build_html.py
	$$(call build,class,$(1) $(2))
	touch $$@
endef

$(foreach cat,$(categories),$(eval $(call category_rule,$(firstword $(subst :, ,$(cat))),$(lastword $(subst :, ,$(cat))))))

define category_rule_md
$$(MDDIR)/source/$(2).md: $$(wildcard $$(MDDIR)/source/$(1).*.md) build_class.py build_md.py
	$$(call build,class,$(1) $(2))
	touch $$@
endef

$(foreach cat,$(categories),$(eval $(call category_rule_md,$(firstword $(subst :, ,$(cat))),$(lastword $(subst :, ,$(cat))))))


$(HTMLDIR)/grassdocs.css: grassdocs.css
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/grass_logo.png: grass_logo.png
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/hamburger_menu.svg: hamburger_menu.svg
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/hamburger_menu_close.svg: hamburger_menu_close.svg
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/grass_icon.png: grass_icon.png
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/jquery.fixedheadertable.min.js: jquery.fixedheadertable.min.js
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/parser_standard_options.js: parser_standard_options.js
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/parser_standard_options.css: parser_standard_options.css
	$(INSTALL_DATA) $< $@

$(MDDIR)/mkdocs.yml: mkdocs/mkdocs.yml
	$(INSTALL_DATA) $< $@

$(MDDIR)/source/index.md: mkdocs/index.md
	$(INSTALL_DATA) $< $@

$(MDDIR)/source/grass_logo.svg: grass_logo.svg
	$(INSTALL_DATA) $< $@

$(MDDIR)/grass_icon.png: grass_icon.png
	$(INSTALL_DATA) $< $@

$(MDDIR)/source/favicon.ico: favicon.ico
	$(INSTALL_DATA) $< $@

$(MDDIR)/source/grassdocs.css: mkdocs/grassdocs.css
	$(INSTALL_DATA) $< $@

$(MDDIR)/source/keywords.md: mkdocs/keywords.md
	$(INSTALL_DATA) $< $@

$(MDDIR)/source/development_intro.md: mkdocs/docs/development_intro.md
	$(INSTALL_DATA) $< $@

$(MDDIR)/source/python_intro.md: mkdocs/docs/python_intro.md
	$(INSTALL_DATA) $< $@

$(MDDIR)/source/command_line_intro.md: mkdocs/docs/command_line_intro.md
	$(INSTALL_DATA) $< $@

$(MDDIR)/scripts/hook_list_scripts.py: mkdocs/scripts/hook_list_scripts.py | $(MDDIR)/scripts
	$(INSTALL_DATA) $< $@

$(MDDIR)/scripts:
	$(MKDIR) $@

$(MDDIR)/overrides/partials/footer.html: mkdocs/overrides/partials/footer.html | $(MDDIR)/overrides/partials
	$(INSTALL_DATA) $< $@

$(MDDIR)/overrides/partials/actions.html: mkdocs/overrides/partials/actions.html | $(MDDIR)/overrides/partials
	$(INSTALL_DATA) $< $@

$(MDDIR)/overrides/partials/source.html: mkdocs/overrides/partials/source.html | $(MDDIR)/overrides/partials
	$(INSTALL_DATA) $< $@

$(MDDIR)/overrides/partials:
	$(MKDIR) $@

$(MDDIR)/overrides/fragments/tags/default:
	$(MKDIR) $@

$(MDDIR)/overrides/fragments/tags/default/listing.html: mkdocs/overrides/fragments/tags/default/listing.html | $(MDDIR)/overrides/fragments/tags/default
	$(INSTALL_DATA) $< $@

build-mkdocs:
	@cd $(MDDIR) ; SITE_NAME="GRASS $(GRASS_VERSION_NUMBER) Reference Manual" \
	COPYRIGHT="&copy; 2003-$(GRASS_VERSION_DATE) GRASS Development Team, GRASS $(GRASS_VERSION_NUMBER) Reference Manual" \
	mkdocs build

serve-mkdocs:
	@cd $(MDDIR) ; SITE_NAME="GRASS $(GRASS_VERSION_NUMBER) Reference Manual" \
	COPYRIGHT="&copy; 2003-$(GRASS_VERSION_DATE) GRASS Development Team, GRASS $(GRASS_VERSION_NUMBER) Reference Manual" \
	mkdocs serve
