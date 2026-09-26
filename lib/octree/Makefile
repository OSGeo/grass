MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Vars.make

LIB = OCTREE

include $(MODULE_TOPDIR)/include/Make/Lib.make

HEADERS := $(ARCH_INCDIR)/octree.h

default: headers
	$(MAKE) lib

headers: $(HEADERS)

$(ARCH_INCDIR)/octree.h: octree.h
	$(INSTALL_DATA) $< $@

DOXNAME = octree
