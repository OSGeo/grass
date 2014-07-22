MODULE_TOPDIR = ../..

LIB = NVIZ

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

ifneq ($(OPENGL_X11),)
EXTRA_CFLAGS += $(XCFLAGS)
endif

EXTRA_INC = $(VECT_INC) $(OPENGLINC) $(TIFFINCPATH)

EXTRA_CFLAGS = $(VECT_CFLAGS)

ifneq ($(USE_OPENGL),)
default: lib
endif

# doxygen
DOXNAME=nviz
