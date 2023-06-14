MODULE_TOPDIR = ../../..

LIB = PARSON

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: $(ARCH_INCDIR)/parson.h
	$(MAKE) lib

$(ARCH_INCDIR)/parson.h: parson.h
	$(INSTALL_DATA) parson.h $(ARCH_INCDIR)/parson.h
