# Makefile for building Mac OS X app startup

MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make

EXTRA_INC = 
EXTRA_CFLAGS = 
XTRA_LDFLAGS = -framework Cocoa -framework AppleScriptKit
ifndef MACOSX_APP_NAME
MACOSX_APP_NAME = GRASS-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.app
endif
APPDIR = $(ARCH_DISTDIR)/${MACOSX_APP_NAME}/Contents
ifndef TCLTK_INTERNAL
TCLTK_INTERNAL = \#
endif
MAKE_DIR_CMD = mkdir -p -m 755
GRASS_VERSION_BUILD = `/bin/date "+%y%m%d"`

OBJS = Info.plist GRASS.scpt grass.sh main.o

ARCH_OBJS := $(foreach obj,$(OBJS),$(OBJDIR)/$(obj)) 

# if building on Leopard with TclTk 8.5, set this version to match
ifndef TCLTKVER
TCLTKVER = 8.4
endif
# ugly hack - assume tcl and tk in same prefix, and same as TCLTKLIBPATH minus 'lib'
ifndef TCLTKPREFIX
TCLTKPREFIX = `echo "${TCLTKLIBPATH}" | sed -e "s,-L,," -e "s,/lib *$$,,"`
endif
# ugly hack - assume gdal prefix starts with -L flag
ifndef GDAL_BIN
GDAL_BIN = `echo "${GDALLIBS}" | sed -e "s,-L,," -e "s,/lib.*$$,/bin,"`
endif
# ugly hack - extract path to PROJ bin from nad2bin
ifndef PROJ_BIN
PROJ_BIN = `echo "${NAD2BIN}" | sed -e "s,.=,," -e "s,/nad2bin,,"`
endif

default: macosxapp

macosxapp: PkgInfo app.icns English.lproj/MainMenu.nib build_html_user_index.sh build_gui_user_menu.sh $(ARCH_OBJS) $(APPDIR)/MacOS/GRASS
	-${MAKE_DIR_CMD} ${APPDIR}/Resources/Scripts
	-${MAKE_DIR_CMD} ${APPDIR}/MacOS/scripts
	-${MAKE_DIR_CMD} ${APPDIR}/MacOS/etc
	-${INSTALL_DATA} app.icns ${APPDIR}/Resources
	-tar cBf - English.lproj --exclude CVS | (cd ${APPDIR}/Resources ; tar xBf - ) 2>/dev/null
	-${INSTALL_DATA} $(OBJDIR)/Info.plist ${APPDIR}
	-${INSTALL_DATA} PkgInfo ${APPDIR}
	-${INSTALL} $(OBJDIR)/grass.sh ${APPDIR}/MacOS
	-${INSTALL_DATA} $(OBJDIR)/GRASS.scpt ${APPDIR}/Resources/Scripts
	-${INSTALL} build_html_user_index.sh ${APPDIR}/MacOS/etc
	-${INSTALL} build_gui_user_menu.sh ${APPDIR}/MacOS/etc
	@# TclTk embedding:
ifdef OPENGL_X11
	-${MAKE_DIR_CMD} ${APPDIR}/MacOS/bin
	-${INSTALL} ${TCLTKPREFIX}/bin/tclsh${TCLTKVER} ${APPDIR}/MacOS/bin
	-${INSTALL} ${TCLTKPREFIX}/bin/wish${TCLTKVER} ${APPDIR}/MacOS/bin
	-${MAKE_DIR_CMD} ${APPDIR}/MacOS/lib
	-${INSTALL} ${TCLTKPREFIX}/lib/libtcl${TCLTKVER}.dylib ${APPDIR}/MacOS/lib
	-${INSTALL} ${TCLTKPREFIX}/lib/libtk${TCLTKVER}.dylib ${APPDIR}/MacOS/lib
	-cd ${TCLTKPREFIX}/lib ; tar cBf - tcl${TCLTKVER} | (cd ${APPDIR}/MacOS/lib ; tar xBf - ) 2>/dev/null
	-cd ${TCLTKPREFIX}/lib ; tar cBf - tk${TCLTKVER} | (cd ${APPDIR}/MacOS/lib ; tar xBf - ) 2>/dev/null
	-${MAKE_DIR_CMD} ${APPDIR}/MacOS/include
	-${INSTALL_DATA} ${TCLTKPREFIX}/include/tcl.h ${APPDIR}/MacOS/include
	-${INSTALL_DATA} ${TCLTKPREFIX}/include/tclDecls.h ${APPDIR}/MacOS/include
	-${INSTALL_DATA} ${TCLTKPREFIX}/include/tclPlatDecls.h ${APPDIR}/MacOS/include
	-${INSTALL_DATA} ${TCLTKPREFIX}/include/tk.h ${APPDIR}/MacOS/include
	-${INSTALL_DATA} ${TCLTKPREFIX}/include/tkDecls.h ${APPDIR}/MacOS/include
	-${INSTALL_DATA} ${TCLTKPREFIX}/include/tkPlatDecls.h ${APPDIR}/MacOS/include
endif

$(OBJDIR)/main.o: main.m
	$(MAKE_DIR_CMD) $(OBJDIR)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(EXTRA_INC) -o $(OBJDIR)/main.o -c main.m

$(APPDIR)/MacOS/GRASS: $(OBJDIR)/main.o
	-$(MAKE_DIR_CMD) ${APPDIR}/MacOS
	$(CC) $(LDFLAGS) -o ${APPDIR}/MacOS/GRASS $(OBJDIR)/main.o $(XTRA_LDFLAGS)

$(OBJDIR)/Info.plist: Info.plist.in
	$(MAKE_DIR_CMD) $(OBJDIR)
	sed -e "s,@GRASS_VERSION_MAJOR@,$(GRASS_VERSION_MAJOR),g" \
	-e "s,@GRASS_VERSION_MINOR@,$(GRASS_VERSION_MINOR),g" \
	-e "s,@GRASS_VERSION_RELEASE@,$(GRASS_VERSION_RELEASE),g" \
	-e "s,@GRASS_VERSION_BUILD@,$(GRASS_VERSION_BUILD),g" \
	Info.plist.in > $(OBJDIR)/Info.plist

$(OBJDIR)/GRASS.scpt: GRASS.applescript
	$(MAKE_DIR_CMD) $(OBJDIR)
	osacompile -d -x -i /System/Library/Frameworks/AppleScriptKit.framework -o $(OBJDIR)/GRASS.scpt GRASS.applescript

$(OBJDIR)/grass.sh: grass.sh.in
	$(MAKE_DIR_CMD) $(OBJDIR)
	sed -e "s,@GDAL_BIN_PATH@,$(GDAL_BIN)," -e "s,@PROJ_BIN_PATH@,$(PROJ_BIN)," -e "s,@TCLTK_INTERNAL@,$(TCLTK_INTERNAL)," -e "s,@TCLTKVER@,$(TCLTKVER)," grass.sh.in > $(OBJDIR)/grass.sh

clean:
	-rm -rf $(OBJDIR) $(EXTRA_CLEAN_DIRS) $(EXTRA_CLEAN_FILES)
