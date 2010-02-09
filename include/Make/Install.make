
# This should be "include"d from the top-level Makefile, and nowhere else

BIN_DIST_FILES = $(FILES) \
	grass$(GRASS_NAME).tmp \
	bin \
	docs \
	driver \
	etc \
	fonts \
	include \
	lib \
	man \
	scripts \
	locale

# Shell commands
MAKE_DIR_CMD	= $(MKDIR) -m 755

strip:
	@ if [ ! -f $(ARCH_BINDIR)/$(GRASS_NAME) ] ; then \
		echo "ERROR: GRASS has not been compiled. Try \"make\" first."; \
		echo "  Strip aborted, exiting Make."; \
		exit; \
	fi; \
	cd $(ARCH_DISTDIR) ; find . -type f -perm +111 -exec strip {} \;

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
	echo $(ARCH_BINDIR)/$(GRASS_NAME)
	@ if [ ! -f $(ARCH_BINDIR)/$(GRASS_NAME) ] ; then \
		echo "ERROR: GRASS has not been compiled. Try \"make\" first."; \
		echo "  Installation aborted, exiting Make."; \
		exit; \
	fi; \
	if [ "$(MACOSX_APP)" = "1" ] ; then \
		$(MAKE) install-macosx; \
		exit; \
	fi; \
	INST_PATH=`dirname $(INST_DIR)`; \
	while [ ! -d $$INST_PATH ]; do \
		INST_PATH=`dirname $$INST_PATH`; \
	done; \
	if [ ! -d "$(INST_DIR)" -a ! -w "$$INST_PATH" ] ; then \
		echo "ERROR: Directory $$INST_PATH is a parent directory of your"; \
		echo "  install directory $(INST_DIR) and is not writable."; \
		echo "  Perhaps you need root access."; \
		echo "  Installation aborted, exiting Make."; \
		exit; \
	fi; \
	if [ -d $(INST_DIR) -a ! -w "$(INST_DIR)" ] ; then \
		echo "ERROR: Your install directory $(INST_DIR) is not writable."; \
		echo "  Perhaps you need root access."; \
		echo "  Installation aborted, exiting Make."; \
		exit; \
	fi; \
	result=`echo "$(INST_DIR)" | awk '{ if ($$1 ~ /grass/) print $$1 }'`; \
	if [ "$$result" = "" ] ; then \
		echo "WARNING: Your install directory $(INST_DIR)"; \
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
	$(MAKE) real-install

real-install:
	test -d $(INST_DIR) || $(MAKE_DIR_CMD) $(INST_DIR)
	test -d $(UNIX_BIN) || $(MAKE_DIR_CMD) $(UNIX_BIN)
	-sed -e "s#^GISBASE.*#GISBASE=$(INST_DIR)#" $(ARCH_BINDIR)/$(GRASS_NAME).sh > $(UNIX_BIN)/$(GRASS_NAME).sh
	-sed -e 's#^gisbase = ".*"#gisbase = "$(INST_DIR)"#' $(ARCH_BINDIR)/$(GRASS_NAME) > $(UNIX_BIN)/$(GRASS_NAME)
	-chmod a+x $(UNIX_BIN)/$(GRASS_NAME)
ifneq ($(strip $(MINGW)),)
	-sed -e "s#WINGISBASE=.*#WINGISBASE=$(INST_DIR)#" $(ARCH_BINDIR)/$(GRASS_NAME).bat > $(UNIX_BIN)/$(GRASS_NAME).bat
	-chmod a+x $(UNIX_BIN)/$(GRASS_NAME).bat
endif
	-tar cBCf $(GISBASE) - . | tar xBCf $(INST_DIR) - 2>/dev/null
	-sed 's#'$(GISBASE)'#'$(INST_DIR)'#g' $(GISBASE)/etc/fontcap > $(INST_DIR)/etc/fontcap
	-$(INSTALL) config.status $(INST_DIR)/config.status
	-chmod -R a+rX $(INST_DIR) 2>/dev/null
	@#GEM installation
	-tar cBf - gem/skeleton | tar xBCf $(INST_DIR)/etc - 2>/dev/null
	-$(INSTALL) gem/gem$(GRASS_VERSION_MAJOR)$(GRASS_VERSION_MINOR) $(UNIX_BIN) 2>/dev/null
	@# enable OSX Help Viewer
	@if [ "`grep -i '^ARCH.*darwin' < include/Make/Platform.make`" ] ; then /bin/ln -sfh "$(INST_DIR)/docs/html" /Library/Documentation/Help/GRASS-$(GRASS_VERSION_MAJOR).$(GRASS_VERSION_MINOR) ; fi


install-strip:
	$(MAKE) strip
	$(MAKE) install

install-macosx:
	$(MAKE) -C macosx install-macosx

bindist:  
	if [ "$(MACOSX_APP)" = "1" ] ; then \
		$(MAKE) bindist-macosx; \
		exit; \
	fi; \
	$(MAKE) real-bindist

real-bindist:
	mkdir -p $(ARCH_DISTDIR)/etc/nad/src ; \
	    cp -f $(MODULE_TOPDIR)/lib/proj/*.lla $(ARCH_DISTDIR)/etc/nad/src ; true
	-cd $(ARCH_DISTDIR); tar cBf - $(BIN_DIST_FILES) | gzip -fc > ../grass-$(GRASS_VERSION_NUMBER)-$(ARCH)-$(DATE).tar.gz
	-name=grass-$(GRASS_VERSION_NUMBER)-$(ARCH)-$(DATE).tar.gz; \
	    size=`ls -l $$name | awk '{print $$5}'`; \
	    sed -e "s/BIN_DIST_VERSION/$(GRASS_VERSION_NUMBER)-$(ARCH)-$(DATE)/" \
	    -e "s/GRASSPRG_NAME/$(GRASS_NAME)/" \
	    -e "s/SIZE_TAR_FILE/$$size/" -e "s#BIN_DIST_DIR#'$(INST_DIR)'#" \
	    -e "s/ARCHITECTURE/$(ARCH)/" \
	    -e "s/LD_LIBRARY_PATH_VAR/$(LD_LIBRARY_PATH_VAR)/" \
	    -e "s/TEST_STR=/TEST_STR=executable/" \
	    -e "s#IMPORTANT.*#Generated from the binaryInstall.src file using the command make bindist#" \
	    -e "s/# executable shell.*//" -e "s/# make bindist.*//" \
	    binaryInstall.src > grass-$(GRASS_VERSION_NUMBER)-$(ARCH)-$(DATE)-install.sh ; \
	    chmod a+x grass-$(GRASS_VERSION_NUMBER)-$(ARCH)-$(DATE)-install.sh 2>/dev/null

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
	tar cvfzh grass-$(GRASS_VERSION_NUMBER).tar.gz ./grass-$(GRASS_VERSION_NUMBER)/* --exclude=CVS
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
	-cp -rL tools ./grass-lib-$(GRASS_VERSION_NUMBER)
	-cp -rL include ./grass-lib-$(GRASS_VERSION_NUMBER)
	-cp -rL --parents lib/external/shapelib ./grass-lib-$(GRASS_VERSION_NUMBER)
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

.PHONY: strip install real-install install-strip install-macosx
.PHONY: bindist real-bindist bindist-macosx srcdist srclibsdist
