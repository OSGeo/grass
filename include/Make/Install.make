
# This should be "include"d from the top-level Makefile, and nowhere else

BIN_DIST_FILES = $(FILES) \
	$(GRASS_NAME).tmp \
	bin \
	demolocation \
	docs \
	driver \
	etc \
	fonts \
	gui \
	include \
	lib \
	locale \
	scripts \
	utils \
	share

# Shell commands
MAKE_DIR_CMD	= $(MKDIR) -m 755

strip: strip-check
	find $(ARCH_DISTDIR) -type f -perm +111 -print0 | xargs -0 strip

strip-check:
	@ if [ ! -f "$(ARCH_BINDIR)/$(GRASS_NAME)" ] ; then \
		echo "ERROR: GRASS has not been compiled. Try \"make\" first." >&2 ; \
		echo "  Strip aborted, exiting Make." >&2 ; \
		exit 1; \
	fi

install:
	@ echo $(ARCH_BINDIR)/$(GRASS_NAME)
	$(MAKE) install-check-built
ifeq ($(strip $(MACOSX_APP)),1)
	$(MAKE) install-macosx
else
	$(MAKE) install-check-parent
	$(MAKE) install-check-writable
	$(MAKE) install-check-prefix
	$(MAKE) real-install
endif

install-check-built:
	@ if [ ! -f "$(ARCH_BINDIR)/$(GRASS_NAME)" ] ; then \
		echo "ERROR: GRASS has not been compiled. Try \"make\" first."; \
		echo "  Installation aborted, exiting Make."; \
		exit; \
	fi

install-check-parent: | $(DESTDIR)
	@ INST_PATH=`dirname $(DESTDIR)$(INST_DIR)`; \
	while [ ! -d "$(DESTDIR)$$INST_PATH" ]; do \
		INST_PATH=`dirname $$INST_PATH`; \
	done; \
	if [ ! -d "$(DESTDIR)$(INST_DIR)" -a ! -w "$(DESTDIR)$$INST_PATH" ] ; then \
		echo "ERROR: Directory $$INST_PATH is a parent directory of your" >&2 ; \
		echo "  install directory $(INST_DIR) and is not writable." >&2 ; \
		echo "  Perhaps you need root access." >&2 ; \
		echo "  Installation aborted, exiting Make." >&2 ; \
		exit 1; \
	fi

install-check-writable:
	@ if [ -d "$(DESTDIR)$(INST_DIR)" -a ! -w "$(DESTDIR)$(INST_DIR)" ] ; then \
		echo "ERROR: Your install directory $(INST_DIR) is not writable." >&2 ; \
		echo "  Perhaps you need root access." >&2 ; \
		echo "  Installation aborted, exiting Make." >&2 ; \
		exit 1; \
	fi

install-check-prefix:
	@ result=`echo "$(DESTDIR)$(INST_DIR)" | awk '{ if (tolower($$1) ~ /grass/) print $$1 }'`; \
	if [ "$$result" = "" ] ; then \
		echo "WARNING: Your install directory $(INST_DIR)" >&2 ; \
		echo "  does not contain the word 'grass'." >&2 ; \
		echo "  It is highly recommended that the word 'grass' be part" >&2 ; \
		echo "  of your install directory to avoid conflicts." >&2 ; \
		echo "  Do you want to continue? [y/n]" >&2 ; \
		read ans; \
		if [ "$$ans" != "y" -a "$$ans" != "Y" ] ; then \
			echo "Installation aborted, exiting Make." >&2 ; \
			exit 1; \
		fi; \
	fi

ifneq ($(strip $(MINGW)),)
STARTUP = $(INST_DIR)/etc/$(GRASS_NAME).py
else
STARTUP = $(DESTDIR)$(UNIX_BIN)/$(GRASS_NAME)
endif

FONTCAP = etc/fontcap
TMPGISRC = demolocation/.grassrc$(GRASS_VERSION_MAJOR)$(GRASS_VERSION_MINOR)
PLATMAKE = include/Make/Platform.make
GRASSMAKE = include/Make/Grass.make

real-install: | $(DESTDIR) $(DESTDIR)$(INST_DIR) $(DESTDIR)$(UNIX_BIN)
	-tar cBCf $(GISBASE) - . | tar xBCf $(DESTDIR)$(INST_DIR) - 2>/dev/null
	-rm $(DESTDIR)$(INST_DIR)/$(GRASS_NAME).tmp
	$(MAKE) $(STARTUP)

	-rm $(DESTDIR)$(INST_DIR)/$(FONTCAP)
	$(MAKE) $(DESTDIR)$(INST_DIR)/$(FONTCAP)

	-rm $(DESTDIR)$(INST_DIR)/$(TMPGISRC)
	$(MAKE) $(DESTDIR)$(INST_DIR)/$(TMPGISRC)

	-rm $(DESTDIR)$(INST_DIR)/$(PLATMAKE)
	$(MAKE) $(DESTDIR)$(INST_DIR)/$(PLATMAKE)

	-rm $(DESTDIR)$(INST_DIR)/$(GRASSMAKE)
	$(MAKE) $(DESTDIR)$(INST_DIR)/$(GRASSMAKE)

	-$(CHMOD) -R a+rX $(DESTDIR)$(INST_DIR) 2>/dev/null

$(DESTDIR):
	$(MAKE_DIR_CMD) -p $@

$(DESTDIR)$(INST_DIR) $(DESTDIR)$(UNIX_BIN):
	$(MAKE_DIR_CMD) $@

$(STARTUP): $(ARCH_DISTDIR)/$(GRASS_NAME).tmp
	sed -e 's#'@GISBASE_INSTALL_PATH@'#'$(INST_DIR)'#g' \
	    -e 's#'@LD_LIBRARY_PATH_VAR@'#'$(LD_LIBRARY_PATH_VAR)'#g' \
	    -e 's#'@CONFIG_PROJSHARE@'#'$(PROJSHARE)'#g' \
	    $< > $@
	-$(CHMOD) a+x $@

define fix_gisbase
sed -e 's#$(GISBASE)#$(INST_DIR)#g' $< > $@
endef

define fix_grass_home
sed -e 's#^\(GRASS_HOME.[^=]*\).*#\1= $(INST_DIR)#g' \
    -e 's#$(GISBASE)#$(INST_DIR)#g' $< > $@
endef

define fix_grass_arch
sed -e 's#^\(ARCH_DISTDIR.[^=]*\).*#\1= $(INST_DIR)#g' \
    -e 's#^\(ARCH_BINDIR.[^=]*\).*#\1= $(UNIX_BIN)#g' $< > $@
endef

$(DESTDIR)$(INST_DIR)/$(FONTCAP): $(GISBASE)/$(FONTCAP)
	$(call fix_gisbase)

$(DESTDIR)$(INST_DIR)/$(TMPGISRC): $(GISBASE)/$(TMPGISRC)
	$(call fix_gisbase)

$(DESTDIR)$(INST_DIR)/$(PLATMAKE): $(GISBASE)/$(PLATMAKE)
	$(call fix_grass_home)

$(DESTDIR)$(INST_DIR)/$(GRASSMAKE): $(GISBASE)/$(GRASSMAKE)
	$(call fix_grass_arch)

install-macosx:
	$(MAKE) -C macosx install-macosx

install-strip:
	$(MAKE) strip
	$(MAKE) install

bindist:
ifeq ($(strip $(MACOSX_APP)),1)
	$(MAKE) bindist-macosx
else
	$(MAKE) real-bindist
endif

BINDISTNAME = grass-$(GRASS_VERSION_NUMBER)-$(ARCH)-$(DATE)

real-bindist:
	-tar cCBf $(ARCH_DISTDIR) - $(BIN_DIST_FILES) | gzip -fc > $(BINDISTNAME).tar.gz
	-size=`ls -l $(BINDISTNAME).tar.gz | awk '{print $$5}'`; \
	    sed -e "s/BIN_DIST_VERSION/$(GRASS_VERSION_NUMBER)-$(ARCH)-$(DATE)/" \
	    -e "s/GRASSPRG_NAME/$(GRASS_NAME)/" \
	    -e "s/SIZE_TAR_FILE/$$size/" -e "s#BIN_DIST_DIR#'$(INST_DIR)'#" \
	    -e "s/ARCHITECTURE/$(ARCH)/" \
	    -e "s/LD_LIBRARY_PATH_VAR/$(LD_LIBRARY_PATH_VAR)/" \
	    -e "s/TEST_STR=/TEST_STR=executable/" \
	    -e "s#IMPORTANT.*#Generated from the binaryInstall.src file using the command make bindist#" \
	    -e "s/# executable shell.*//" -e "s/# make bindist.*//" \
	    binaryInstall.src > $(BINDISTNAME)-install.sh ; \
	    chmod a+x $(BINDISTNAME)-install.sh 2>/dev/null

bindist-macosx:
	$(MAKE) -C macosx bindist-macosx

# make a source package for distribution:
srcdist: distclean
	-$(MAKE_DIR_CMD) ./grass-$(GRASS_VERSION_NUMBER)

	@ # needed to store code in package with grass-version path:
	-mv * ./grass-$(GRASS_VERSION_NUMBER)
	@ # do not include the debian control files:
	-mv ./grass-$(GRASS_VERSION_NUMBER)/debian .
	@ #we use -h to get the linked files into as real files:
	tar cvfzh grass-$(GRASS_VERSION_NUMBER).tar.gz ./grass-$(GRASS_VERSION_NUMBER)/* --exclude=.git
	@ # restore src code location:
	-mv ./grass-$(GRASS_VERSION_NUMBER)/* .
	-rmdir ./grass-$(GRASS_VERSION_NUMBER)
	@ echo "Distribution source package: grass-$(GRASS_VERSION_NUMBER).tar.gz ready. Calculating MD5 sum..."
	md5sum grass-$(GRASS_VERSION_NUMBER).tar.gz > grass-$(GRASS_VERSION_NUMBER).md5sum

# make a source package for library distribution:
srclibsdist: distclean
	-$(MAKE_DIR_CMD) ./grass-lib-$(GRASS_VERSION_NUMBER)

	@ # needed to store code in package with grass-version path:
	-cp -L * ./grass-lib-$(GRASS_VERSION_NUMBER)
	-cp -rL utils ./grass-lib-$(GRASS_VERSION_NUMBER)
	-cp -rL demolocation ./grass-lib-$(GRASS_VERSION_NUMBER)
	-cp -rL include ./grass-lib-$(GRASS_VERSION_NUMBER)
	-cp -rL --parents lib/external/parson ./grass-lib-$(GRASS_VERSION_NUMBER)
	-cp -rL --parents lib/external/shapelib ./grass-lib-$(GRASS_VERSION_NUMBER)
	-cp -rL --parents lib/external/bwidget ./grass-lib-$(GRASS_VERSION_NUMBER)
	-cp -rL --parents lib/datetime ./grass-lib-$(GRASS_VERSION_NUMBER)
	-cp -rL --parents lib/db ./grass-lib-$(GRASS_VERSION_NUMBER)
	-cp -rL --parents lib/gis ./grass-lib-$(GRASS_VERSION_NUMBER)
	-cp -rL --parents lib/linkm ./grass-lib-$(GRASS_VERSION_NUMBER)
	-cp -rL --parents lib/form ./grass-lib-$(GRASS_VERSION_NUMBER)
	-cp -rL --parents lib/vector ./grass-lib-$(GRASS_VERSION_NUMBER)
	-cp -rL --parents db/drivers ./grass-lib-$(GRASS_VERSION_NUMBER)

	tar chvfz grass-lib-$(GRASS_VERSION_NUMBER).tar.gz ./grass-lib-$(GRASS_VERSION_NUMBER)/* --exclude=CVS
	-rm -r ./grass-lib-$(GRASS_VERSION_NUMBER)
	@ echo "Distribution source package: grass-lib-$(GRASS_VERSION_NUMBER).tar.gz ready."

.PHONY: strip strip-check
.PHONY: install-check-built install-check-parent install-check-writable install-check-prefix
.PHONY: install real-install install-strip install-macosx
.PHONY: bindist real-bindist bindist-macosx srcdist srclibsdist
