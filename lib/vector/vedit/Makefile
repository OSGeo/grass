MODULE_TOPDIR = ../../..

LIB_NAME = $(VEDIT_LIBNAME)

DEPENDENCIES = $(VECTDEP) $(GISDEP)

EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)
EXTRA_LIBS = $(VECTLIB) $(DBMILIB) $(GISLIB) $(MATHLIB)

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

default: lib

# doxygen
DOXNAME=vedit
