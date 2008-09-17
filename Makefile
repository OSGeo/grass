#############################################################################
#
# MODULE:   	GRASS Compilation
# AUTHOR(S):	Original author unknown - probably CERL
#   	    	Justin Hickey - Thailand - jhickey AT hpcc.nectec.or.th
#		Markus Neteler - Germany - neteler AT itc.it
#		Andreas Lange - Germany - Andreas.Lange AT Rhein-Main.de
#		Radim Blazek - Italy - blazek AT itc.it
# PURPOSE:  	It provides the commands necessary to compile, install,
#		clean, and uninstall GRASS
#		See INSTALL file for explanations.
# COPYRIGHT:    (C) 2002,2004 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#   	    	License (>=v2). Read the file COPYING that comes with GRASS
#   	    	for details.
#
#############################################################################

MODULE_TOPDIR = .

include $(MODULE_TOPDIR)/include/Make/Vars.make

# Install directories
exec_prefix=            ${prefix}
BINDIR=			${UNIX_BIN}

# Shell commands
MAKE_DIR_CMD=		mkdir -p -m 755

# Extra commands
HTML2PDF=		htmldoc --footer d.1
GRASS_PDFDIR=		$(DOCSDIR)/pdf


DIRS = \
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
	sites \
	scripts \
	doc \
	gui \
	visualization \
	tools \
	gem \
	man \
	locale \
	macosx

SUBDIRS = $(DIRS)

FILES = AUTHORS COPYING CHANGES REQUIREMENTS.html GPL.TXT

BIN_DIST_FILES = $(FILES) \
	grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}.tmp \
	bin \
	bwidget \
	docs \
	driver \
	etc \
	fonts \
	include \
	lib \
	man \
	scripts

default: builddemolocation
	@echo "GRASS GIS compilation log"     > $(ERRORLOG)
	@echo "-------------------------"    >> $(ERRORLOG)
	@echo "Started compilation: `date`"  >> $(ERRORLOG)
	@echo "--"                           >> $(ERRORLOG)
	@echo "Errors in:"                   >> $(ERRORLOG)
	chmod 755 install-sh
	@list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) -C $$subdir; \
	done
	-cp -f $(FILES) ${ARCH_DISTDIR}/
	-cp -f ${ARCH_BINDIR}/grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR} ${ARCH_DISTDIR}/grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}.tmp
	@(cd tools ; sh -c "./build_html_index.sh")
	@if [ `cat "$(ERRORLOG)" | wc -l` -gt 5 ] ; then \
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
	@if [ `cat "$(ERRORLOG)" | wc -l` -gt 8 ] ; then false ; else true ; fi

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
	make -C lib/ headers
	@list='$(LIBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) -C $$subdir; \
	done
	-cp -f $(FILES) ${ARCH_DISTDIR}/
	-cp -fr --parents include ${ARCH_DISTDIR}/

cleandistdirs: 
	-rm -rf ${ARCH_DISTDIR}
	-rm -rf ${ARCH_BINDIR}

# Clean out the strings extracted from scripts for translation
cleanscriptstrings:
	rm -f locale/scriptstrings/*.c 2>/dev/null

clean: cleandistdirs cleanscriptstrings
	@list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) -C $$subdir clean; \
	done

libsclean: cleandistdirs
	@list='$(LIBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) -C $$subdir clean; \
	done

distclean: clean
	-rm -f config.cache config.log config.status config.status.${ARCH} 2>/dev/null
	-rm -f ChangeLog ChangeLog.bak $(ERRORLOG) grass.pc
	-rm -f include/config.h include/version.h include/Make/Platform.make 2>/dev/null
	-rm -f swig/perl/Makefile.PL swig/perl2/make.pl 2>/dev/null

strip:
	@ if [ ! -f ${ARCH_BINDIR}/grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR} ] ; then \
		echo "ERROR: GRASS has not been compiled. Try \"make\" first."; \
		echo "  Strip aborted, exiting Make."; \
		exit; \
	fi; \
	cd ${ARCH_DISTDIR} ; find . -type f -perm +111 -exec strip {} \;

install:
	@ # The following action MUST be a single action. That is, all lines
	@ # except the last line must have a backslash (\) at the end to
	@ # continue the statement. The reason for this is that Make does not
	@ # have an exit command thus, exit terminates the shell. However, 
	@ # Make creates a new shell for each action listed for a target.
	@ # Therefore, the only way exit will quit Make is if there is only
	@ # a single action for the target.
	@ # Check if grass has been compiled, if INST_DIR is writable, and if
	@ # grass is part of INST_DIR
	echo ${ARCH_BINDIR}/grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}
	@ if [ ! -f ${ARCH_BINDIR}/grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR} ] ; then \
		echo "ERROR: GRASS has not been compiled. Try \"make\" first."; \
		echo "  Installation aborted, exiting Make."; \
		exit; \
	fi; \
	if [ "${MACOSX_APP}" = "1" ] ; then \
		${MAKE} install-macosx; \
		exit; \
	fi; \
	INST_PATH=`dirname ${INST_DIR}`; \
	while [ ! -d $$INST_PATH ]; do \
		INST_PATH=`dirname $$INST_PATH`; \
	done; \
	if [ ! -d "${INST_DIR}" -a ! -w "$$INST_PATH" ] ; then \
		echo "ERROR: Directory $$INST_PATH is a parent directory of your"; \
		echo "  install directory ${INST_DIR} and is not writable."; \
		echo "  Perhaps you need root access."; \
		echo "  Installation aborted, exiting Make."; \
		exit; \
	fi; \
	if [ -d ${INST_DIR} -a ! -w "${INST_DIR}" ] ; then \
		echo "ERROR: Your install directory ${INST_DIR} is not writable."; \
		echo "  Perhaps you need root access."; \
		echo "  Installation aborted, exiting Make."; \
		exit; \
	fi; \
	result=`echo "${INST_DIR}" | awk '{ if ($$1 ~ /grass/) print $$1 }'`; \
	if [ "$$result" = "" ] ; then \
		echo "WARNING: Your install directory ${INST_DIR}"; \
		echo "  does not contain the word 'grass'."; \
		echo "  It is highly recommended that the word 'grass' be part"; \
		echo "  of your install directory to avoid conflicts."; \
		echo "  Do you want to continue? [y/n]"; \
		read ans; \
		ans=`echo "$$ans" | tr A-Z a-z`; \
		if [ "$$ans" != "y" ] ; then \
			echo "Installation aborted, exiting Make."; \
			exit; \
		fi; \
	fi; \
	${MAKE} real-install

real-install:
	test -d ${INST_DIR} || ${MAKE_DIR_CMD} ${INST_DIR}
	test -d ${BINDIR} || ${MAKE_DIR_CMD} ${BINDIR}
	-sed -e "s#^GISBASE.*#GISBASE=${INST_DIR}#" ${ARCH_BINDIR}/grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR} > ${BINDIR}/grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}
	-chmod a+x ${BINDIR}/grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}
ifneq ($(strip $(MINGW)),)
	-sed -e "s#WINGISBASE=.*#WINGISBASE=${INST_DIR}#" ${ARCH_BINDIR}/grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}.bat > ${BINDIR}/grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}.bat
	-chmod a+x ${BINDIR}/grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}.bat
endif
	-tar cBCf ${GISBASE} - . | tar xBCf ${INST_DIR} - 2>/dev/null
	-sed 's#'${GISBASE}'#'${INST_DIR}'#g' ${GISBASE}/etc/fontcap > ${INST_DIR}/etc/fontcap
	-chmod -R a+rX ${INST_DIR} 2>/dev/null
	@#GEM installation
	-tar cBf - gem/skeleton | tar xBCf ${INST_DIR}/etc - 2>/dev/null
	-${INSTALL} gem/gem7 ${BINDIR} 2>/dev/null
	@# enable OSX Help Viewer
	@if [ "`cat include/Make/Platform.make | grep -i '^ARCH.*darwin'`" ] ; then /bin/ln -sfh "${INST_DIR}/docs/html" /Library/Documentation/Help/GRASS-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR} ; fi


install-strip:
	${MAKE} strip
	${MAKE} install

install-macosx:
	${MAKE} -C macosx install-macosx

bindist:  
	if [ "${MACOSX_APP}" = "1" ] ; then \
		${MAKE} bindist-macosx; \
		exit; \
	fi; \
	${MAKE} real-bindist

real-bindist:
	mkdir -p ${ARCH_DISTDIR}/etc/nad/src ; \
	    cp -f ${MODULE_TOPDIR}/lib/proj/*.lla ${ARCH_DISTDIR}/etc/nad/src ; true
	( date=`date '+%d_%m_%Y'`; cd ${ARCH_DISTDIR}; tar cBf - ${BIN_DIST_FILES} | gzip -fc > ../grass-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}-${ARCH}-$$date.tar.gz)
	-date=`date '+%d_%m_%Y'`; name=grass-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}-${ARCH}-$$date.tar.gz; \
	    size=`ls -l $$name | awk '{print $$5}'`; \
	    sed -e "s/BIN_DIST_VERSION/${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}-${ARCH}-$$date/" \
	    -e "s/GRASSPRG_NAME/grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}/" \
	    -e "s/SIZE_TAR_FILE/$$size/" -e "s#BIN_DIST_DIR#'${INST_DIR}'#" \
	    -e "s/ARCHITECTURE/${ARCH}/" \
	    -e "s/LD_LIBRARY_PATH_VAR/${LD_LIBRARY_PATH_VAR}/" \
	    -e "s/TEST_STR=/TEST_STR=executable/" \
	    -e "s#IMPORTANT.*#Generated from the binaryInstall.src file using the command make bindist#" \
	    -e "s/# executable shell.*//" -e "s/# make bindist.*//" \
	    binaryInstall.src > grass-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}-${ARCH}-$$date-install.sh ; \
	    chmod a+x grass-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}-${ARCH}-$$date-install.sh 2>/dev/null

bindist-macosx:
	${MAKE} -C macosx bindist-macosx

# make a source package for distribution:
srcdist: distclean
	-${MAKE_DIR_CMD} ./grass-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}

	@ # needed to store code in package with grass-version path:
	-mv * ./grass-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}
	@ # do not include the debian control files:
	-mv ./grass-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}/debian .
	@ #we use -h to get the linked files into as real files:
	tar cvfzh grass-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}.tar.gz ./grass-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}/* --exclude=CVS
	@ # restore src code location:
	-mv ./grass-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}/* .
	-rmdir ./grass-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}
	@ echo "Distribution source package: grass-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}.tar.gz ready. Calculating MD5 sum..."
	md5sum grass-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}.tar.gz > grass-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}.md5sum

# make a source package for library distribution:
srclibsdist: distclean
	-${MAKE_DIR_CMD} ./grass-lib-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}

	@ # needed to store code in package with grass-version path:
	-cp -L * ./grass-lib-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}
	-cp -rL tools ./grass-lib-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}
	-cp -rL include ./grass-lib-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}
	-cp -rL --parents lib/external/shapelib ./grass-lib-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}
	-cp -rL --parents lib/datetime ./grass-lib-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}
	-cp -rL --parents lib/db ./grass-lib-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}
	-cp -rL --parents lib/gis ./grass-lib-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}
	-cp -rL --parents lib/linkm ./grass-lib-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}
	-cp -rL --parents lib/form ./grass-lib-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}
	-cp -rL --parents lib/vector ./grass-lib-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}

	-cp -rL --parents db/drivers ./grass-lib-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}

	tar chvfz grass-lib-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}.tar.gz ./grass-lib-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}/* --exclude=CVS
	-rm -r ./grass-lib-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}
	@ echo "Distribution source package: grass-lib-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}.tar.gz ready."

# generate docs as single HTML document:
htmldocs-single:
	(cd lib/ ; $(MAKE) cleandocs ; $(MAKE) htmldocs-single)
	(cd rfc/ ; $(MAKE) cleandocs ; $(MAKE) htmldocs-single)
	(cd swig/; $(MAKE) cleandocs ; $(MAKE) htmldocs-single)

# generate docs as multiple HTML documents:
htmldocs:
	(cd lib/db/ ; $(MAKE) cleandocs ; $(MAKE) htmldocs)
	(cd lib/g3d/ ; $(MAKE) cleandocs ; $(MAKE) htmldocs)
	(cd lib/gis/ ; $(MAKE) cleandocs ; $(MAKE) htmldocs)
	(cd lib/gmath/ ; $(MAKE) cleandocs ; $(MAKE) htmldocs)
	(cd lib/gpde/ ; $(MAKE) cleandocs ; $(MAKE) htmldocs)
	(cd lib/ogsf/ ; $(MAKE) cleandocs ; $(MAKE) htmldocs)
	(cd lib/proj/ ; $(MAKE) cleandocs ; $(MAKE) htmldocs)
	(cd lib/segment/; $(MAKE) cleandocs ; $(MAKE) htmldocs)
	(cd lib/vector/ ; $(MAKE) cleandocs ; $(MAKE) htmldocs)
	(cd lib/vector/dglib/ ; $(MAKE) cleandocs ; $(MAKE) htmldocs)
	(cd rfc/ ; $(MAKE) cleandocs ; $(MAKE) htmldocs)
	(cd swig/; $(MAKE) cleandocs ; $(MAKE) htmldocs)

packagehtmldocs: htmldocs
	tar chvfz grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}refman_`date '+%Y_%m_%d'`_html.tar.gz lib/db/html lib/g3d/html lib/gis/html lib/gmath/html lib/gpde/html lib/proj/html lib/ogsf/html lib/segment/html lib/vector/html lib/vector/dglib/html rfc/html swig/html

#alternatively, the docs can be generated as single PDF document (see doxygen FAQ for 'TeX capacity exceeded'):
#  (cd lib/ ; make pdfdocs)

pdfdocs:
	(cd lib/db/ ; $(MAKE) cleandocs ; $(MAKE) pdfdocs)
	(cd lib/g3d/ ; $(MAKE) cleandocs ; $(MAKE) pdfdocs)
	(cd lib/gis/ ; $(MAKE) cleandocs ; $(MAKE) pdfdocs)
	(cd lib/gmath/ ; $(MAKE) cleandocs ; $(MAKE) pdfdocs)
	(cd lib/gpde/ ; $(MAKE) cleandocs ; $(MAKE) pdfdocs)
	(cd lib/ogsf/ ; $(MAKE) cleandocs ; $(MAKE) pdfdocs)
	(cd lib/proj/ ; $(MAKE) cleandocs ; $(MAKE) pdfdocs)
	(cd lib/segment/; $(MAKE) cleandocs ; $(MAKE) pdfdocs)
	(cd lib/vector/ ; $(MAKE) cleandocs ; $(MAKE) pdfdocs)
	(cd lib/vector/dglib/ ; $(MAKE) cleandocs ; $(MAKE) pdfdocs)
	(cd rfc/ ; $(MAKE) cleandocs ; $(MAKE) pdfdocs)
	(cd swig/; $(MAKE) cleandocs ; $(MAKE) pdfdocs)
	@echo "Written PDF docs in: lib/db/latex/, lib/g3d/latex/, lib/gis/latex/, lib/gmath/latex/ lib/gpde/latex/ lib/ogsf/latex/, lib/proj//latex/, lib/segment/latex/, lib/vector/latex/ lib/vector/dglib/latex/ rfc/latex/ swig/latex/"

cleandocs:
	(cd lib/db/ ; $(MAKE) cleandocs)
	(cd lib/g3d/ ; $(MAKE) cleandocs)
	(cd lib/gis/ ; $(MAKE) cleandocs)
	(cd lib/gmath/ ; $(MAKE) cleandocs)
	(cd lib/gpde/ ; $(MAKE) cleandocs)
	(cd lib/ogsf/ ; $(MAKE) cleandocs)
	(cd lib/proj/ ; $(MAKE) cleandocs)
	(cd lib/segment/; $(MAKE) cleandocs)
	(cd lib/vector/ ; $(MAKE) cleandocs)
	(cd lib/vector/dglib/ ; $(MAKE) cleandocs)
	(cd lib/ ; $(MAKE) cleandocs)
	(cd rfc/ ; $(MAKE) cleandocs)
	(cd swig/; $(MAKE) cleandocs)

html2pdfdoc:
	@ echo "Light PDF document from modules' HTML documentation"
	@ # http://www.htmldoc.org
	@test -d $(GRASS_PDFDIR) || mkdir -p $(GRASS_PDFDIR)
	(cd ${ARCH_DISTDIR}/docs/html ; $(HTML2PDF) --webpage --no-links database.html display.html general.html imagery.html misc.html photo.html postscript.html raster.html raster3D.html vector.html -f $(GRASS_PDFDIR)/grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}commands.pdf)

html2pdfdoccomplete:
	@ echo "Complete PDF document from modules' HTML documentation"
	@ # http://www.htmldoc.org
	@test -d $(GRASS_PDFDIR) || mkdir -p $(GRASS_PDFDIR)
	(cd ${ARCH_DISTDIR}/docs/html ; $(HTML2PDF) --webpage database.html db.*.html -f $(GRASS_PDFDIR)/grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}database.pdf)
	(cd ${ARCH_DISTDIR}/docs/html ; $(HTML2PDF) --webpage display.html d.*.html -f $(GRASS_PDFDIR)/grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}display.pdf)
	(cd ${ARCH_DISTDIR}/docs/html ; $(HTML2PDF) --webpage general.html g.*.html -f $(GRASS_PDFDIR)/grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}general.pdf)
	(cd ${ARCH_DISTDIR}/docs/html ; $(HTML2PDF) --webpage imagery.html i.*.html -f $(GRASS_PDFDIR)/grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}imagery.pdf)
	(cd ${ARCH_DISTDIR}/docs/html ; $(HTML2PDF) --webpage misc.html m.*.html -f $(GRASS_PDFDIR)/grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}misc.pdf)
	(cd ${ARCH_DISTDIR}/docs/html ; $(HTML2PDF) --webpage photo.html i.ortho*.html photo*.html -f $(GRASS_PDFDIR)/grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}photo.pdf)
	(cd ${ARCH_DISTDIR}/docs/html ; $(HTML2PDF) --webpage postscript.html ps.*.html -f $(GRASS_PDFDIR)/grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}postscript.pdf)
	(cd ${ARCH_DISTDIR}/docs/html ; $(HTML2PDF) --webpage raster.html r.*.html -f $(GRASS_PDFDIR)/grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}raster.pdf)
	(cd ${ARCH_DISTDIR}/docs/html ; $(HTML2PDF) --webpage raster3D.html r3.*.html -f $(GRASS_PDFDIR)/grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}raster3d.pdf)
	(cd ${ARCH_DISTDIR}/docs/html ; $(HTML2PDF) --webpage vector.html v*.html -f $(GRASS_PDFDIR)/grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}vector.pdf)

changelog:
	@ echo "creating ChangeLog file (following 'trunk' only)..."
	@ # svn2cl creates a GNU style ChangeLog file:
	@ # http://ch.tudelft.nl/~arthur/svn2cl/
	@if [ ! -x "`which svn2cl`" ] ; then \
		echo "\"svn2cl\" is required, please install first from http://ch.tudelft.nl/~arthur/svn2cl/" ;	exit 1 ; \
	fi
	sh svn2cl ./ChangeLog


builddemolocation:
	test -d ${ARCH_DISTDIR} || ${MAKE_DIR_CMD} ${ARCH_DISTDIR}
	-tar cBf - demolocation | (cd ${ARCH_DISTDIR}/ ; tar xBfo - ) 2>/dev/null
	@ echo "GISDBASE: ${RUN_GISBASE}" > ${RUN_GISRC}
	@ echo "LOCATION_NAME: demolocation" >> ${RUN_GISRC}
	@ echo "MAPSET: PERMANENT" >> ${RUN_GISRC}
	@ echo "GRASS_DB_ENCODING: utf-8" >> ${RUN_GISRC}
	@ echo "DEBUG: 0" >> ${RUN_GISRC}
	@ echo "GRASS_GUI: text" >> ${RUN_GISRC}

.PHONY: default libs cleandistdirs cleanscriptstrings clean libsclean
.PHONY: distclean strip install real-install install-strip install-macosx
.PHONY: bindist real-bindist bindist-macosx srcdist srclibsdist
.PHONY: htmldocs-single htmldocs packagehtmldocs pdfdocs cleandocs html2pdfdoc
.PHONY: html2pdfdoccomplete changelog builddemolocation
