# MODULE_TOPDIR = ../../..

# include $(MODULE_TOPDIR)/include/Make/Vars.make

# MOD_OBJS := $(filter-out %-template.o,$(AUTO_OBJS))

# LIB = GRAPH

# include $(MODULE_TOPDIR)/include/Make/Lib.make include
# $(MODULE_TOPDIR)/include/Make/Doxygen.make

# DGLINC = $(ARCH_INCDIR)/dgl

# default: headers $(MAKE) lib

# headers: $(DGLINC)/avl.h $(DGLINC)/tavl.h $(DGLINC)/graph.h $(DGLINC)/heap.h \
# $(DGLINC)/tree.h $(DGLINC)/type.h $(DGLINC)/helpers.h $(DGLINC)/graph_v1.h
# $(DGLINC)/graph_v2.h \ $(ARCH_INCDIR)/dgl.h

# $(DGLINC)/%.h: %.h | $(DGLINC) $(INSTALL_DATA) $< $@

set(DGL_headers
    avl.h
    graph.h
    graph_v1.h
    graph_v2.h
    heap.h
    helpers.h
    tavl.h
    tree.h
    type.h)
foreach(DGL_header ${DGL_headers})
  file(COPY ${DGL_header}
       DESTINATION ${OUTDIR}/${GRASS_INSTALL_INCLUDEDIR}/grass/dgl)
endforeach()
# file(COPY dgl.h DESTINATION ${OUTDIR}/${GRASS_INSTALL_INCLUDEDIR}/grass)

set(graphlib_SRCS
    avl.c
    graph.c
    graph_v1.c
    graph_v2.c
    heap.c
    helpers.c
    tavl.c
    tree.c)

build_module(NAME grass_dgl SOURCES "${graphlib_SRCS}" HEADERS "dgl.h")
