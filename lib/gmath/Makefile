MODULE_TOPDIR = ../..

EXTRA_LIBS = $(OPENMP_LIBPATH) $(OPENMP_LIB)
EXTRA_INC = $(OPENMP_INCPATH) $(BLASINC) $(LAPACKINC)
EXTRA_CFLAGS = $(FFTWINC) $(OPENMP_CFLAGS)

LIB = GMATH

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

default: lib

#doxygen:
DOXNAME=gmath
