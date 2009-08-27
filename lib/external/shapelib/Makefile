MODULE_TOPDIR = ../../..

LIB = SHAPE

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: headers
	$(MAKE) lib

headers: $(ARCH_INCDIR)/shapefil.h

$(ARCH_INCDIR)/%.h: %.h
	$(INSTALL_DATA) $< $@
