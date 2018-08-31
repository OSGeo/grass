MODULE_TOPDIR = ../../..

LIB = INTERPDATA
DEPENDENCIES = $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: $(ARCH_INCDIR)/dataquad.h
	$(MAKE) lib

$(ARCH_INCDIR)/dataquad.h: dataquad.h
	$(INSTALL_DATA) dataquad.h $(ARCH_INCDIR)/dataquad.h
