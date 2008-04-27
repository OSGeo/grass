MODULE_TOPDIR = ../../..

PGM=psdriver

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

EXTRA_CFLAGS = -I../../../lib/driver -I../../../lib/psdriver

OBJS =  main.o

ARCH_OBJS := $(foreach obj,$(OBJS),OBJ.$(ARCH)/$(obj))

LIBES = $(PSDRIVERLIB) $(DRIVERLIB) $(GISLIB) $(FTLIB) $(ICONVLIB)
DEPENDENCIES = $(PSDRIVERDEP) $(DRIVERDEP) $(GISDEP)

default: $(DRIVERDIR)/PS $(HTMLDIR)/$(PGM).html moncap

moncap:
	@if [ -f $(ETC)/monitorcap ] && egrep '^PS' $(ETC)/monitorcap >/dev/null 2>&1 ; then \
	echo PS already defined in $(ETC)/monitorcap ; \
	else \
	echo adding PS driver to $(ETC)/monitorcap ; \
	cat moncap.entry >> $(ETC)/monitorcap ; \
	fi

$(DRIVERDIR)/PS: $(ARCH_OBJS) $(DEPENDENCIES)
	$(CC) $(LDFLAGS) -o $@ $(ARCH_OBJS) $(LIBES) $(MATHLIB) $(XDRLIB)
