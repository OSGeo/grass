MODULE_TOPDIR = ..

include $(MODULE_TOPDIR)/include/Make/Vars.make

SUBDIRS = 
ifeq ($(MACOSX_APP),1)
SUBDIRS = app
endif

include $(MODULE_TOPDIR)/include/Make/Dir.make

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
# get python version - use python in path, should be config'd python
PYVER = `python -V 2>&1 | cut -d \  -f 2 | cut -d . -f 1,2`

# packagemaker location, options and behaviour changed in xcode 3
ifeq ($(findstring darwin8,$(ARCH)),darwin8)
XCODE_VER = xcode2
PACKAGEMAKER = /Developer/Tools/packagemaker
PKG_INST_DIR = ${prefix}/${MACOSX_APP_NAME}
else
XCODE_VER = xcode3
PACKAGEMAKER = /Developer/usr/bin/packagemaker
PKG_INST_DIR = ${prefix}
endif

default: subdirs

include bundle.make

# Duplicated, mostly, from the top-level makefile.

install-macosx: INST_DIR_TARGET = ${INST_DIR}
install-macosx: PREFIX_TARGET = ${prefix}
install-macosx: install-macosxapp install-basic-macosx macosx-support-dir bundle-macosx
	echo ${ARCH_DISTDIR}/${MACOSX_APP_NAME}
	@# help file package
	${LN_DIR} "${INST_DIR_TARGET}/docs/html" /Library/Documentation/Help/GRASS-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}

# create a packagemaker installer package.
# pretty much the same as install, but needs intermediate install location
# to assemble package parts.

bindist-macosx: INST_DIR_TARGET = ${MACOSX_BUNDLE_PREFIX}
bindist-macosx: PREFIX_TARGET = ${GRASS_HOME}/macosx/dist
bindist-macosx: install-macosxapp install-basic-macosx bundle-macosx
	@# installer pkg resources
	${MAKE_DIR_CMD} dist/resources
	-sed -e "s#@INST_DIR@#${INST_DIR}#g" \
		-e "s#@MACOSX_SUPPDIR@#${MACOSX_SUPPDIR}#g" \
		-e "s#@GRASS_VER@#${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}#g" \
		-e "s#@PYVER@#${PYVER}#g" \
		pkg/resources/postflight.in > dist/resources/postflight
	-chmod a+rx dist/resources/postflight
	-sed -e "s#@GRASS_VER@#${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}#g" \
		pkg/resources/Description.plist.in > dist/Description.plist
	-sed -e "s#@GRASS_VERSION_MAJOR@#${GRASS_VERSION_MAJOR}#g" \
		-e "s#@GRASS_VERSION_MINOR@#${GRASS_VERSION_MINOR}#g" \
		-e "s#@GRASS_VERSION_RELEASE@#${GRASS_VERSION_RELEASE}#g" \
		-e "s#@VER_MINOR_REL@#${VER_MINOR_REL}#g" \
		-e "s#@VER_DATE@#${VER_DATE}#g" \
		-e "s#@PKG_INST_DIR@#${PKG_INST_DIR}#g" \
		pkg/resources/Info.plist.in > dist/Info.plist
	-${INSTALL_DATA} pkg/resources/License.rtf dist/resources
	-${INSTALL_DATA} pkg/resources/ReadMe.rtf dist/resources
	@# build the package
ifeq ($(XCODE_VER),xcode3)
	${PACKAGEMAKER} --target 10.4 --root-volume-only --id org.osgeo.grass --root dist/${MACOSX_APP_NAME} --info dist/Info.plist --out "GRASS-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.pkg" --resources dist/resources
	rm -f "GRASS-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.pkg/Contents/Resources/TokenDefinitions.plist"
else
	${PACKAGEMAKER} -build -ds -p "GRASS-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.pkg" -f dist/${MACOSX_APP_NAME} -r dist/resources -i dist/Info.plist -d dist/Description.plist
endif

install-macosxapp:
	-$(MAKE_DIR_CMD) ${INST_DIR_TARGET}/bin
	-$(MAKE_DIR_CMD) ${INST_DIR_TARGET}/etc
	-$(MAKE_DIR_CMD) ${PREFIX_TARGET}/${MACOSX_APP_NAME}/Contents/Resources/Scripts
	-$(MAKE_DIR_CMD) ${PREFIX_TARGET}/${MACOSX_APP_NAME}/Contents/Resources/English.lproj/MainMenu.nib
	$(INSTALL) app/build_html_user_index.sh ${INST_DIR_TARGET}/etc
	$(INSTALL) app/build_gui_user_menu.sh ${INST_DIR_TARGET}/etc
	$(INSTALL) app/python_wrapper ${INST_DIR_TARGET}/bin/python 
	$(INSTALL_DATA) app/app.icns ${PREFIX_TARGET}/${MACOSX_APP_NAME}/Contents/Resources
	$(INSTALL_DATA) app/${OBJDIR}/Info.plist ${PREFIX_TARGET}/${MACOSX_APP_NAME}/Contents
	$(INSTALL_DATA) app/PkgInfo ${PREFIX_TARGET}/${MACOSX_APP_NAME}/Contents
	$(INSTALL) app/${OBJDIR}/GRASS.scpt ${PREFIX_TARGET}/${MACOSX_APP_NAME}/Contents/Resources/Scripts/
	$(INSTALL) app/${OBJDIR}/grass.sh ${INST_DIR_TARGET}
	$(INSTALL) app/${OBJDIR}/GRASS ${INST_DIR_TARGET}
	$(INSTALL_DATA) app/English.lproj/MainMenu.nib/* ${PREFIX_TARGET}/${MACOSX_APP_NAME}/Contents/Resources/English.lproj/MainMenu.nib

install-basic-macosx:
	-tar cBCf $(GISBASE) - . | tar xBCf $(INST_DIR_TARGET) - 2>/dev/null
	-sed -e 's#'$(GISBASE)'#'$(INST_DIR)'#g' \
		${GISBASE}/etc/fontcap > ${INST_DIR_TARGET}/etc/fontcap
	-sed -e 's#'$(GISBASE)'#'$(INST_DIR)'#g' \
		${GISBASE}/demolocation/.grassrc$(GRASS_VERSION_MAJOR)$(GRASS_VERSION_MINOR) > ${INST_DIR_TARGET}/demolocation/.grassrc$(GRASS_VERSION_MAJOR)$(GRASS_VERSION_MINOR)
	sed -e 's#^\(GRASS_HOME.[^=]*\).*#\1= $(INST_DIR)#g' \
		-e 's#$(GISBASE)#$(INST_DIR)#g' \
		${GISBASE}/include/Make/Platform.make > ${INST_DIR_TARGET}/include/Make/Platform.make
	sed -e 's#^\(ARCH_DISTDIR.[^=]*\).*#\1= $(INST_DIR)#g' \
		-e 's#^\(ARCH_BINDIR.[^=]*\).*#\1= $(UNIX_BIN)#g' \
		${GISBASE}/include/Make/Grass.make > ${INST_DIR_TARGET}/include/Make/Grass.make
	-sed -e 's#'@GISBASE@'#'$(INST_DIR)'#g' \
	     -e 's#'@LD_LIBRARY_PATH_VAR@'#'$(LD_LIBRARY_PATH_VAR)'#g' \
	     -e 's#'@CONFIG_PROJSHARE@'#'$(PROJSHARE)'#g' \
	     $(ARCH_DISTDIR)/$(GRASS_NAME).tmp > $(INST_DIR_TARGET)/$(GRASS_NAME)
	-chmod a+x $(UNIX_BIN)/$(GRASS_NAME)
	-rm -f $(INST_DIR_TARGET)/$(GRASS_NAME).tmp
	-chmod -R a+rX ${PREFIX_TARGET}/${MACOSX_APP_NAME} 2>/dev/null
	-touch ${PREFIX_TARGET}/${MACOSX_APP_NAME}

macosx-support-dir:
	${MAKE_DIR_CMD} "${MACOSX_SUPPDIR}"
	${MAKE_DIR_CMD} "${MACOSX_SUPPDIR}/Modules/bin"
	${MAKE_DIR_CMD} "${MACOSX_SUPPDIR}/Modules/docs/html"
	${MAKE_DIR_CMD} "${MACOSX_SUPPDIR}/Modules/etc"
	${MAKE_DIR_CMD} "${MACOSX_SUPPDIR}/Modules/lib"
