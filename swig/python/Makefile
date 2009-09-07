MODULE_TOPDIR = ../..

PACKAGE = "grasslibs"

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Compile.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

# substitute OSX arch flags for python
ifneq ($(MACOSX_ARCHS),)
CFLAGS := $(subst $(MACOSX_ARCHS),,$(CFLAGS)) $(MACOSX_ARCHS_PYTHON)
LDFLAGS := $(subst $(MACOSX_ARCHS),,$(LDFLAGS)) $(MACOSX_ARCHS_PYTHON)
endif

# doxygen:
DOXNAME=
DOXINPUT=grasspython.dox

MODULES = utils date grass raster math proj imagery vector display stats \
	dbmi g3d arraystats cluster trans vedit

date_LIBS       = $(DATETIMELIB)
grass_LIBS      = $(GISLIB)
raster_LIBS     = $(RASTERLIB)
math_LIBS       = $(GMATHLIB)
proj_LIBS       = $(GPROJLIB)
imagery_LIBS    = $(IMAGERYLIB)
vector_LIBS     = $(VECTLIB)
display_LIBS    = $(DISPLAYLIB)
stats_LIBS      = $(STATSLIB)
dbmi_LIBS       = $(DBMILIB)
g3d_LIBS        = $(G3DLIB)
arraystats_LIBS = $(ARRAYSTATSLIB)
cluster_LIBS    = $(CLUSTERLIB)
trans_LIBS      = $(TRANSLIB)
vedit_LIBS      = $(VEDITLIB)

vector_wrap_c_FLAGS = $(VECT_INC) $(VECT_CFLAGS) $(GEOSCFLAGS)
vedit_wrap_c_FLAGS = $(VECT_INC) $(VECT_CFLAGS)
proj_wrap_c_FLAGS = $(PROJINC) $(GDALCFLAGS)

EXTRA_SWIG = ../include/python/my_typemaps.i ../include/python/common.i
SWIGFLAGS = $(INC) -I../include/python -outdir .
EXTRA_CFLAGS = $(PYMOD_CFLAGS)
EXTRA_CLEAN_FILES := $(foreach M,$(MODULES),$(M)_wrap.o $(M)_wrap.c $(M).pyc $(M).py _$(M).so)
CLEAN_SUBDIRS = NumPtr

include $(MODULE_TOPDIR)/include/Make/Python.make

PYDIR = $(ETC)/python
GDIR = $(PYDIR)/grass
DSTDIR = $(GDIR)/lib

LIBFILES := $(patsubst %,$(DSTDIR)/_%.so,$(MODULES))
PYFILES  := $(patsubst %,$(DSTDIR)/%.py,$(MODULES) __init__)
LLIBFILES := $(patsubst %,_%.so,$(MODULES))
WRAPFILES := $(patsubst %,%_wrap.c,$(MODULES))
LPYFILES := $(patsubst %,%.py,$(MODULES))

default:
	$(MAKE) $(DSTDIR)
	$(MAKE) $(LLIBFILES) $(WRAPFILES) $(LPYFILES) $(LIBFILES) $(PYFILES)

%_wrap.c %.py: ../include/%.i $(EXTRA_SWIG)
	$(SWIG) $(SWIGFLAGS) -python -o $*_wrap.c -module $* -shadow $<

%_wrap.c %.py: ../include/python/%.i $(EXTRA_SWIG)
	$(SWIG) $(SWIGFLAGS) -python -o $*_wrap.c -module $* -shadow $<

$(PYDIR):
	test -d $@ || $(MKDIR) -p $@

$(GDIR): $(PYDIR)
	test -d $@ || $(MKDIR) -p $@

$(DSTDIR): $(GDIR)
	test -d $@ || $(MKDIR) -p $@

$(DSTDIR)/_%.so: _%.so
	$(INSTALL) $< $@

$(DSTDIR)/%.py: %.py
	$(INSTALL_DATA) $< $@

numptr:
	$(MAKE) -C NumPtr

.PHONY: numptr

.SECONDARY: $(patsubst %,%.py,$(MODULES))

.SECONDARY: $(patsubst %,%_wrap.c,$(MODULES))

.SECONDARY: $(patsubst %,$(OBJDIR)/%_wrap.o,$(MODULES))

.SECONDARY: $(patsubst %,_%.so,$(MODULES))

date_wrap.c: $(GISBASE)/include/grass/datetime.h
date_wrap.c: $(GISBASE)/include/grass/P_datetime.h
grass_wrap.c: $(GISBASE)/include/grass/gis.h
grass_wrap.c: $(GISBASE)/include/grass/gisdefs.h
raster_wrap.c: $(GISBASE)/include/grass/raster.h
math_wrap.c: $(GISBASE)/include/grass/gmath.h
proj_wrap.c: $(GISBASE)/include/grass/gprojects.h
imagery_wrap.c: $(GISBASE)/include/grass/imagedefs.h
imagery_wrap.c: $(GISBASE)/include/grass/imagery.h
vector_wrap.c: $(GISBASE)/include/grass/vector.h
vector_wrap.c: $(GISBASE)/include/grass/vect/dig_defines.h
vector_wrap.c: $(GISBASE)/include/grass/vect/dig_structs.h
display_wrap.c: $(GISBASE)/include/grass/display.h
stats_wrap.c: $(GISBASE)/include/grass/stats.h
dbmi_wrap.c: $(GISBASE)/include/grass/dbmi.h
dbmi_wrap.c: $(GISBASE)/include/grass/proto_dbmi.h
g3d_wrap.c: $(GISBASE)/include/grass/G3d.h
arraystats_wrap.c: $(GISBASE)/include/grass/arraystats.h
cluster_wrap.c: $(GISBASE)/include/grass/cluster.h
trans_wrap.c: $(GISBASE)/include/grass/transform.h
vedit_wrap.c: $(GISBASE)/include/grass/vedit.h
