ifneq (@(type sphinx-apidoc2 > /dev/null),)
SPHINXAPIDOC   = sphinx-apidoc2
endif
ifneq (@(type sphinx-apidoc > /dev/null),)
SPHINXAPIDOC   = sphinx-apidoc
endif

ifneq (@(type sphinx-build2 > /dev/null),)
SPHINXBUILD   = sphinx-build2
endif
ifneq (@(type sphinx-build > /dev/null),)
SPHINXBUILD   = sphinx-build
endif

checksphinx:
	@(type $(SPHINXBUILD) > /dev/null || (echo "ERROR: Install 'sphinx-build' software first (get from http://sphinx-doc.org)" && exit 1))
	@(type $(SPHINXAPIDOC) > /dev/null || (echo "ERROR: Install 'sphinx-apidoc' software first (get from http://sphinx-doc.org)" && exit 1))

cleansphinx:
	$(MAKE) -C $(MODULE_TOPDIR)/gui/wxpython/docs/wxgui_sphinx/ wxguiclean
	$(MAKE) -C $(MODULE_TOPDIR)/lib/python/docs libpythonclean

sphinxdoc: checksphinx cleansphinx
	$(MAKE) -C $(MODULE_TOPDIR)/gui/wxpython/docs/wxgui_sphinx/ wxguiapidoc
	$(MAKE) -C $(MODULE_TOPDIR)/gui/wxpython/docs/wxgui_sphinx/ wxguihtml
	$(MAKE) -C $(MODULE_TOPDIR)/lib/python/docs/ libpythonapidoc
	$(MAKE) -C $(MODULE_TOPDIR)/lib/python/docs/ libpythonhtml

sphinxman:checksphinx cleansphinx
	$(MAKE) -C $(MODULE_TOPDIR)/gui/wxpython/docs/wxgui_sphinx/ wxguiapidoc
	$(MAKE) -C $(MODULE_TOPDIR)/gui/wxpython/docs/wxgui_sphinx/ wxguiman
	$(MAKE) -C $(MODULE_TOPDIR)/lib/python/docs/ libpythonapidoc
	$(MAKE) -C $(MODULE_TOPDIR)/lib/python/docs/ libpythonman

