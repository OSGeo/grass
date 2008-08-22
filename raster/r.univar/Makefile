
MODULE_TOPDIR = ../..

LIBES = $(G3DLIB) $(GISLIB)
DEPENDENCIES = $(G3DDEP) $(GISDEP)

PROGRAMS = r.univar r3.univar

r_univar_OBJS = r.univar_main.o sort.o stats.o
r3_univar_OBJS = r3.univar_main.o sort.o stats.o

include $(MODULE_TOPDIR)/include/Make/Multi.make

default: multi
