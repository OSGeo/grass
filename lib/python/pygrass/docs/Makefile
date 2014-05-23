# Makefile for Sphinx documentation
#
MODULE_TOPDIR=../../../..
include $(MODULE_TOPDIR)/include/Make/Vars.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

# You can set these variables from the command line.
SPHINXOPTS    =
ifneq (@(type sphinx-build2 > /dev/null),)
SPHINXBUILD   = sphinx-build2
endif
ifneq (@(type sphinx-build > /dev/null),)
SPHINXBUILD   = sphinx-build
endif
PAPER         =
BUILDDIR      = _build
BUILDDIR_HTML = $(HTMLDIR)/pygrass

MYGISBASE = $(GISBASE)

# Internal variables.
PAPEROPT_a4     = -D latex_paper_size=a4
PAPEROPT_letter = -D latex_paper_size=letter
ALLSPHINXOPTS   = -d $(BUILDDIR)/doctrees $(PAPEROPT_$(PAPER)) $(SPHINXOPTS) .
# the i18n builder cannot share the environment and doctrees with the others
I18NSPHINXOPTS  = $(PAPEROPT_$(PAPER)) $(SPHINXOPTS) .

.PHONY: help clean html dirhtml singlehtml pickle json htmlhelp qthelp devhelp epub latex latexpdf text man changes linkcheck doctest gettext

.DEFAULT_GOAL := help

help:
	@echo "Please use \`make <target>' where <target> is one of"
	@echo "  html       to make standalone HTML files"
	@echo "  dirhtml    to make HTML files named index.html in directories"
	@echo "  singlehtml to make a single large HTML file"
	@echo "  epub       to make an epub"
	@echo "  latex      to make LaTeX files, you can set PAPER=a4 or PAPER=letter"
	@echo "  latexpdf   to make LaTeX files and run them through pdflatex"
	@echo "  man        to make manual pages"
	@echo "  doctest    to run all doctests embedded in the documentation (if enabled)"

clean:
	-rm -rf $(BUILDDIR)/*
	-rm -f _templates/layout.html

html:
	$(call run_grass,$(SPHINXBUILD) -b html $(ALLSPHINXOPTS) $(BUILDDIR_HTML))
	@echo
	@echo "Build finished. The HTML pages are in $(BUILDDIR_HTML)"

dirhtml:
	$(call run_grass,$(SPHINXBUILD) -b dirhtml $(ALLSPHINXOPTS) $(BUILDDIR_HTML))
	@echo
	@echo "Build finished. The HTML pages are in $(BUILDDIR_HTML)"

singlehtml:
	$(call run_grass,$(SPHINXBUILD) -b singlehtml $(ALLSPHINXOPTS) $(BUILDDIR_HTML))
	@echo
	@echo "Build finished. The HTML page is in $(BUILDDIR_HTML)"

epub:
	$(call run_grass,$(SPHINXBUILD) -b epub $(ALLSPHINXOPTS) $(BUILDDIR)/epub)
	@echo
	@echo "Build finished. The epub file is in $(BUILDDIR)/epub/"

latex:
	$(call run_grass,$(SPHINXBUILD) -b latex $(ALLSPHINXOPTS) $(BUILDDIR)/latex)
	@echo
	@echo "Build finished; the LaTeX files are in $(BUILDDIR)/latex."
	@echo "Run \`make' in that directory to run these through (pdf)latex" \
	      "(use \`make latexpdf' here to do that automatically)."

latexpdf:
	$(call run_grass,$(SPHINXBUILD) -b latex $(ALLSPHINXOPTS) $(BUILDDIR)/latex)
	@echo "Running LaTeX files through pdflatex..."
	$(MAKE) -C $(BUILDDIR)/latex all-pdf
	@echo "pdflatex finished; the PDF files are in $(BUILDDIR)/latex/"

man:
	$(call run_grass,$(SPHINXBUILD) -b man $(ALLSPHINXOPTS) $(MANDIR))
	@echo
	@echo "Build finished. The manual pages are in $(MANDIR)/"

doctest:
	$(call run_grass,$(SPHINXBUILD) -b doctest $(ALLSPHINXOPTS) $(BUILDDIR)/doctest)
	@echo "Testing of doctests in the sources finished, look at the " \
	      "results in $(BUILDDIR)/doctest/output.txt."
