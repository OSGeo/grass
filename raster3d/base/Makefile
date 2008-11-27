
MODULE_TOPDIR = ../..

LIBES = $(G3DLIB) $(GISLIB)
DEPENDENCIES = $(G3DDEP) $(GISDEP)

PROGRAMS = r3.mask r3.null

r3_mask_OBJS = r3.mask.main.o mask_functions.o
r3_null_OBJS = r3.null.main.o mask_functions.o

include $(MODULE_TOPDIR)/include/Make/Multi.make

default: multi
