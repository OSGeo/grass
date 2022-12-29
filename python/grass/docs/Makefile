# Makefile for Sphinx documentation
#
MODULE_TOPDIR=../../..
include $(MODULE_TOPDIR)/include/Make/Vars.make
include $(MODULE_TOPDIR)/include/Make/Rules.make


PAPER         =
BUILDDIR      = _build
BUILDDIR_HTML = $(HTMLDIR)/libpython

MYGISBASE = $(GISBASE)

# Internal variables.
PAPEROPT_a4     = -D latex_paper_size=a4
PAPEROPT_letter = -D latex_paper_size=letter
ALLSPHINXOPTS   = -d $(BUILDDIR)/doctrees $(PAPEROPT_$(PAPER)) $(SPHINXOPTS) -c . src/
# the i18n builder cannot share the environment and doctrees with the others
I18NSPHINXOPTS  = $(PAPEROPT_$(PAPER)) $(SPHINXOPTS) .
SPHINXBUILD     = sphinx-build
SPHINXAPIDOC    = sphinx-apidoc

# some distros come with a different name
BUILD  := $(type $(SPHINXBUILD) >/dev/null)  || (SPHINXBUILD  = sphinx-build2)
APIDOC := $(type $(SPHINXAPIDOC) >/dev/null) || (SPHINXAPIDOC = sphinx-apidoc2)

checksphinx:
	@echo "SPHINXBUILD: Found <$(SPHINXBUILD)>"
	@echo "SPHINXAPIDOC: Found <$(SPHINXAPIDOC)>"
	@(type $(SPHINXBUILD) > /dev/null || (echo "ERROR: Install 'sphinx-build' software first (get from http://sphinx-doc.org)" && exit 1))
	@(type $(SPHINXAPIDOC) > /dev/null || (echo "ERROR: Install 'sphinx-apidoc' software first (get from http://sphinx-doc.org)" && exit 1))

.PHONY: help clean html dirhtml singlehtml pickle json htmlhelp qthelp devhelp checksphinx libpythonapidoc libpythonclean libpythondirhtml libpythondoctest libpythonepub libpythonhelp libpythonhtml libpythonlatex libpythonlatexpdf libpythonman libpythonsinglehtml

.DEFAULT_GOAL := libpythonhelp

libpythonhelp:
	@echo "Please use \`make <target>' where <target> is one of"
	@echo "  libpythonhtml       to make standalone HTML files"
	@echo "  libpythondirhtml    to make HTML files named index.html in directories"
	@echo "  libpythonsinglehtml to make a single large HTML file"
	@echo "  libpythonepub       to make an epub"
	@echo "  libpythonlatex      to make LaTeX files, you can set PAPER=a4 or PAPER=letter"
	@echo "  libpythonlatexpdf   to make LaTeX files and run them through pdflatex"
	@echo "  libpythonman        to make manual pages"
	@echo "  libpythondoctest    to run all doctests embedded in the documentation (if enabled)"

libpythonclean:
	-rm -rf $(BUILDDIR)/*
	-rm -f _templates/layout.html
	-rm -f src/ctypes*.rst
	-rm -f src/exceptions.rst
	-rm -f src/gunittest.*rst
	-rm -f src/imaging.rst
	-rm -f src/pydispatch.rst
	-rm -f src/pygrass.*rst
	-rm -f src/script.rst
	-rm -f src/temporal.rst

libpythonapidoc:
	@echo "SPHINXBUILD: Using <$(SPHINXBUILD)>"
	$(call run_grass,$(SPHINXAPIDOC) -T -f -o src/ ../imaging/)
	$(call run_grass,$(SPHINXAPIDOC) -T -f -o src/ ../exceptions/)
	$(call run_grass,$(SPHINXAPIDOC) -T -f -o src/ ../gunittest/ ../gunittest/multireport.py ../gunittest/multirunner.py ../gunittest/main.py)
	$(call run_grass,$(SPHINXAPIDOC) -T -f -o src/ ../pydispatch/)
	$(call run_grass,$(SPHINXAPIDOC) -T -f -o src/ ../pygrass/)
	$(call run_grass,$(SPHINXAPIDOC) -T -f -o src/ ../script/)
	$(call run_grass,$(SPHINXAPIDOC) -T -f -o src/ ../temporal/)

libpythonhtml: checksphinx
	@echo "SPHINXBUILD: Using <$(SPHINXBUILD)>"
	$(call run_grass,$(SPHINXBUILD) -b html $(ALLSPHINXOPTS) $(BUILDDIR_HTML))
	@echo
	@echo "Build finished. The HTML pages are in $(BUILDDIR_HTML)"

libpythondirhtml:
	@echo "SPHINXBUILD: Using <$(SPHINXBUILD)>"
	$(call run_grass,$(SPHINXBUILD) -b dirhtml $(ALLSPHINXOPTS) $(BUILDDIR_HTML))
	@echo
	@echo "Build finished. The HTML pages are in $(BUILDDIR_HTML)"

libpythonsinglehtml:
	@echo "SPHINXBUILD: Using <$(SPHINXBUILD)>"
	$(call run_grass,$(SPHINXBUILD) -b singlehtml $(ALLSPHINXOPTS) $(BUILDDIR_HTML))
	@echo
	@echo "Build finished. The HTML page is in $(BUILDDIR_HTML)"

libpythonepub:
	@echo "SPHINXBUILD: Using <$(SPHINXBUILD)>"
	$(call run_grass,$(SPHINXBUILD) -b epub $(ALLSPHINXOPTS) $(BUILDDIR)/epub)
	@echo
	@echo "Build finished. The epub file is in $(BUILDDIR)/epub/"

# requires: 
#  texlive-titlesec texlive-framed texlive-threeparttable texlive-wrapfig texlive-upquote texlive-multirow
libpythonlatex:
	@echo "SPHINXBUILD: Using <$(SPHINXBUILD)>"
	$(call run_grass,$(SPHINXBUILD) -b latex $(ALLSPHINXOPTS) $(BUILDDIR)/latex)
	@echo
	@echo "Build finished; the LaTeX files are in $(BUILDDIR)/latex."
	@echo "Run \`make' in that directory to run these through (pdf)latex" \
	      "(use \`make latexpdf' here to do that automatically)."

libpythonlatexpdf:
	@echo "SPHINXBUILD: Using <$(SPHINXBUILD)>"
	$(call run_grass,$(SPHINXBUILD) -b latex $(ALLSPHINXOPTS) $(BUILDDIR)/latex)
	@echo "Running LaTeX files through pdflatex..."
	$(MAKE) -C $(BUILDDIR)/latex all-pdf
	@echo "pdflatex finished; the PDF files are in $(BUILDDIR)/latex/"

libpythonman:
	@echo "SPHINXBUILD: Using <$(SPHINXBUILD)>"
	$(call run_grass,$(SPHINXBUILD) -b man $(ALLSPHINXOPTS) $(MANDIR))
	@echo
	@echo "Build finished. The manual pages are in $(MANDIR)/"

libpythondoctest:
	@echo "SPHINXBUILD: Using <$(SPHINXBUILD)>"
	$(call run_grass,$(SPHINXBUILD) -b doctest $(ALLSPHINXOPTS) $(BUILDDIR)/doctest)
	@echo "Testing of doctests in the sources finished, look at the " \
	      "results in $(BUILDDIR)/doctest/output.txt."
