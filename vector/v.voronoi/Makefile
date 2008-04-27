
MODULE_TOPDIR = ../..

VORONOI = v.voronoi
DELAUNAY = v.delaunay

PROGRAMS = $(VORONOI) $(DELAUNAY)

SWEEP_OBJS = sw_edgelist.o sw_geometry.o sw_heap.o sw_main.o sw_memory.o sw_output.o \
             sw_voronoi.o

VORONOI_OBJS = vo_main.o vo_extend.o vo_write.o dt_write.o $(SWEEP_OBJS)

DELAUNAY_OBJS = dt_main.o vo_extend.o vo_write.o dt_write.o $(SWEEP_OBJS)


LIBES = $(VECTLIB) $(GISLIB)
DEPENDENCIES= $(VECTDEP) $(GISDEP)
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Multi.make

VORONOI_ARCH_OBJS := $(foreach obj,$(VORONOI_OBJS),$(OBJDIR)/$(obj))

DELAUNAY_ARCH_OBJS := $(foreach obj,$(DELAUNAY_OBJS),$(OBJDIR)/$(obj))

default: $(BIN)/$(DELAUNAY)$(EXE) $(BIN)/$(VORONOI)$(EXE)
	$(MAKE) htmlmulti

$(BIN)/$(VORONOI)$(EXE): $(VORONOI_ARCH_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(MATHLIB) $(XDRLIB)

$(BIN)/$(DELAUNAY)$(EXE):$(DELAUNAY_ARCH_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(MATHLIB) $(XDRLIB)

