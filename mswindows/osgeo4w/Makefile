MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Other.make

EXTRA_CLEAN_FILES = setup.hint

ifneq ($(MINGW),)
default: setup.hint
else
default:
endif

setup.hint: setup.hint.tmpl
	sed \
	-e 's#@GRASS_VERSION_NUMBER@#$(GRASS_VERSION_NUMBER)#' \
	$< > $@
