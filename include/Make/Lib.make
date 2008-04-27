
# common dependencies and rules for building libraries

#for i18N support
PACKAGE ="grasslibs"

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make

ifndef LIB_OBJS
LIB_OBJS = $(subst .c,.o,$(wildcard *.c))
endif

ARCH_LIB_OBJS := $(foreach obj,$(LIB_OBJS),$(OBJDIR)/$(obj))

STLIB_NAME = $(LIB_NAME)
STLIB_OBJS = $(ARCH_LIB_OBJS)
SHLIB_NAME = $(LIB_NAME)
SHLIB_OBJS = $(ARCH_LIB_OBJS)

include $(MODULE_TOPDIR)/include/Make/Rules.make
include $(MODULE_TOPDIR)/include/Make/Stlib.make
include $(MODULE_TOPDIR)/include/Make/Shlib.make

lib: $(GRASS_LIBRARY_TYPE)

