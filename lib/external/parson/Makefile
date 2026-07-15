MODULE_TOPDIR = ../../..

LIB = PARSON

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: headers
	$(MAKE) lib

headers: $(ARCH_INCDIR)/gjson.h

$(ARCH_INCDIR)/%.h: %.h
	$(INSTALL_DATA) $< $@
