MODULE_TOPDIR = ../..

PACKAGE = "grasslibs"

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Compile.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

# doxygen:
DOXNAME=
DOXINPUT=grasspython.dox

MODULES = utils date grass math proj imagery vector raster display stats \
	dbmi g3d arraystats cluster trans vedit

date_LIBS       = $(DATETIMELIB)
grass_LIBS      = $(GISLIB)
math_LIBS       = $(GMATHLIB)
proj_LIBS       = $(GPROJLIB)
imagery_LIBS    = $(IMAGERYLIB)
vector_LIBS     = $(VECTLIB)
raster_LIBS     = $(RASTERLIB)
display_LIBS    = $(DISPLAYLIB)
stats_LIBS      = $(STATSLIB)
dbmi_LIBS       = $(DBMILIB)
g3d_LIBS        = $(G3DLIB)
arraystats_LIBS = $(ARRAYSTATSLIB)
cluster_LIBS    = $(CLUSTERLIB)
trans_LIBS      = $(TRANSLIB)
vedit_LIBS      = $(VEDITLIB)

vector_wrap_c_FLAGS = $(VECT_INC)
proj_wrap_c_FLAGS = $(PROJINC) $(GDALCFLAGS)

EXTRA_SWIG = my_typemaps.i common.i
EXTRA_CFLAGS = $(PYMOD_CFLAGS)
EXTRA_CLEAN_FILES := $(foreach M,$(MODULES),$(M)_wrap.o $(M)_wrap.c $(M).pyc $(M).py _$(M).so)
CLEAN_SUBDIRS = NumPtr

include $(MODULE_TOPDIR)/include/Make/Python.make

DSTDIR = $(ARCH_DISTDIR)/etc/python/swig

LIBFILES := $(patsubst %,$(DSTDIR)/_%.so,$(MODULES))
PYFILES  := $(patsubst %,$(DSTDIR)/%.py,$(MODULES) __init__)
PYCFILES := $(patsubst %,$(DSTDIR)/%.pyc,$(MODULES) __init__)

default:
	$(MAKE) $(DSTDIR)
	$(MAKE) $(LIBFILES) $(PYFILES) $(PYCFILES)

$(DSTDIR):
	test -d $@ || $(MKDIR) -p $@

$(DSTDIR)/_%.so: _%.so
	$(INSTALL) $< $@

$(DSTDIR)/%.py: %.py
	$(INSTALL_DATA) $< $@

$(DSTDIR)/%.pyc: %.pyc
	$(INSTALL_DATA) $< $@

numptr:
	$(MAKE) -C NumPtr

.PHONY: numptr

.SECONDARY: $(patsubst %,$(OBJDIR)/%_wrap.o,$(MODULES))

.SECONDARY: $(patsubst %,_%.so,$(MODULES))

date_wrap.c: $(ARCH_INCDIR)/datetime.h
date_wrap.c: $(ARCH_INCDIR)/P_datetime.h
grass_wrap.c: $(ARCH_INCDIR)/gis.h
grass_wrap.c: $(ARCH_INCDIR)/gisdefs.h
math_wrap.c: $(ARCH_INCDIR)/gmath.h
proj_wrap.c: $(ARCH_INCDIR)/gprojects.h
imagery_wrap.c: $(ARCH_INCDIR)/imagedefs.h
imagery_wrap.c: $(ARCH_INCDIR)/imagery.h
vector_wrap.c: $(ARCH_INCDIR)/Vect.h
vector_wrap.c: $(ARCH_INCDIR)/vect/dig_defines.h
vector_wrap.c: $(ARCH_INCDIR)/vect/dig_structs.h
raster_wrap.c: $(ARCH_INCDIR)/raster.h
display_wrap.c: $(ARCH_INCDIR)/display.h
stats_wrap.c: $(ARCH_INCDIR)/stats.h
dbmi_wrap.c: $(ARCH_INCDIR)/dbmi.h
dbmi_wrap.c: $(ARCH_INCDIR)/proto_dbmi.h
g3d_wrap.c: $(ARCH_INCDIR)/G3d.h
arraystats_wrap.c: $(ARCH_INCDIR)/arraystats.h
cluster_wrap.c: $(ARCH_INCDIR)/cluster.h
trans_wrap.c: $(ARCH_INCDIR)/transform.h
vedit_wrap.c: $(ARCH_INCDIR)/vedit.h
utils_wrap.c:
