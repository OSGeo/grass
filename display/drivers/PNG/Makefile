MODULE_TOPDIR = ../../..

PGM=pngdriver

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

EXTRA_CFLAGS = $(PNGINC) -I../../../lib/driver -I../../../lib/pngdriver

OBJS =  main.o

ARCH_OBJS := $(foreach obj,$(OBJS),OBJ.$(ARCH)/$(obj))

LIBES = $(PNGDRIVERLIB) $(DRIVERLIB) $(GISLIB) $(FTLIB) $(ICONVLIB)
DEPENDENCIES = $(PNGDRIVERDEP) $(DRIVERDEP) $(GISLIBDEP)

default: $(DRIVERDIR)/PNG $(HTMLDIR)/$(PGM).html moncap

moncap:
	@if [ -f $(ETC)/monitorcap ] && egrep '^PNG' $(ETC)/monitorcap >/dev/null 2>&1 ; then \
	echo PNG already defined in $(ETC)/monitorcap ; \
	else \
	echo adding PNG driver to $(ETC)/monitorcap ; \
	cat moncap.entry >> $(ETC)/monitorcap ; \
	fi

$(DRIVERDIR)/PNG: $(ARCH_OBJS) $(DEPENDENCIES)
	$(CC) $(LDFLAGS) -o $@ $(ARCH_OBJS) $(LIBES) $(PNGLIB) $(MATHLIB) $(XDRLIB)
