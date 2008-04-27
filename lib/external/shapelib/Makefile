MODULE_TOPDIR = ../../..

LIB_NAME = $(SHAPE_LIBNAME)

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: $(ARCH_INCDIR)/shapefil.h
	$(MAKE) lib

$(ARCH_INCDIR)/shapefil.h: shapefil.h
	$(INSTALL_DATA) shapefil.h $(ARCH_INCDIR)/shapefil.h
