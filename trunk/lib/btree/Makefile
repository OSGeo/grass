MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Vars.make

MOD_OBJS := $(filter-out try.o,$(AUTO_OBJS))

LIB = BTREE

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: lib
