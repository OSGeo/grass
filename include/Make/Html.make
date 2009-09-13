
# generic html rules for all commands

ifdef CROSS_COMPILING

html:

else

htmldesc = $(call run_grass,$(1) --html-description < /dev/null | grep -v '</body>\|</html>' > $(2))

$(HTMLDIR)/%.html: %.html %.tmp.html $(HTMLSRC)
	-test -d $(HTMLDIR) || $(MKDIR) $(HTMLDIR)
	$(GISBASE)/tools/mkhtml.sh $* > $@
	-for file in  *.png *.jpg ; do \
		head -n 1 $$file | grep '^\#!' > /dev/null ; \
		if [ $$? -ne 0 ] ; then \
		   $(INSTALL_DATA) $$file $(HTMLDIR) ; \
		fi \
		done 2> /dev/null ; true

$(MANDIR)/%.$(MANSECT): $(HTMLDIR)/%.html
	$(HTML2MAN) $< $@

%.tmp.html: $(HTMLSRC)
	if [ "$(HTMLSRC)" != "" ] ; then $(call htmldesc,$<,$@) ; fi

html: $(HTMLDIR)/$(PGM).html $(MANDIR)/$(PGM).$(MANSECT)

endif

.PHONY: html
