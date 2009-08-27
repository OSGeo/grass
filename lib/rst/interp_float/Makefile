MODULE_TOPDIR = ../../..


LIB = INTERPFL

DEPENDENCIES = $(GISDEP) $(RASTERDEP)

include $(MODULE_TOPDIR)/include/Make/Lib.make

EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

default: $(ARCH_INCDIR)/interpf.h
	$(MAKE) lib

$(ARCH_INCDIR)/interpf.h: interpf.h
	$(INSTALL_DATA) interpf.h $(ARCH_INCDIR)/interpf.h

