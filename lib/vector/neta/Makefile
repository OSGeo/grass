MODULE_TOPDIR = ../../..

LIB = NETA

LIBES = $(VECTLIB) $(DBMILIB) $(GISLIB) $(GRAPHLIB)
DEPENDENCIES= $(VECTDEP) $(DBMIDEP) $(GISDEP) 
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

default: lib

DOXNAME=neta
