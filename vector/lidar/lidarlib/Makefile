MODULE_TOPDIR = ../../..

DEPENDENCIES= $(VECTDEP) $(DBMIDEP) $(GISDEP) $(RASTERDEP) $(BSPLINE_ARCH_OBJ) $(GMATHDEP)
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

LIB = LIDAR

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: $(ARCH_INCDIR)/PolimiFunct.h lib

$(ARCH_INCDIR)/PolimiFunct.h: PolimiFunct.h
	cp PolimiFunct.h $(ARCH_INCDIR)/PolimiFunct.h
