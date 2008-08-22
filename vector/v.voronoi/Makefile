
MODULE_TOPDIR = ../..

VORONOI = v.voronoi
DELAUNAY = v.delaunay

PROGRAMS = $(VORONOI) $(DELAUNAY)

SWEEP_OBJS = sw_edgelist.o sw_geometry.o sw_heap.o sw_main.o sw_memory.o sw_output.o \
             sw_voronoi.o

v_voronoi_OBJS = vo_main.o vo_extend.o vo_write.o dt_write.o $(SWEEP_OBJS)

v_delaunay_OBJS = dt_main.o vo_extend.o vo_write.o dt_write.o $(SWEEP_OBJS)

LIBES = $(VECTLIB) $(GISLIB)
DEPENDENCIES= $(VECTDEP) $(GISDEP)
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Multi.make

default: multi
