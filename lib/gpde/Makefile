MODULE_TOPDIR = ../..

EXTRA_LIBS=$(GISLIB) $(RASTERLIB) $(RASTER3DLIB) $(GMATHLIB) $(OPENMP_LIBPATH) $(OPENMP_LIB) $(MATHLIB)
EXTRA_CFLAGS = $(OPENMP_CFLAGS)
EXTRA_INC = $(OPENMP_INCPATH)

LIB_NAME = $(GPDE_LIBNAME)

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

HEADERS := $(patsubst %.h,$(ARCH_INCDIR)/%.h,$(wildcard N_*.h))

default: $(HEADERS)
	$(MAKE) lib

$(ARCH_INCDIR)/%.h: %.h
	$(INSTALL_DATA) $< $@

#doxygen:
DOXNAME=gpde
