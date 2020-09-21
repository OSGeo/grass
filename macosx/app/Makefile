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

BUNDLE_VERSION := ${GRASS_VERSION_NUMBER}
# add git commit hash to bundle version if it is a development release
ifneq (,$(findstring dev,$(GRASS_VERSION_RELEASE)))
BUNDLE_VERSION := ${GRASS_VERSION_NUMBER} (${GRASS_VERSION_GIT})
endif

# Figure out version for MACOSX_DEPLOYMENT_TARGET, with following order:
# - if MACOSX_SDK is set then it will be used
# - the result of `xcrun --show-sdk-path`
# - check if Command Line Tools' SDKs is present, and use the default version
# - use the MacOS version of building machine
SDK_CMD = ""
ifeq ($(strip $(MACOSX_SDK)),)
SDK_CMD = xcrun --show-sdk-path
else
SDK_CMD = echo "${MACOSX_SDK}" | cut -d \  -f 2
endif
SDK := $(shell $(SDK_CMD))

ifeq ($(SDK),)
ifneq ($(wildcard /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/.*),)
SDK = /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk
endif
endif

ifneq ($(SDK),)
DEPLOYMENT_TARGET_CMD = plutil -extract DefaultProperties.MACOSX_DEPLOYMENT_TARGET \
	xml1 -o - $(SDK)/SDKSettings.plist | awk -F '[<>]' '/string/{print $$3}'
else
DEPLOYMENT_TARGET_CMD = sw_vers -productVersion | cut -d . -f 1,2
endif

DEPLOYMENT_TARGET := $(shell $(DEPLOYMENT_TARGET_CMD))

# ugly hack - assume gdal prefix starts with -L flag
ifndef GDAL_BIN
GDAL_BIN = `echo "${GDALLIBS}" | sed -e "s,-L,," -e "s,/lib.*$$,/bin:,"`
endif
# ugly hack - extract path to PROJ bin from nad2bin
ifndef PROJ_BIN
PROJ_BIN = `echo "${NAD2BIN}" | sed -e "s,.=,," -e "s,/nad2bin,:,"`
endif
# get python version - use python in path, should be config'd python
PYVER = `python -V 2>&1 | cut -d \  -f 2 | cut -d . -f 1,2`
ifneq ($(filter ppc64 x86_64,$(MACOSX_ARCHS_WXPYTHON)),)
WX64BIT = 1
else
WX64BIT = 0
endif

FILES = \
	$(OBJDIR)/GRASS \
	$(OBJDIR)/Info.plist \
	$(OBJDIR)/GRASS.scpt \
	$(OBJDIR)/grass.sh

default: $(FILES)

$(OBJDIR)/main.o: main.m
	$(MAKE_DIR_CMD) $(OBJDIR)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(EXTRA_INC) -o $@ -c $<

$(OBJDIR)/GRASS: $(OBJDIR)/main.o
	$(MAKE_DIR_CMD) $(OBJDIR)
	$(CC) $(LDFLAGS) -o $@ $< $(EXTRA_LDFLAGS)

$(OBJDIR)/Info.plist: Info.plist.in
	$(MAKE_DIR_CMD) $(OBJDIR)
	sed -e "s,@GRASS_VERSION_MAJOR@,$(GRASS_VERSION_MAJOR),g" \
		-e "s,@GRASS_VERSION_MINOR@,$(GRASS_VERSION_MINOR),g" \
		-e "s,@GRASS_VERSION_DATE@,$(GRASS_VERSION_DATE),g" \
		-e "s,@BUNDLE_VERSION@,$(BUNDLE_VERSION),g" \
		-e "s,@DEPLOYMENT_TARGET@,$(DEPLOYMENT_TARGET),g" \
		$< > $@

$(OBJDIR)/GRASS.scpt: GRASS.applescript
	$(MAKE_DIR_CMD) $(OBJDIR)
	osacompile -d -x -i /System/Library/Frameworks/AppleScriptKit.framework -o $@ $<

$(OBJDIR)/grass.sh: grass.sh.in
	$(MAKE_DIR_CMD) $(OBJDIR)
	sed -e "s,@GDAL_BIN_PATH@,$(GDAL_BIN)," \
		-e "s,@PROJ_BIN_PATH@,$(PROJ_BIN)," \
		-e "s,@X11@,$(USE_X11),g" \
		-e "s,@PYVER@,$(PYVER),g" \
		-e "s,@WX64BIT@,$(WX64BIT),g" \
		-e "s,@GRASS_NAME@,$(GRASS_NAME),g" \
		$< > $@

clean:
	-rm -rf $(OBJDIR) $(EXTRA_CLEAN_DIRS) $(EXTRA_CLEAN_FILES)
