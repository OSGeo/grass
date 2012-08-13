MODULE_TOPDIR = ..

include $(MODULE_TOPDIR)/include/Make/Other.make

MANPAGES := $(patsubst $(HTMLDIR)/%.html,$(MANDIR)/%.$(MANSECT),$(wildcard $(HTMLDIR)/*.html))

DSTFILES := \
	$(HTMLDIR)/grassdocs.css \
	$(HTMLDIR)/grass_logo.png \
	$(HTMLDIR)/grass_icon.png \
	$(HTMLDIR)/nviz/grassdocs.css \
	$(HTMLDIR)/nviz/grass_logo.png

DSTFILES_REST := \
	$(RESTDIR)/grassdocs.css \
	$(RESTDIR)/grass_logo.png \
	$(RESTDIR)/grass_icon.png \
	$(RESTDIR)/nviz/grassdocs.css \
	$(RESTDIR)/nviz/grass_logo.png \
	$(RESTDIR)/conf.py \
	$(RESTDIR)/make.bat \
	$(RESTDIR)/Makefile \

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

IDXSRC = full_index index $(IDXCATS)

INDICES := $(patsubst %,$(HTMLDIR)/%.html,$(IDXSRC))

INDICES_REST := $(patsubst %,$(RESTDIR)/%.txt,$(IDXSRC))

ALL_HTML := $(wildcard $(HTMLDIR)/*.*.html)

ALL_REST := $(wildcard $(RESTDIR)/*.*.txt)

default: $(DSTFILES)
	@echo "Generating HTML manual pages index (help system)..."
	$(MAKE) $(INDICES)
	$(call build,check)
	$(MAKE) manpages
	$(MAKE) restdocs

restdocs: $(DSTFILES_REST)
	  $(MAKE) $(INDICES_REST)

# This must be a separate target so that evaluation of $(MANPAGES)
# is delayed until the indices have been generated
manpages:
	$(MAKE) $(MANPAGES)

.PHONY: manpages

define build
GISBASE="$(RUN_GISBASE)" ARCH="$(ARCH)" ARCH_DISTDIR="$(ARCH_DISTDIR)" $(PYTHON) ./build_$(1).py $(2)
endef

define build_rest
GISBASE="$(RUN_GISBASE)" ARCH="$(ARCH)" ARCH_DISTDIR="$(ARCH_DISTDIR)" $(PYTHON) ./build_$(1)_rest.py $(2)
endef

$(HTMLDIR)/full_index.html: $(ALL_HTML) build_full_index.py build_html.py
	$(call build,full_index)
	touch $@

$(RESTDIR)/full_index.txt: $(ALL_REST) build_full_index_rest.py build_rest.py
	$(call build_rest,full_index)
	touch $@	
	
$(HTMLDIR)/index.html: build_index.py build_html.py
	$(call build,index)
	touch $@

$(RESTDIR)/index.txt: build_index_rest.py build_rest.py
	$(call build_rest,index)
	touch $@	
	
define category_rule
$$(HTMLDIR)/$(2).html: $$(wildcard $$(HTMLDIR)/$(1).*.html) build_class.py build_html.py
	$$(call build,class,$(1) $(2))
	touch $$@
$$(RESTDIR)/$(2).txt: $$(wildcard $$(RESTDIR)/$(1).*.txt) build_class_rest.py build_rest.py
	$$(call build_rest,class,$(1) $(2))
	touch $$@
endef

$(foreach cat,$(categories),$(eval $(call category_rule,$(firstword $(subst :, ,$(cat))),$(lastword $(subst :, ,$(cat))))))

$(HTMLDIR)/grassdocs.css: grassdocs.css
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/nviz/grassdocs.css: grassdocs.css | $(HTMLDIR)/nviz
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/grass_logo.png: grass_logo.png
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/nviz/grass_logo.png: grass_logo.png | $(HTMLDIR)/nviz
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/grass_icon.png: grass_icon.png
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/nviz:
	$(MKDIR) $@

$(RESTDIR)/grassdocs.css: grassdocs.css
	$(INSTALL_DATA) $< $@

$(RESTDIR)/nviz/grassdocs.css: grassdocs.css | $(RESTDIR)/nviz
	$(INSTALL_DATA) $< $@

$(RESTDIR)/grass_logo.png: grass_logo.png
	$(INSTALL_DATA) $< $@

$(RESTDIR)/nviz/grass_logo.png: grass_logo.png | $(RESTDIR)/nviz
	$(INSTALL_DATA) $< $@

$(RESTDIR)/grass_icon.png: grass_icon.png
	$(INSTALL_DATA) $< $@

$(RESTDIR)/conf.py: sphinx/conf.py
	$(INSTALL_DATA) $< $@

$(RESTDIR)/make.bat: sphinx/make.bat
	$(INSTALL_DATA) $< $@

$(RESTDIR)/Makefile: sphinx/Makefile
	$(INSTALL_DATA) $< $@
	
$(RESTDIR)/nviz:
	$(MKDIR) $@

