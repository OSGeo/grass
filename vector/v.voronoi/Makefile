
MODULE_TOPDIR = ../..

PGM = v.voronoi

SWEEP_OBJS = sw_edgelist.o sw_geometry.o sw_heap.o sw_main.o sw_memory.o sw_output.o \
             sw_voronoi.o

v_voronoi_OBJS = vo_main.o vo_extend.o vo_write.o $(SWEEP_OBJS)

LIBES = $(VECTLIB) $(GISLIB)
DEPENDENCIES= $(VECTDEP) $(GISDEP)
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
