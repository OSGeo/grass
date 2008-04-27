MODULE_TOPDIR = ../..

EXTRA_LIBS=$(GISLIB) $(FFTWLIB) $(LAPACKLIB) $(BLASLIB)

EXTRA_CFLAGS = $(FFTWINC) $(PICFLAGS)

LIB_NAME = $(GMATH_LIBNAME)

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

default: lib

#doxygen:
DOXNAME=gmath
