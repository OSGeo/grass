
# generic html rules for all commands

include $(MODULE_TOPDIR)/include/Make/HtmlRules.make

$(HTMLDIR)/%.html: %.html %.tmp.html $(HTMLSRC) $(IMGDST) | $(HTMLDIR)
	$(PYTHON) $(GISBASE)/tools/mkhtml.py $* $(GRASS_VERSION_DATE) > $@

$(MANDIR)/%.$(MANSECT): $(HTMLDIR)/%.html
	$(HTML2MAN) $< $@

%.tmp.html: $(HTMLSRC)
	if [ "$(HTMLSRC)" != "" ] ; then $(call htmldesc,$<,$@) ; fi

ifdef CROSS_COMPILING

html:

else

html: $(HTMLDIR)/$(PGM).html $(MANDIR)/$(PGM).$(MANSECT)

endif

.PHONY: html
