#############################################################################
#
# MODULE:   	GRASS Compilation
# AUTHOR(S):	Original author unknown - probably CERL
#   	    	Justin Hickey - Thailand - jhickey AT hpcc.nectec.or.th
#   	    	Markus Neteler - Germany - neteler AT itc.it
#   	    	Andreas Lange - Germany - Andreas.Lange AT Rhein-Main.de
#   	    	Radim Blazek - Italy - blazek AT itc.it
# PURPOSE:  	It provides the commands necessary to compile, install,
#   	    	clean, and uninstall GRASS
#   	    	See INSTALL file for explanations.
# COPYRIGHT:    (C) 2002-2012 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#   	    	License (>=v2). Read the file COPYING that comes with GRASS
#   	    	for details.
#
#############################################################################

MODULE_TOPDIR = .

include $(MODULE_TOPDIR)/include/Make/Dir.make
include $(MODULE_TOPDIR)/include/Make/Compile.make

DATE := $(shell date '+%d_%m_%Y')

DIRS = \
	demolocation \
	tools \
	include \
	lib \
	db \
	display \
	general \
	raster \
	raster3d \
	vector \
	misc \
	imagery \
	ps \
	scripts \
	temporal \
	doc \
	gui \
	visualization \
	locale \
	man \
	macosx \
	mswindows

SUBDIRS = $(DIRS)

FILES = AUTHORS COPYING CHANGES REQUIREMENTS.html GPL.TXT contributors.csv contributors_extra.csv translators.csv
FILES_DST = $(patsubst %,$(ARCH_DISTDIR)/%,$(FILES))

default:
	@echo "GRASS GIS $(GRASS_VERSION_MAJOR).$(GRASS_VERSION_MINOR).$(GRASS_VERSION_RELEASE) $(GRASS_VERSION_SVN) compilation log" \
		> $(ERRORLOG)
	@echo "--------------------------------------------------" >> $(ERRORLOG)
	@echo "Started compilation: `date`"                        >> $(ERRORLOG)
	@echo "--"                                                 >> $(ERRORLOG)
	@echo "Errors in:"                                         >> $(ERRORLOG)
	-$(CHMOD) 755 install-sh
	$(MAKE) subdirs
	$(MAKE) $(FILES_DST)
	$(MAKE) manifests
	@if [ `wc -l < "$(ERRORLOG)"` -gt 5 ] ; then \
		echo "--"     >> $(ERRORLOG) ; \
		echo "In case of errors please change into the directory with error and run 'make'." >> $(ERRORLOG) ; \
		echo "If you get multiple errors, you need to deal with them in the order they"      >> $(ERRORLOG) ; \
		echo "appear in the error log. If you get an error building a library, you will"     >> $(ERRORLOG) ; \
		echo "also get errors from anything which uses the library."  >> $(ERRORLOG) ; \
	else \
		echo "No errors detected." >> $(ERRORLOG) ; \
	fi
	@echo "--"  >> $(ERRORLOG)
	@echo "Finished compilation: `date`" >> $(ERRORLOG)
	@cat $(ERRORLOG)
	@if [ `wc -l < "$(ERRORLOG)"` -gt 8 ] ; then false ; else true ; fi

manifests:
ifeq ($(MANIFEST),external)
	find $(ARCH_DISTDIR) -type f -name '*.exe' | \
	while read file ; do \
	    $(MAKE) "$$file".manifest ; \
	done
endif

$(ARCH_DISTDIR)/%: %
	$(INSTALL_DATA) $< $@

LIBDIRS = \
	lib/external/shapelib \
	lib/datetime \
	lib/gis \
	lib/linkm \
	lib/db \
	lib/form \
	lib/vector \
	db/drivers

# Compile libraries only
libs:
	$(MAKE) -C include
	$(MAKE) subdirs SUBDIRS=$(LIBDIRS)
	$(MAKE) $(FILES_DST)

cleandistdirs: 
	-rm -rf $(ARCH_DISTDIR)
	-rm -rf $(ARCH_BINDIR)

# Clean out the strings extracted from scripts for translation
cleanscriptstrings:
	rm -f locale/scriptstrings/*.c 2>/dev/null

clean: cleandistdirs cleanscriptstrings cleandocs

libsclean: cleandistdirs
	$(MAKE) clean-recursive SUBDIRS=$(LIBDIRS)

distclean: clean
	-rm -f config.cache config.log config.status config.status.$(ARCH) 2>/dev/null
	-rm -f ChangeLog ChangeLog.bak $(ERRORLOG) grass.pc
	-rm -f include/config.h include/version.h
	-rm -f include/Make/Platform.make include/Make/Doxyfile_arch_html include/Make/Doxyfile_arch_latex 2>/dev/null

include $(MODULE_TOPDIR)/include/Make/Install.make
include $(MODULE_TOPDIR)/include/Make/Docs.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

DOXNAME=grass

.PHONY: default libs
.PHONY: cleandistdirs cleanscriptstrings clean libsclean distclean
