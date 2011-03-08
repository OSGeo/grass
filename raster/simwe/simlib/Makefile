MODULE_TOPDIR = ../../..

LIBES = $(GISLIB) $(DATETIMELIB) $(GMATHLIB) $(RASTERLIB) $(VECTLIB)
DEPENDENCIES = $(GISDEP) $(BITMAPDEP) $(DBMIDEP) $(GMATHDEP) $(LINKMDEP) $(XDRDEP) $(SITESDEP) $(VECTDEP) $(RASTERDEP) $(VECTDEP)
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

LIB = SIM

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: $(ARCH_INCDIR)/waterglobs.h lib

$(ARCH_INCDIR)/waterglobs.h: waterglobs.h
	cp waterglobs.h $(ARCH_INCDIR)/waterglobs.h
