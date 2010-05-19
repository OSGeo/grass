MODULE_TOPDIR = ../../..

DEPENDENCIES= $(VECTDEP) $(DBMIDEP) $(GISDEP) $(RASTERDEP) $(BSPLINE_ARCH_OBJ) $(GMATHDEP)
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

LIB = LIDAR

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: $(ARCH_INCDIR)/lidar.h
	$(MAKE) lib

$(ARCH_INCDIR)/lidar.h: lidar.h
	$(INSTALL_DATA) lidar.h $(ARCH_INCDIR)/lidar.h
