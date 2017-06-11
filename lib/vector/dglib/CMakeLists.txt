# MODULE_TOPDIR = ../../..

# include $(MODULE_TOPDIR)/include/Make/Vars.make

# MOD_OBJS := $(filter-out %-template.o,$(AUTO_OBJS))

# LIB = GRAPH

# include $(MODULE_TOPDIR)/include/Make/Lib.make
# include $(MODULE_TOPDIR)/include/Make/Doxygen.make

# DGLINC = $(ARCH_INCDIR)/dgl

# default: headers
# 	$(MAKE) lib

# headers: $(DGLINC)/avl.h $(DGLINC)/tavl.h $(DGLINC)/graph.h $(DGLINC)/heap.h \
# 	 $(DGLINC)/tree.h $(DGLINC)/type.h $(DGLINC)/helpers.h $(DGLINC)/graph_v1.h $(DGLINC)/graph_v2.h \
# 	 $(ARCH_INCDIR)/dgl.h

# $(DGLINC)/%.h: %.h | $(DGLINC)
# 	$(INSTALL_DATA) $< $@

set(DGL_headers
  avl.h
  tavl.h
  graph.h
  heap.h
  tree.h
  type.h
  helpers.h
  graph_v1.h
  graph_v2.h
)
foreach(DGL_header ${DGL_headers})
  file(COPY ${DGL_header} DESTINATION  ${CMAKE_BINARY_DIR}/include/grass/dgl)
endforeach()

set(graphlib_SRCS
  avl.c
  graph.c
  graph_v2.c
  helpers.c
  tree.c
  graph_v1.c
  heap.c
  tavl.c
  )

build_module(
  NAME grass_graph
  SOURCES "${graphlib_SRCS}"
  HEADERS "dgl.h"
  )


