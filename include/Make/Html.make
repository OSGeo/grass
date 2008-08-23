
# generic html rules for all commands

ifdef CROSS_COMPILING

html:

else

htmldesc = \
	GISRC=$(RUN_GISRC) \
	GISBASE=$(RUN_GISBASE) \
	PATH="$(BIN):$$PATH" \
	PYTHONPATH="$(GISBASE)/etc/python:$$PYTHONPATH" \
	$(LD_LIBRARY_PATH_VAR)="$(BIN):$(ARCH_LIBDIR):$($(LD_LIBRARY_PATH_VAR))" \
	LC_ALL=C \
	$(1) --html-description < /dev/null | grep -v '</body>\|</html>' > $(2)

$(HTMLDIR)/%.html: %.html %.tmp.html $(HTMLSRC)
	-test -d $(HTMLDIR) || $(MKDIR) $(HTMLDIR)
	$(MODULE_TOPDIR)/tools/mkhtml.sh $* > $@
	-for file in  *.png *.jpg ; do \
		head -n 1 $$file | grep '^\#!' > /dev/null ; \
		if [ $$? -ne 0 ] ; then \
		   $(INSTALL_DATA) $$file $(HTMLDIR) ; \
		fi \
		done 2> /dev/null ; true

%.tmp.html: $(HTMLSRC)
	if [ "$(HTMLSRC)" != "" ] ; then $(call htmldesc,$<,$@) ; fi

html: $(HTMLDIR)/$(PGM).html

endif

.PHONY: html
