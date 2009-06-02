MODULE_TOPDIR = ..

include $(MODULE_TOPDIR)/include/Make/Other.make

MANPAGES = $(patsubst $(HTMLDIR)/%.html,$(MANDIR)/%.$(MANSECT),$(wildcard $(HTMLDIR)/*.html))

default: indices
	$(MAKE) $(MANPAGES)

indices:
	cd ../tools && \
	GISBASE="$(GISBASE)" ARCH="$(ARCH)" ARCH_DISTDIR="$(ARCH_DISTDIR)" \
	sh build_html_index.sh
