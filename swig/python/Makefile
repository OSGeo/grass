MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

CFLAGS=-c -fpic -I. $(ARCH_INC) $(GDALCFLAGS) $(PYTHONCFLAGS)

LDFLAGS=-shared -L$(ARCH_LIBDIR) $(GDALLIBS) $(PYTHONLDFLAGS) $(VECTLIB) $(IMAGERYLIB) $(GISLIB)

# TODO: build the NumPtr module. See NumPtr/README.GRASS
#SUBDIRS = NumPtr
#include $(MODULE_TOPDIR)/include/Make/Dir.make

default: python_grass7.so numptr

clean:
	-rm -f *.o *.so python_grass7_wrap.c python_grass7.pyc python_grass7.py

python_grass7_wrap.c: python_grass7.i
	$(SWIG) $(ARCH_INC) -python -shadow $<


python_grass7_wrap.o: python_grass7_wrap.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $<

python_grass7.so: python_grass7_wrap.o
	$(LD) $(LDFLAGS) $< -o _python_grass7.so

numptr:
	$(MAKE) -C NumPtr

# doxygen:
DOXNAME=
DOXINPUT=grasspython.dox
