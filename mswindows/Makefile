MODULE_TOPDIR = ..

SUBDIRS = \
	osgeo4w

include $(MODULE_TOPDIR)/include/Make/Dir.make

EXTRA_CLEAN_FILES = GRASS-Packager.bat GRASS-Installer.nsi

ifneq ($(MINGW),)
default: GRASS-Packager.bat GRASS-Installer.nsi parsubdirs
else
default:
endif	

GRASS-Packager.bat: GRASS-Packager.bat.tmpl
	sed \
	-e 's#@GRASS_VERSION_MAJOR@#$(GRASS_VERSION_MAJOR)#' \
	-e 's#@GRASS_VERSION_MINOR@#$(GRASS_VERSION_MINOR)#' \
	$< > $@

GRASS-Installer.nsi: GRASS-Installer.nsi.tmpl
	sed \
	-e 's#@GRASS_VERSION_SVN@#$(GRASS_VERSION_SVN)#' \
	-e 's#@GRASS_VERSION_MAJOR@#$(GRASS_VERSION_MAJOR)#' \
	-e 's#@GRASS_VERSION_MINOR@#$(GRASS_VERSION_MINOR)#' \
	-e 's#@GRASS_VERSION_RELEASE@#$(GRASS_VERSION_RELEASE)#' \
	-e 's#@ARCH@#$(ARCH)#' \
	$< > $@
