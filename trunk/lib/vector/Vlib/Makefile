MODULE_TOPDIR = ../../..

EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(ZLIBINCPATH) $(PROJINC) $(VECT_CFLAGS)

LIB = VECTOR
DEPENDENCIES =  $(ARCH_INCDIR)/Vect.h $(ARCH_INCDIR)/V_.h \
		$(ARCH_INCDIR)/vect/dig_defines.h \
		$(ARCH_INCDIR)/vect/dig_macros.h $(ARCH_INCDIR)/vect/dig_structs.h \
		$(ARCH_INCDIR)/vect/dig_externs.h $(ARCH_INCDIR)/vect/dig_globs.h \
		$(GISDEP) $(RASTERDEP)

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: lib

