APIDOC := $(shell sphinx-apidoc2 --help 2>/dev/null)
ifdef APIDOC
SPHINXAPIDOC = sphinx-apidoc2
else
	APIDOC := $(shell sphinx-apidoc --help 2>/dev/null)
	ifdef APIDOC
		SPHINXAPIDOC = sphinx-apidoc
	endif
endif

BUILD := $(shell sphinx-build2 --version 2>/dev/null)
ifdef BUILD
	SPHINXBUILD = sphinx-build2
else
	BUILD := $(shell sphinx-build --version 2>/dev/null)
	ifdef BUILD
		SPHINXBUILD = sphinx-build
	endif
endif

checksphinx:
	@echo $(SPHINXBUILD)
	@echo $(SPHINXAPIDOC)
	@(type $(SPHINXBUILD) > /dev/null || (echo "ERROR: Install 'sphinx-build' software first (get from http://sphinx-doc.org)" && exit 1))
	@(type $(SPHINXAPIDOC) > /dev/null || (echo "ERROR: Install 'sphinx-apidoc' software first (get from http://sphinx-doc.org)" && exit 1))

DOCSDIR = $(MODULE_TOPDIR)/python/grass/docs/

cleansphinx:
	$(MAKE) -C $(MODULE_TOPDIR)/gui/wxpython/docs/wxgui_sphinx/ wxguiclean
	$(MAKE) -C $(DOCSDIR) libpythonclean

sphinxdoc: checksphinx cleansphinx
	$(MAKE) -C $(MODULE_TOPDIR)/gui/wxpython/docs/wxgui_sphinx/ wxguiapidoc
	$(MAKE) -C $(MODULE_TOPDIR)/gui/wxpython/docs/wxgui_sphinx/ wxguihtml
	$(MAKE) -C $(DOCSDIR) libpythonapidoc
	$(MAKE) -C $(DOCSDIR) libpythonhtml

sphinxman:checksphinx cleansphinx
	$(MAKE) -C $(MODULE_TOPDIR)/gui/wxpython/docs/wxgui_sphinx/ wxguiapidoc
	$(MAKE) -C $(MODULE_TOPDIR)/gui/wxpython/docs/wxgui_sphinx/ wxguiman
	$(MAKE) -C $(DOCSDIR) libpythonapidoc
	$(MAKE) -C $(DOCSDIR) libpythonman

cleansphinxlib:
	$(MAKE) -C $(DOCSDIR) libpythonclean

sphinxdoclib: checksphinx cleansphinxlib
	$(MAKE) -C $(DOCSDIR) libpythonapidoc
	$(MAKE) -C $(DOCSDIR) libpythonhtml

sphinxmanlib:checksphinx cleansphinxlib
	$(MAKE) -C $(DOCSDIR) libpythonapidoc
	$(MAKE) -C $(DOCSDIR) libpythonman
