MODULE_TOPDIR = ../..

EXTRA_LIBS=$(GISLIB) $(RASTERLIB) $(G3DLIB) $(GMATHLIB) $(MATHLIB)


# GCC >4.2: openMP support
#EXTRA_CFLAGS=-fopenmp
#EXTRA_LIBS=$(GISLIB) $(RASTERLIB) $(G3DLIB) -lgomp $(GMATHLIB) $(MATHLIB)
# ?? EXTRA_LIBS += -lgomp


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
