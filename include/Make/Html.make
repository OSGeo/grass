
# generic html rules for all commands

include $(MODULE_TOPDIR)/include/Make/HtmlRules.make

$(HTMLDIR)/%.html: %.html %.tmp.html $(HTMLSRC) $(IMGDST) | $(HTMLDIR)
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) MODULE_TOPDIR=$(MODULE_TOPDIR) \
        $(PYTHON) $(GISBASE)/utils/mkhtml.py $* > $@

$(MANDIR)/%.$(MANSECT): $(HTMLDIR)/%.html
	$(HTML2MAN) "$<" "$@"

%.tmp.html: $(HTMLSRC)
	if [ "$(HTMLSRC)" != "" ] ; then $(call htmldesc,$<,$@) ; fi

ifdef CROSS_COMPILING

html:

else

html: $(HTMLDIR)/$(PGM).html $(MANDIR)/$(PGM).$(MANSECT)

endif

.PHONY: html
