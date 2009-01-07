MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

CFLAGS=-c -fpic -I. $(ARCH_INC) $(GDALCFLAGS) $(PYTHONCFLAGS)
LDFLAGS=-shared -L$(ARCH_LIBDIR) $(GDALLIBS) $(PYTHONLDFLAGS)

MODULES = grass math imagery vector proj raster display stats

vector_LIBS = $(VECTLIB)
imagery_LIBS = $(IMAGERYLIB)
grass_LIBS = $(GISLIB)
math_LIBS = $(GMATHLIB)
proj_LIBS = $(GPROJLIB)
raster_LIBS = $(RASTERLIB)
display_LIBS = $(DISPLAYLIB)
stats_LIBS = $(STATSLIB)

EXTRA_CLEAN_FILES := $(foreach M,$(MODULES),$(M)_wrap.o $(M)_wrap.c $(M).pyc $(M).py)
CLEAN_SUBDIRS = NumPtr

DSTDIR = $(ARCH_DISTDIR)/etc/python/swig

LIBFILES := $(patsubst %,$(DSTDIR)/_%.so,$(MODULES))
PYFILES  := $(patsubst %,$(DSTDIR)/%.py,$(MODULES))
PYCFILES := $(patsubst %,$(DSTDIR)/%.pyc,$(MODULES))

default: $(LIBFILES) $(PYFILES) $(DSTDIR)/__init__.pyc numptr

%_wrap.c %.py: %.i my_typemaps.i renames.i common.i
	$(SWIG) $(ARCH_INC) -python -shadow $<

$(OBJDIR)/%_wrap.o: %_wrap.c
	test -d $(OBJDIR) || $(MKDIR) -p $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -o $@ $<

$(DSTDIR)/_%.so: $(OBJDIR)/%_wrap.o
	test -d $(DSTDIR) || $(MKDIR) -p $(DSTDIR)
	$(SHLIB_LD) -o $@ $(LDFLAGS) $^ $($*_LIBS)

$(DSTDIR)/%.py: %.py
	test -d $(DSTDIR) || $(MKDIR) -p $(DSTDIR)
	$(INSTALL_DATA) $< $@

$(DSTDIR)/%.pyc: $(DSTDIR)/%.py $(DSTDIR)/_%.so
	cd $(DSTDIR) && echo "import $*" | python; true

$(DSTDIR)/__init__.pyc: $(DSTDIR)/__init__.py $(LIBFILES) $(PYCFILES)
	cd $(DSTDIR) && echo "import __init__" | python; true

numptr:
	$(MAKE) -C NumPtr

.PHONY: numptr

# doxygen:
DOXNAME=
DOXINPUT=grasspython.dox

display_wrap.c: $(ARCH_INCDIR)/display.h
grass_wrap.c: $(ARCH_INCDIR)/gis.h
grass_wrap.c: $(ARCH_INCDIR)/gisdefs.h
imagery_wrap.c: $(ARCH_INCDIR)/imagedefs.h
imagery_wrap.c: $(ARCH_INCDIR)/imagery.h
math_wrap.c: $(ARCH_INCDIR)/gmath.h
proj_wrap.c: $(ARCH_INCDIR)/gprojects.h
raster_wrap.c: $(ARCH_INCDIR)/raster.h
stats_wrap.c: $(ARCH_INCDIR)/stats.h
vector_wrap.c: $(ARCH_INCDIR)/Vect.h
vector_wrap.c: $(ARCH_INCDIR)/vect/dig_defines.h
vector_wrap.c: $(ARCH_INCDIR)/vect/dig_structs.h
