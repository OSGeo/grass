
MODULE_TOPDIR = ../../..

LIB = LRS
DEPENDENCIES = $(GISDEP)
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: lib $(ARCH_INCDIR)/lrs.h

$(ARCH_INCDIR)/lrs.h: lrs.h
	$(INSTALL_DATA) lrs.h $(ARCH_INCDIR)/lrs.h


