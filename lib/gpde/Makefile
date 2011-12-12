MODULE_TOPDIR = ../..

EXTRA_LIBS=$(GISLIB) $(RASTERLIB) $(RASTER3DLIB) $(GMATHLIB) $(OMPLIB) $(MATHLIB)
EXTRA_CFLAGS = $(OMPCFLAGS)
# needed? $(OMPLIBPATH)

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
