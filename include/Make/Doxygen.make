# common dependencies and rules for building libraries
include $(MODULE_TOPDIR)/include/Make/Platform.make

DOXINPUT=$(DOXNAME)lib.dox
DOXOUTPUT=$(DOXNAME)lib

#check for program
checkdoxygen:
	@(type doxygen > /dev/null || (echo "ERROR: Install 'doxygen' software first (get from http://www.doxygen.org)" && exit 1))

# generate docs as single HTML document:
htmldocs-single: checkdoxygen cleandocs
	doxygen $(MODULE_TOPDIR)/include/Make/Doxyfile_arch_html
	@echo "HTML reference in directory ./html/index.html"

# generate docs as multiple HTML documents:
htmldocs: checkdoxygen cleandocs
# hack needed to get main page at beginning:
	@mv $(DOXINPUT) $(DOXINPUT).org
	@sed 's+/\*! \\page +/\*! \\mainpage +g' $(DOXINPUT).org > $(DOXINPUT)
	doxygen $(MODULE_TOPDIR)/include/Make/Doxyfile_arch_html
	@mv $(DOXINPUT).org $(DOXINPUT)
	@echo "HTML reference in directory ./html/index.html"

# NOTE: stubs/ and sqlp/ are excluded in ./Doxyfile_arch_latex
latexdocs: checkdoxygen cleandocs
	test ! -d latex || (cd ./latex && $(MAKE) clean)
# hack needed to get main page at beginning:
	@mv $(DOXINPUT) $(DOXINPUT).org
	@sed 's+/\*! \\page +/\*! \\mainpage +g' $(DOXINPUT).org > $(DOXINPUT)
	doxygen $(MODULE_TOPDIR)/include/Make/Doxyfile_arch_latex
#this hack is needed to make Acroread's search engine happy:
	(cd ./latex ; echo "\usepackage[T1]{fontenc}" >> doxygen.sty)
	(cd ./latex && $(MAKE) )
	@mv $(DOXINPUT).org $(DOXINPUT)
	@echo "Latex reference in directory ./latex/refman.dvi"

pdfdocs: checkdoxygen cleandocs
	test ! -d latex || (cd ./latex && $(MAKE) clean)
# hack needed to get main page at beginning:
	@mv $(DOXINPUT) $(DOXINPUT).org
	@sed 's+/\*! \\page +/\*! \\mainpage +g' $(DOXINPUT).org > $(DOXINPUT)
	doxygen $(MODULE_TOPDIR)/include/Make/Doxyfile_arch_latex
#this hack is needed to make Acroread's search engine happy:
	(cd ./latex ; echo "\usepackage[T1]{fontenc}" >> doxygen.sty)
	(cd ./latex && $(MAKE) refman.pdf && mv refman.pdf grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}$(DOXOUTPUT)_`date '+%Y_%m_%d'`_refman.pdf)
	@mv $(DOXINPUT).org $(DOXINPUT)
	@echo "PDF reference in directory ./latex/grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}$(DOXOUTPUT)_`date '+%Y_%m_%d'`_refman.pdf"

cleandocs:
	rm -rf ./latex ./html
