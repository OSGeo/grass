
# common dependencies and rules for building libraries

#for i18N support
PACKAGE ="grasslibs"

include $(MODULE_TOPDIR)/include/Make/Vars.make
include $(MODULE_TOPDIR)/include/Make/Rules.make
include $(MODULE_TOPDIR)/include/Make/Html.make
include $(MODULE_TOPDIR)/include/Make/Compile.make

STLIB_NAME = $(LIB_NAME)
STLIB_OBJS = $(ARCH_OBJS)
SHLIB_NAME = $(LIB_NAME)
SHLIB_OBJS = $(ARCH_OBJS)

include $(MODULE_TOPDIR)/include/Make/Stlib.make
include $(MODULE_TOPDIR)/include/Make/Shlib.make

lib: $(GRASS_LIBRARY_TYPE)
	if [ "$(PGM)" != "" -a -f "$(PGM)".html ] ; then $(MAKE) html ; fi
