
MODULE_TOPDIR = ../..

LIBES = $(G3DLIB) $(GISLIB)
DEPENDENCIES = $(G3DDEP) $(GISDEP)

PROGRAMS = r3.info r3.mask r3.null r3.timestamp

r3_mask_OBJS = r3.mask.main.o mask_functions.o
r3_null_OBJS = r3.null.main.o mask_functions.o
r3_info_OBJS = r3.info.main.o
r3_timestamp_OBJS = r3.timestamp.main.o

include $(MODULE_TOPDIR)/include/Make/Multi.make

default: multi
