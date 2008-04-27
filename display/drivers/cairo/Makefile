MODULE_TOPDIR = ../../..

PGM=cairodriver

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

EXTRA_CFLAGS = -I../../../lib/driver -I../../../lib/cairodriver $(CAIROINC)

OBJS = main.o

ARCH_OBJS := $(foreach obj,$(OBJS),OBJ.$(ARCH)/$(obj))

LIBES = $(CAIRODRIVERLIB) $(DRIVERLIB) $(GISLIB) $(FTLIB) $(ICONVLIB)
DEPENDENCIES = $(CAIRODRIVERDEP) $(DRIVERDEP) $(GISDEP)

default: $(DRIVERDIR)/cairo $(HTMLDIR)/$(PGM).html moncap

moncap:
	@if [ -f $(ETC)/monitorcap ] && egrep '^cairo' $(ETC)/monitorcap >/dev/null 2>&1 ; then \
	echo cairo already defined in $(ETC)/monitorcap ; \
	else \
	echo adding cairo driver to $(ETC)/monitorcap ; \
	cat moncap.entry >> $(ETC)/monitorcap ; \
	fi

$(DRIVERDIR)/cairo: $(ARCH_OBJS) $(DEPENDENCIES)
	$(CC) $(LDFLAGS) -o $@ $(ARCH_OBJS) $(LIBES) $(MATHLIB) $(XDRLIB)
