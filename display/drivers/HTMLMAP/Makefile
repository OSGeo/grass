MODULE_TOPDIR = ../../..

PGM=htmlmapdriver

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

EXTRA_CFLAGS=-I../../../lib/driver

OBJS =  \
	Graph_Clse.o\
	Graph_Set.o\
	Polygn_abs.o\
	Text.o\
	main.o

ARCH_OBJS := $(foreach obj,$(OBJS),OBJ.$(ARCH)/$(obj))

LIBES = $(DRIVERLIB) $(DISPLAYLIB) $(RASTERLIB) $(GISLIB) $(FTLIB) $(ICONVLIB)
DEPENDENCIES = $(DRIVERLIBDEP) $(DRIVERDEP) $(DISPLAYDEP) $(RASTERDEP) $(GISLIBDEP)

default: $(DRIVERDIR)/HTMLMAP $(HTMLDIR)/$(PGM).html moncap

moncap:
	@if [ -f $(ETC)/monitorcap ] && egrep '^HTMLMAP' $(ETC)/monitorcap >/dev/null 2>&1 ; then \
	echo HTMLMAP already defined in $(ETC)/monitorcap ; \
	else \
	echo adding HTMLMAP driver to $(ETC)/monitorcap ; \
	cat moncap.entry >> $(ETC)/monitorcap ; \
	fi

$(DRIVERDIR)/HTMLMAP: $(ARCH_OBJS) $(DEPENDENCIES)
	$(CC) $(LDFLAGS) -o $@ $(ARCH_OBJS) $(LIBES) $(MATHLIB) $(XDRLIB)
