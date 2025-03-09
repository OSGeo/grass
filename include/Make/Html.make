
# generic html rules for all commands

include $(MODULE_TOPDIR)/include/Make/HtmlRules.make

$(HTMLDIR)/%.html: %.html %.tmp.html $(HTMLSRC) $(IMGDST) | $(HTMLDIR)
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) MODULE_TOPDIR=$(MODULE_TOPDIR) \
        $(PYTHON) $(GISBASE)/utils/mkhtml.py $* > $@

$(MDDIR)/source/%.md: %.md %.tmp.md $(HTMLSRC) $(IMGDST_MD) | $(MDDIR)
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) MODULE_TOPDIR=$(MODULE_TOPDIR) \
        $(PYTHON) $(GISBASE)/utils/mkmarkdown.py $* > $@

$(MANDIR)/%.$(MANSECT): $(HTMLDIR)/%.html
	$(HTML2MAN) "$<" "$@"

%.tmp.html: $(HTMLSRC)
	if [ "$(HTMLSRC)" != "" ] ; then $(call htmldesc,$<,$@) ; fi

%.tmp.md: $(HTMLSRC)
	if [ "$(HTMLSRC)" != "" ] ; then $(call mddesc,$<,$@) ; fi

ifdef CROSS_COMPILING

html:

else

html: $(HTMLDIR)/$(PGM).html $(MANDIR)/$(PGM).$(MANSECT) $(MDDIR)/source/$(PGM).md

endif

.PHONY: html
