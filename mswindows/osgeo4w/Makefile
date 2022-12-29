MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Other.make

EXTRA_CLEAN_FILES = setup_x86.hint setup_x86_64.hint

default: setup_x86.hint setup_x86_64.hint

setup_x86.hint: setup_x86.hint.tmpl
	sed \
	-e 's#@GRASS_VERSION_NUMBER@#$(GRASS_VERSION_NUMBER)#' \
	$< > $@

setup_x86_64.hint: setup_x86_64.hint.tmpl
	sed \
	-e 's#@GRASS_VERSION_NUMBER@#$(GRASS_VERSION_NUMBER)#' \
	$< > $@
