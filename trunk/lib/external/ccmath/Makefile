MODULE_TOPDIR = ../../..

LIB = CCMATH

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: $(ARCH_INCDIR)/ccmath_grass.h
	$(MAKE) lib

$(ARCH_INCDIR)/ccmath_grass.h: ccmath.h
	$(INSTALL_DATA) ccmath.h $(ARCH_INCDIR)/ccmath_grass.h

#doxygen:
DOXNAME=ccmath
