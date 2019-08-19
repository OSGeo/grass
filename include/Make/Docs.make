
# This should be "include"d from the top-level Makefile, and nowhere else

# Extra commands
HTML2PDF=		htmldoc --footer d.1
GRASS_PDFDIR=		$(DOCSDIR)/pdf

# generate programmer's manual as single HTML document:

htmldocs-single:
	$(MAKE) -C . htmldox-single
	for dir in lib ; do \
	  $(MAKE) -C $$dir htmldox-single ; \
	done

# generate programmer's manual as multiple HTML documents:
docs_dirs = \
	lib/db \
	lib/raster3d \
	lib/gis \
	lib/gmath \
	lib/gpde \
	lib/proj \
	lib/ogsf \
	lib/segment \
	lib/vector \
	lib/vector/dglib

htmldocs_dirs := $(patsubst %,%/html,$(docs_dirs))
latexdocs_dirs := $(patsubst %,%/latex,$(docs_dirs))

htmldocs:
	$(MAKE) -C . htmldox
	for dir in $(docs_dirs) ; do \
	  $(MAKE) -C $$dir htmldox ; \
	  done

packagehtmldocs: htmldocs
	tar chvfz $(GRASS_NAME)refman_$(DATE)_html.tar.gz $(htmldocs_dirs)

#alternatively, the docs can be generated as single PDF document (see doxygen FAQ for 'TeX capacity exceeded'):
#  (cd lib/ ; make pdfdox)

pdfdocs:
	for dir in $(docs_dirs) ; do \
	  $(MAKE) -C $$dir pdfdox ; \
	  done
	@echo "Written PDF docs in: $(latexdocs_dirs)"

cleandocs:
	$(MAKE) -C . cleandox
	for dir in $(docs_dirs) ; do \
	  $(MAKE) -C $$dir cleandox ; \
	  done

indices = \
	database.html \
	display.html \
	general.html \
	imagery.html \
	miscellaneous.html \
	photo.html \
	postscript.html \
	raster.html \
	raster3d.html \
	vector.html

html_pdf = \
	cd $(ARCH_DISTDIR)/docs/html && \
	$(HTML2PDF) --webpage $(1).html $(2).*.html -f $(GRASS_PDFDIR)/$(GRASS_NAME)$(1).pdf

html2pdfdoc:
	@ echo "Light PDF document from modules' HTML documentation"
	@ # http://www.htmldoc.org
	@test -d $(GRASS_PDFDIR) || mkdir -p $(GRASS_PDFDIR)
	$(call html_pdf commands,--no-links $(indices))

html2pdfdoccomplete:
	@ echo "Complete PDF document from modules' HTML documentation"
	@ # http://www.htmldoc.org
	@test -d $(GRASS_PDFDIR) || mkdir -p $(GRASS_PDFDIR)
	$(call html_pdf database,db.*.html)
	$(call html_pdf display,d.*.html)
	$(call html_pdf general,g.*.html)
	$(call html_pdf imagery,i.*.html)
	$(call html_pdf miscellaneous,m.*.html)
	$(call html_pdf photo,i.ortho*.html photo*.html)
	$(call html_pdf postscript,ps.*.html)
	$(call html_pdf raster,r.*.html)
	$(call html_pdf raster3d,r3.*.html)
	$(call html_pdf vector,v.*.html)

changelog:
	@ echo "creating ChangeLog file (following 'releasebranch_7_8' only)..."
	@ # tools/gitlog2changelog.py creates a GNU style ChangeLog file:
	python tools/gitlog2changelog.py

.PHONY: htmldocs-single htmldocs packagehtmldocs pdfdocs cleandocs html2pdfdoc
.PHONY: html2pdfdoccomplete changelog
