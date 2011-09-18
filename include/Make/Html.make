
# generic html rules for all commands

ifdef CROSS_COMPILING

html:

else

htmldesc = $(call run_grass,$(1) --html-description < /dev/null | grep -v '</body>\|</html>' > $(2))

IMGSRC := $(wildcard *.png) $(wildcard *.jpg)

$(HTMLDIR)/%.html: %.html %.tmp.html $(HTMLSRC) | $(HTMLDIR)
	$(PYTHON) $(GISBASE)/tools/mkhtml.py $* $(GRASS_VERSION_DATE) > $@
ifneq ($(strip $(IMGSRC)),)
	if test -n "$(IMGSRC)" ; then \
		$(MAKE) $(patsubst %,$(HTMLDIR)/%,$(IMGSRC)) ; \
	fi
endif

$(HTMLDIR)/%.png: %.png | $(HTMLDIR)
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/%.jpg: %.jpg | $(HTMLDIR)
	$(INSTALL_DATA) $< $@

$(MANDIR)/%.$(MANSECT): $(HTMLDIR)/%.html
	$(HTML2MAN) $< $@

%.tmp.html: $(HTMLSRC)
	if [ "$(HTMLSRC)" != "" ] ; then $(call htmldesc,$<,$@) ; fi

html: $(HTMLDIR)/$(PGM).html $(MANDIR)/$(PGM).$(MANSECT)

endif

.PHONY: html
