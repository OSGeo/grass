MODULE_TOPDIR = ../../..

LIB_NAME = $(SHAPE_LIBNAME)

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: headers
	$(MAKE) lib

headers: $(ARCH_INCDIR)/shapefil.h

$(ARCH_INCDIR)/%.h: %.h
	$(INSTALL_DATA) $< $@
