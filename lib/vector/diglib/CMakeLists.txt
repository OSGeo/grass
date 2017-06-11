# MODULE_TOPDIR = ../../..

# include $(MODULE_TOPDIR)/include/Make/Vars.make

# MOD_OBJS := $(filter-out test.o port_test.o,$(AUTO_OBJS))

# LIB = DIG2

# include $(MODULE_TOPDIR)/include/Make/Lib.make

# EXTRA_INC = $(VECT_INC)
# EXTRA_CFLAGS = $(VECT_CFLAGS)
# LIBES = $(GISLIB) $(RTREELIB) $(MATHLIB)

# #compile if LFS (Large File Support) present:
# ifneq ($(USE_LARGEFILES),)
# 	TESTFILE = test64.ok
# else
# 	TESTFILE = test32.ok
# endif

# default: lib
# 	@echo "==============TEST============="
# ifeq ($(strip $(CROSS_COMPILING)),)
# 	$(MAKE) test
# endif

# test: $(OBJDIR)/test.tmp $(TESTFILE)
# 	cmp $(OBJDIR)/test.tmp $(TESTFILE)

# $(OBJDIR)/test.tmp: $(OBJDIR)/test$(EXE)
# 	cd $(OBJDIR) && $(call run_grass,./test)

# #  Test portable read/write functions
# $(OBJDIR)/test$(EXE):  $(OBJDIR)/test.o $(DIG2DEP)
# 	$(call linker)

# $(OBJDIR)/test$(EXE): LIBES = $(DIG2LIB)


set(dig2_SRCS
allocation.c   cindex.c       frmt.c         line_dist.c    plus.c         plus_node.c    port_init.c    prune.c        struct_alloc.c update.c
angle.c        cindex_rw.c    head.c         linecros.c     plus_area.c    plus_struct.c      spindex.c      
box.c          file.c         inside.c       list.c         plus_line.c    poly.c         portable.c     spindex_rw.c   type.c
)



build_module(NAME grass_dig2
  SOURCES ${dig2_SRCS}
  DEPENDS GDAL GEOS grass_gmath grass_gis grass_rtree)
