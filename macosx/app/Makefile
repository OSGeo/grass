# Makefile for building Mac OS X app startup

MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Vars.make
MOD_OBJS = 

EXTRA_INC = 
EXTRA_CFLAGS = 
EXTRA_LDFLAGS = -framework Cocoa -framework AppleScriptKit
ifndef MACOSX_APP_NAME
MACOSX_APP_NAME = GRASS-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.app
endif
APPDIR = $(ARCH_DISTDIR)/${MACOSX_APP_NAME}/Contents
MAKE_DIR_CMD = mkdir -p -m 755
GRASS_VERSION_BUILD = `/bin/date "+%y%m%d"`

# ugly hack - assume gdal prefix starts with -L flag
ifndef GDAL_BIN
GDAL_BIN = `echo "${GDALLIBS}" | sed -e "s,-L,," -e "s,/lib.*$$,/bin:,"`
endif
# ugly hack - extract path to PROJ bin from nad2bin
ifndef PROJ_BIN
PROJ_BIN = `echo "${NAD2BIN}" | sed -e "s,.=,," -e "s,/nad2bin,:,"`
endif

NIBSRC := $(wildcard English.lproj/MainMenu.nib/*)
NIBDST := $(patsubst English.lproj/%,$(APPDIR)/Resources/English.lproj/%,$(NIBSRC))

FILES = \
	$(APPDIR)/MacOS/etc/build_html_user_index.sh \
	$(APPDIR)/MacOS/etc/build_gui_user_menu.sh \
	$(APPDIR)/Resources/app.icns \
	$(APPDIR)/Info.plist \
	$(APPDIR)/PkgInfo \
	$(APPDIR)/Resources/Scripts/GRASS.scpt \
	$(APPDIR)/MacOS/grass.sh \
	$(APPDIR)/MacOS/GRASS \
	$(NIBDST)

default: macosxapp

macosxapp:
	-$(MAKE_DIR_CMD) $(OBJDIR)
	-$(MAKE_DIR_CMD) $(APPDIR)/Resources/Scripts
	-$(MAKE_DIR_CMD) $(APPDIR)/MacOS/scripts
	-$(MAKE_DIR_CMD) $(APPDIR)/MacOS/etc
	-$(MAKE_DIR_CMD) $(APPDIR)/Resources/English.lproj/MainMenu.nib
	$(MAKE) $(FILES)

$(APPDIR)/Resources/app.icns: app.icns
	$(INSTALL_DATA) $< $@

$(APPDIR)/Resources/English.lproj/%: English.lproj/%
	$(INSTALL_DATA) $< $@

$(APPDIR)/Info.plist: $(OBJDIR)/Info.plist
	$(INSTALL_DATA) $< $@

$(APPDIR)/PkgInfo: PkgInfo
	$(INSTALL_DATA) $< $@

$(APPDIR)/Resources/Scripts/GRASS.scpt: $(OBJDIR)/GRASS.scpt
	$(INSTALL_DATA) $< $@

$(APPDIR)/MacOS/grass.sh: $(OBJDIR)/grass.sh
	$(INSTALL) $< $@

$(APPDIR)/MacOS/etc/%.sh: %.sh
	$(INSTALL) $< $@

$(OBJDIR)/main.o: main.m
	$(MAKE_DIR_CMD) $(OBJDIR)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(EXTRA_INC) -o $(OBJDIR)/main.o -c main.m

$(APPDIR)/MacOS/GRASS: $(OBJDIR)/main.o
	-$(MAKE_DIR_CMD) $(APPDIR)/MacOS
	$(CC) $(LDFLAGS) -o $(APPDIR)/MacOS/GRASS $(OBJDIR)/main.o $(EXTRA_LDFLAGS)

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
	sed -e "s,@GDAL_BIN_PATH@,$(GDAL_BIN)," \
	-e "s,@PROJ_BIN_PATH@,$(PROJ_BIN)," \
	-e "s,@X11@,$(USE_X11),g" \
	grass.sh.in > $(OBJDIR)/grass.sh

clean:
	-rm -rf $(OBJDIR) $(EXTRA_CLEAN_DIRS) $(EXTRA_CLEAN_FILES)
