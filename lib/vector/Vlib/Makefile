MODULE_TOPDIR = ../../..

EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(ZLIBINCPATH) $(VECT_CFLAGS)
EXTRA_LIBS=$(GISLIB) $(DIG2LIB) $(GRAPHLIB) $(RTREELIB) $(LINKMLIB) $(DBMILIB) $(GDALLIBS) $(GEOSLIBS)

LIB_NAME = $(VECTR_LIBNAME)
DEPENDENCIES =  $(ARCH_INCDIR)/Vect.h $(ARCH_INCDIR)/V_.h \
		$(ARCH_INCDIR)/vect/dig_defines.h \
		$(ARCH_INCDIR)/vect/dig_macros.h $(ARCH_INCDIR)/vect/dig_structs.h \
		$(ARCH_INCDIR)/vect/dig_externs.h $(ARCH_INCDIR)/vect/dig_globs.h $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Lib.make

#compile if LFS (Large File Support) present:
ifneq ($(USE_LARGEFILES),)
	EXTRA_CFLAGS += -D_FILE_OFFSET_BITS=64
endif

default: lib

