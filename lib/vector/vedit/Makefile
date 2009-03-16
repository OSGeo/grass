MODULE_TOPDIR = ../../..

LIB_NAME = $(VEDIT_LIBNAME)

DEPENDENCIES = $(VECTDEP) $(GISDEP)

EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)
EXTRA_LIBS = $(VECTLIB) $(DBMILIB) $(GISLIB)

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

#compile if LFS (Large File Support) present:
ifneq ($(USE_LARGEFILES),)
	EXTRA_CFLAGS += -D_FILE_OFFSET_BITS=64
endif

default: lib

# doxygen
DOXNAME=vedit
