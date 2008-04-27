
MODULE_TOPDIR = ../../..

LIB_NAME = $(LRS_LIBNAME)
DEPENDENCIES = $(GISDEP)
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)
EXTRA_LIBS = $(VECTLIB) $(DBMILIB) $(GISLIB)

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: lib $(ARCH_INCDIR)/lrs.h

$(ARCH_INCDIR)/lrs.h: lrs.h
	$(INSTALL_DATA) lrs.h $(ARCH_INCDIR)/lrs.h


