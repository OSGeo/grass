MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Vars.make

MOD_OBJS := $(filter-out try.o,$(AUTO_OBJS))

LIB = BTREE2

include $(MODULE_TOPDIR)/include/Make/Lib.make

HEADERS := $(ARCH_INCDIR)/kdtree.h

default: headers
	$(MAKE) lib

headers: $(HEADERS)

$(ARCH_INCDIR)/kdtree.h: kdtree.h
	$(INSTALL_DATA) $< $@
