MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Vars.make

MOD_OBJS := $(filter-out %-template.o,$(AUTO_OBJS))

LIB = GRAPH

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

DGLINC = $(ARCH_INCDIR)/dgl

default: headers
	$(MAKE) lib

headers: $(DGLINC)/avl.h $(DGLINC)/tavl.h $(DGLINC)/graph.h $(DGLINC)/heap.h \
	 $(DGLINC)/tree.h $(DGLINC)/type.h $(DGLINC)/helpers.h $(DGLINC)/graph_v1.h $(DGLINC)/graph_v2.h \
	 $(ARCH_INCDIR)/dgl.h

$(DGLINC)/%.h: %.h | $(DGLINC)
	$(INSTALL_DATA) $< $@

$(DGLINC):
	$(MKDIR) $@

.PHONY: headers

#doxygen:
DOXNAME=dg
