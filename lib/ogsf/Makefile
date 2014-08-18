MODULE_TOPDIR = ../..


LIB = OGSF

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

EXTRA_INC = $(VECT_INC) $(OPENGLINC) $(TIFFINCPATH)
EXTRA_CFLAGS = $(VECT_CFLAGS) $(XCFLAGS)

ifneq ($(USE_OPENGL),)
default: lib
endif

#doxygen:
DOXNAME=ogsf
