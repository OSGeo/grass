
# This should be "include"d from the top-level Makefile, and nowhere else

# Extra commands
HTML2PDF=		htmldoc --footer d.1
GRASS_PDFDIR=		$(DOCSDIR)/pdf

# generate programmer's manual as single HTML document:

htmldocs-single:
	for dir in lib rfc gui/wxpython ; do \
	  $(MAKE) -C $$dir htmldocs-single ; \
	done

# generate programmer's manual as multiple HTML documents:
htmldocs:

docs_dirs = \
	lib/db \
	lib/g3d \
	lib/gis \
	lib/gmath \
	lib/gpde \
	lib/proj \
	lib/python \
	lib/ogsf \
	lib/segment \
	lib/vector \
	lib/vector/dglib \
	gui/wxpython \
	rfc

htmldocs_dirs := $(patsubst %,%/html,$(docs_dirs))
latexdocs_dirs := $(patsubst %,%/latex,$(docs_dirs))

htmldocs:
	for dir in $(docs_dirs) ; do \
	  $(MAKE) -C $$dir htmldocs ; \
	  done

packagehtmldocs: htmldocs
	tar chvfz $(GRASS_NAME)refman_$(DATE)_html.tar.gz $(htmldocs_dirs)

#alternatively, the docs can be generated as single PDF document (see doxygen FAQ for 'TeX capacity exceeded'):
#  (cd lib/ ; make pdfdocs)

pdfdocs:
	for dir in $(docs_dirs) ; do \
	  $(MAKE) -C $$dir pdfdocs ; \
	  done
	@echo "Written PDF docs in: $(latexdocs_dirs)"

cleandocs:
	for dir in $(docs_dirs) ; do \
	  $(MAKE) -C $$dir cleandocs ; \
	  done

indices = \
	database.html \
	display.html \
	general.html \
	imagery.html \
	misc.html \
	photo.html \
	postscript.html \
	raster.html \
	raster3D.html \
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
	$(call html_pdf misc,m.*.html)
	$(call html_pdf photo,i.ortho*.html photo*.html)
	$(call html_pdf postscript,ps.*.html)
	$(call html_pdf raster,r.*.html)
	$(call html_pdf raster3d,r3.*.html)
	$(call html_pdf vector,v.*.html)

changelog:
	@ echo "creating ChangeLog file (following 'trunk' only)..."
	@ # svn2cl creates a GNU style ChangeLog file:
	@ # http://ch.tudelft.nl/~arthur/svn2cl/
	@if [ ! -x "`which svn2cl`" ] ; then \
		echo "\"svn2cl\" is required, please install first from http://ch.tudelft.nl/~arthur/svn2cl/" ;	exit 1 ; \
	fi
	sh svn2cl ./ChangeLog

.PHONY: htmldocs-single htmldocs packagehtmldocs pdfdocs cleandocs html2pdfdoc
.PHONY: html2pdfdoccomplete changelog
