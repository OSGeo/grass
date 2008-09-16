MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

LIB_NAME = python_grass7

CFLAGS=-c -fpic -I. $(ARCH_INC) $(GDALCFLAGS) $(PYTHONCFLAGS)

LDFLAGS=-shared -L$(ARCH_LIBDIR) $(GDALLIBS) $(PYTHONLDFLAGS) $(VECTLIB) $(IMAGERYLIB) $(GMATHLIB) $(GISLIB)

#include $(MODULE_TOPDIR)/include/Make/Dir.make

default: _$(LIB_NAME).so numptr

clean:
	-rm -f *.o *.so $(LIB_NAME)_wrap.c $(LIB_NAME).pyc $(LIB_NAME).py

$(LIB_NAME)_wrap.c: $(LIB_NAME).i
	$(SWIG) $(ARCH_INC) -python -shadow $<


$(LIB_NAME)_wrap.o: $(LIB_NAME)_wrap.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $<

_$(LIB_NAME).so: $(LIB_NAME)_wrap.o
	$(LD) $(LDFLAGS) $< -o _$(LIB_NAME).so

numptr:
	$(MAKE) -C NumPtr

.PHONY: numptr

# doxygen:
DOXNAME=
DOXINPUT=grasspython.dox

$(LIB_NAME)_wrap.c: $(ARCH_INCDIR)/gis.h
$(LIB_NAME)_wrap.c: $(ARCH_INCDIR)/gisdefs.h
$(LIB_NAME)_wrap.c: $(ARCH_INCDIR)/imagery.h
$(LIB_NAME)_wrap.c: $(ARCH_INCDIR)/imagedefs.h
$(LIB_NAME)_wrap.c: $(ARCH_INCDIR)/Vect.h
$(LIB_NAME)_wrap.c: $(ARCH_INCDIR)/vect/dig_structs.h
$(LIB_NAME)_wrap.c: $(ARCH_INCDIR)/vect/dig_defines.h
