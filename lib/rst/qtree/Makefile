MODULE_TOPDIR = ../../..

LIB_NAME = $(QTREE_LIBNAME)
DEPENDENCIES = $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: $(ARCH_INCDIR)/qtree.h lib

$(ARCH_INCDIR)/qtree.h: qtree.h
	$(INSTALL_DATA) qtree.h $(ARCH_INCDIR)/qtree.h
