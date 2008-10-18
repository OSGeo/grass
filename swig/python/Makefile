MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

LIB_NAME = python_grass7

CFLAGS=-c -fpic -I. $(ARCH_INC) $(GDALCFLAGS) $(PYTHONCFLAGS)
LDFLAGS=-shared -L$(ARCH_LIBDIR) $(GDALLIBS) $(PYTHONLDFLAGS) $(VECTLIB) $(IMAGERYLIB) $(GMATHLIB) $(GISLIB)


EXTRA_CLEAN_FILES = $(LIB_NAME)_wrap.o _$(LIB_NAME).so $(LIB_NAME)_wrap.c $(LIB_NAME).pyc $(LIB_NAME).py
CLEAN_SUBDIRS = NumPtr

default: _$(LIB_NAME).so numptr

%_wrap.c: %.i my_typemaps.i renames.i
	$(SWIG) $(ARCH_INC) -python -shadow $<

%_wrap.o: %_wrap.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $<

_%.so: %_wrap.o
	$(LD) $(LDFLAGS) $< -o $@
	# build bytecode .pyc file; ignore undefined symbol error
	echo "import $*" | python; true

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
