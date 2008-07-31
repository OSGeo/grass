MODULE_TOPDIR = ../../..

LIB_NAME = $(GRAPH_LIBNAME)

LIB_OBJS = \
	avl.o \
	tavl.o \
	graph.o \
	heap.o \
	tree.o \
	helpers.o \
	graph_v1.o \
	graph_v2.o

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

DGLINC = $(ARCH_INCDIR)/dgl

default: headers
	$(MAKE) lib

headers: $(DGLINC)/avl.h $(DGLINC)/tavl.h $(DGLINC)/graph.h $(DGLINC)/heap.h \
	 $(DGLINC)/tree.h $(DGLINC)/type.h $(DGLINC)/helpers.h $(DGLINC)/graph_v1.h $(DGLINC)/graph_v2.h \
	 $(ARCH_INCDIR)/dgl.h

$(DGLINC)/%.h: %.h
	-test -d $(DGLINC) || $(MKDIR) $(DGLINC)
	$(INSTALL_DATA) $^ $@

$(ARCH_INCDIR)/dgl.h: dgl.h
	$(INSTALL_DATA) $^ $@

#doxygen:
DOXNAME=dg
