MODULE_TOPDIR = ..

include $(MODULE_TOPDIR)/include/Make/Platform.make

MACOSX_APP_NAME = GRASS-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.app
MACOSX_APPDIR = ${MACOSX_APP_NAME}/Contents/MacOS
INST_DIR_MACOSX = ${prefix}/${MACOSX_APPDIR}
MACOSX_SUPPDIR = /Library/GRASS/${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}
MACOSX_BUNDLE_PREFIX = ${GRASS_HOME}/macosx/dist/${MACOSX_APPDIR}
VER_MINOR_REL = `printf "%d0%d" ${GRASS_VERSION_MINOR} ${GRASS_VERSION_RELEASE}`
VER_DATE = `date +"%g%m%d"`
MAKE_DIR_CMD = mkdir -p -m 755
LN = /bin/ln -sf
LN_DIR = /bin/ln -sfh

# packagemaker location, options and behaviour changed in xcode 3
ifeq ($(findstring darwin9,$(ARCH)),darwin9)
XCODE_VER = xcode3
PACKAGEMAKER = /Developer/usr/bin/packagemaker
PKG_INST_DIR = ${prefix}
else
XCODE_VER = xcode2
PACKAGEMAKER = /Developer/Tools/packagemaker
PKG_INST_DIR = ${prefix}/${MACOSX_APP_NAME}
endif

SUBDIRS = 
ifeq ($(MACOSX_APP),1)
SUBDIRS = app modbuild
endif

include $(MODULE_TOPDIR)/include/Make/Dir.make

default: subdirs

install-macosx:
	echo ${ARCH_DISTDIR}/${MACOSX_APP_NAME}
	@ if [ ! -d ${ARCH_DISTDIR}/${MACOSX_APP_NAME} ] ; then \
		echo "ERROR: GRASS.app has not been compiled. Try \"make\" in the top level source dir first."; \
		echo "  Installation aborted, exiting Make."; \
		exit; \
	fi; \
	${MAKE} real-install-macosx

# Duplicated, mostly, from the top-level makefile.  There are enough
# differences that make it simpler to have a separate install target.

real-install-macosx:
	${MAKE_DIR_CMD} ${INST_DIR}
	${MAKE} macosx-support-dir
	-cd ${GISBASE} ; tar cBf - ${MACOSX_APP_NAME} | (cd ${prefix} ; tar xBf - ) 2>/dev/null
	-cd ${GISBASE} ; tar cBf - bin | (cd ${INST_DIR} ; tar xBf - ) 2>/dev/null
	-cd ${GISBASE} ; tar cBf - bwidget | (cd ${INST_DIR} ; tar xBf - ) 2>/dev/null
	-cd ${GISBASE} ; tar cBf - docs | (cd ${INST_DIR} ; tar xBf - ) 2>/dev/null
	-cd ${GISBASE} ; tar cBf - driver | (cd ${INST_DIR} ; tar xBf - ) 2>/dev/null
	-cd ${GISBASE} ; tar cBf - etc | (cd ${INST_DIR} ; tar xBf - ) 2>/dev/null
	-cd ${GISBASE} ; tar cBf - fonts | (cd ${INST_DIR} ; tar xBf - ) 2>/dev/null
	-cd ${GISBASE} ; tar cBf - man | (cd ${INST_DIR} ; tar xBf - ) 2>/dev/null
	-cd ${GISBASE} ; tar cBf - scripts | (cd ${INST_DIR} ; tar xBf - ) 2>/dev/null
	if [ ${LOCALE} -eq 1 ] ; then cd ${GISBASE} ; tar cBf - locale | (cd ${INST_DIR} ; tar xBf - ) 2>/dev/null ; fi
	-cd ${GISBASE} ; tar cBf - include | (cd ${INST_DIR} ; tar xBf - ) 2>/dev/null
	-cd ${GISBASE} ; tar cBf - lib | (cd ${INST_DIR} ; tar xBf - ) 2>/dev/null
	-sed 's#'${GISBASE}'#'${INST_DIR}'#g' ${GISBASE}/etc/monitorcap > ${INST_DIR}/etc/monitorcap
	-chmod -R a+rX ${prefix}/${MACOSX_APP_NAME} 2>/dev/null
	-touch ${prefix}/${MACOSX_APP_NAME}
	-cd ${GRASS_HOME} ; tar cBf - gem/skeleton | (cd ${INST_DIR}/etc ; tar xBf - ) 2>/dev/null
	-${INSTALL} ${GRASS_HOME}/gem/gem6 ${prefix}/${MACOSX_APP_NAME}/Contents/MacOS 2>/dev/null
	-cd ${GISBASE} ; tar cBf - modbuild | (cd ${MACOSX_SUPPDIR} ; tar xBf - ) 2>/dev/null
	@# help file package - in the future files will go in /Library/Documentation/Help
	${LN_DIR} "${INST_DIR}/docs/html" /Library/Documentation/Help/GRASS-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}

# create a packagemaker installer package.
# pretty much the same as install, but needs intermediate install location
# to assemble package parts.

include pkg/bundle.make

bindist-macosx:
	${MAKE_DIR_CMD} ${GRASS_HOME}/macosx/dist
	-cd ${GISBASE} ; tar cBf - ${MACOSX_APP_NAME} | (cd ${GRASS_HOME}/macosx/dist ; tar xBf - ) 2>/dev/null
	-cd ${GISBASE} ; tar cBf - bin | (cd ${MACOSX_BUNDLE_PREFIX} ; tar xBf - ) 2>/dev/null
	-cd ${GISBASE} ; tar cBf - bwidget | (cd ${MACOSX_BUNDLE_PREFIX} ; tar xBf - ) 2>/dev/null
	-cd ${GISBASE} ; tar cBf - docs | (cd ${MACOSX_BUNDLE_PREFIX} ; tar xBf - ) 2>/dev/null
	-cd ${GISBASE} ; tar cBf - driver | (cd ${MACOSX_BUNDLE_PREFIX} ; tar xBf - ) 2>/dev/null
	-cd ${GISBASE} ; tar cBf - etc | (cd ${MACOSX_BUNDLE_PREFIX} ; tar xBf - ) 2>/dev/null
	-cd ${GISBASE} ; tar cBf - fonts | (cd ${MACOSX_BUNDLE_PREFIX} ; tar xBf - ) 2>/dev/null
	-cd ${GISBASE} ; tar cBf - man | (cd ${MACOSX_BUNDLE_PREFIX} ; tar xBf - ) 2>/dev/null
	-cd ${GISBASE} ; tar cBf - scripts | (cd ${MACOSX_BUNDLE_PREFIX} ; tar xBf - ) 2>/dev/null
	if [ ${LOCALE} -eq 1 ] ; then cd ${GISBASE} ; tar cBf - locale | (cd ${MACOSX_BUNDLE_PREFIX} ; tar xBf - ) 2>/dev/null ; fi
	-cd ${GISBASE} ; tar cBf - include | (cd ${MACOSX_BUNDLE_PREFIX} ; tar xBf - ) 2>/dev/null
	-cd ${GISBASE} ; tar cBf - lib | (cd ${MACOSX_BUNDLE_PREFIX} ; tar xBf - ) 2>/dev/null
	-sed 's#'${GISBASE}'#'${INST_DIR_MACOSX}'#g' ${GISBASE}/etc/monitorcap > dist/${MACOSX_APPDIR}/etc/monitorcap
	-chmod -R a+rX dist/${MACOSX_APPDIR} 2>/dev/null
	-cd ${GRASS_HOME} ; tar cBf - gem/skeleton | (cd ${MACOSX_BUNDLE_PREFIX}/etc ; tar xBf - ) 2>/dev/null
	-${INSTALL} ${GRASS_HOME}/gem/gem6 dist/${MACOSX_APP_NAME}/Contents/MacOS 2>/dev/null
	@# extras to bundle
	${MAKE} bundle-macosx
	@# pkg resources
	${MAKE_DIR_CMD} dist/resources
	-sed -e "s#@INST_DIR@#${INST_DIR}#g" -e "s#@MACOSX_SUPPDIR@#${MACOSX_SUPPDIR}#g" -e "s#@GRASS_VER@#${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}#g" pkg/resources/postflight.in > dist/resources/postflight
	-chmod a+rx dist/resources/postflight
	-sed -e "s#@GRASS_VER@#${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}#g" pkg/resources/Description.plist.in > dist/Description.plist
	-sed -e "s#@GRASS_VERSION_MAJOR@#${GRASS_VERSION_MAJOR}#g" -e "s#@GRASS_VERSION_MINOR@#${GRASS_VERSION_MINOR}#g" -e "s#@GRASS_VERSION_RELEASE@#${GRASS_VERSION_RELEASE}#g" -e "s#@VER_MINOR_REL@#${VER_MINOR_REL}#g" -e "s#@VER_DATE@#${VER_DATE}#g" -e "s#@PKG_INST_DIR@#${PKG_INST_DIR}#g" pkg/resources/Info.plist.in > dist/Info.plist
	-${INSTALL_DATA} pkg/resources/License.rtf dist/resources
	-${INSTALL_DATA} pkg/resources/ReadMe.rtf dist/resources
	-cd ${GISBASE} ; tar cBf - modbuild | (cd ${GRASS_HOME}/macosx/dist/resources ; tar xBf - ) 2>/dev/null
	@# build the package
ifeq ($(XCODE_VER),xcode3)
	${PACKAGEMAKER} --target 10.4 --root-volume-only --id org.osgeo.grass --root dist/${MACOSX_APP_NAME} --info dist/Info.plist --out "GRASS-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.pkg" --resources dist/resources
	rm -f "GRASS-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.pkg/Contents/Resources/TokenDefinitions.plist"
else
	${PACKAGEMAKER} -build -ds -p "GRASS-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.pkg" -f dist/${MACOSX_APP_NAME} -r dist/resources -i dist/Info.plist -d dist/Description.plist
endif
	@# remove intermediate files
	@# -rm -rf dist 2>/dev/null

macosx-support-dir:
	${MAKE_DIR_CMD} ${MACOSX_SUPPDIR}
	${MAKE_DIR_CMD} ${MACOSX_SUPPDIR}/Modules/bin
	${MAKE_DIR_CMD} ${MACOSX_SUPPDIR}/Modules/docs/html
	${MAKE_DIR_CMD} ${MACOSX_SUPPDIR}/Modules/etc
	${MAKE_DIR_CMD} ${MACOSX_SUPPDIR}/Modules/lib

clean: cleansubdirs
	-rm -rf ${ARCH_DISTDIR}/${MACOSX_APP_NAME}/ 2>/dev/null
	-rm -rf ${ARCH_DISTDIR}/modbuild/ 2>/dev/null
