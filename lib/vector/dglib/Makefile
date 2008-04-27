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

default: $(DGLINC)/avl.h $(DGLINC)/tavl.h $(DGLINC)/graph.h $(DGLINC)/heap.h \
	 $(DGLINC)/tree.h $(DGLINC)/type.h $(DGLINC)/helpers.h $(DGLINC)/graph_v1.h $(DGLINC)/graph_v2.h \
	 $(ARCH_INCDIR)/dgl.h lib

$(DGLINC)/avl.h: avl.h
	$(MKDIR) $(DGLINC)
	$(INSTALL_DATA) avl.h $(DGLINC)/avl.h

$(DGLINC)/tavl.h: tavl.h
	$(INSTALL_DATA) tavl.h $(DGLINC)/tavl.h

$(DGLINC)/heap.h: heap.h
	$(INSTALL_DATA) heap.h $(DGLINC)/heap.h

$(DGLINC)/graph.h: graph.h
	$(INSTALL_DATA) graph.h $(DGLINC)/graph.h

$(DGLINC)/tree.h: tree.h
	$(INSTALL_DATA) tree.h $(DGLINC)/tree.h

$(DGLINC)/type.h: type.h
	$(INSTALL_DATA) type.h $(DGLINC)/type.h

$(DGLINC)/graph_v1.h: graph_v1.h
	$(INSTALL_DATA) graph_v1.h $(DGLINC)/graph_v1.h

$(DGLINC)/graph_v2.h: graph_v2.h
	$(INSTALL_DATA) graph_v2.h $(DGLINC)/graph_v2.h

$(DGLINC)/helpers.h: helpers.h
	$(INSTALL_DATA) helpers.h $(DGLINC)/helpers.h

$(ARCH_INCDIR)/dgl.h: dgl.h
	$(INSTALL_DATA) dgl.h $(ARCH_INCDIR)/dgl.h

#doxygen:
DOXNAME=dg
