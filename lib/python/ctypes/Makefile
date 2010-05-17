MODULE_TOPDIR = ../../..

PACKAGE = "grasslibs"

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

# doxygen:
DOXNAME=
DOXINPUT=grasspython.dox

MODULES = date grass raster gmath proj imagery vector display stats \
	dbmi g3d arraystats cluster trans vedit

date_LIBS       = $(DATETIMELIB)
grass_LIBS      = $(GISLIB)
raster_LIBS     = $(RASTERLIB)
gmath_LIBS      = $(GMATHLIB)
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

date_INC        = datetime.h P_datetime.h
grass_INC       = gis.h gisdefs.h
raster_INC      = raster.h rasterdefs.h
gmath_INC       = gmath.h
proj_INC        = gprojects.h
imagery_INC     = imagery.h imagedefs.h
vector_INC      = vector.h vect/dig_structs.h vect/dig_defines.h
display_INC     = display.h
stats_INC       = stats.h
dbmi_INC        = dbmi.h
g3d_INC         = G3d.h
arraystats_INC  = arraystats.h
cluster_INC     = cluster.h
trans_INC       = transform.h
vedit_INC       = vedit.h

CTYPESFLAGS = $(INC) $(LIBPATH)
EXTRA_CLEAN_FILES := $(foreach M,$(MODULES),$(M).pyc $(M).py)

ifneq ($(MINGW),)
EXTRA_LIBS = $(INTLLIB)
endif

include $(MODULE_TOPDIR)/include/Make/Python.make

PYDIR = $(ETC)/python
GDIR = $(PYDIR)/grass
DSTDIR = $(GDIR)/lib

PYFILES  := $(patsubst %,$(DSTDIR)/%.py,$(MODULES) __init__)
PYCFILES  := $(patsubst %,$(DSTDIR)/%.pyc,$(MODULES) __init__)
LPYFILES := $(patsubst %,%.py,$(MODULES))

ifneq ($(strip $(CTYPESGEN)),)
default:
	$(MAKE) $(DSTDIR)
	$(MAKE) $(LPYFILES) $(PYFILES) $(PYCFILES)
endif

$(DSTDIR)/%.py: %.py | $(DSTDIR)
	$(INSTALL_DATA) $< $@

%.py: $(%_INC) $(%_LIBS)
	$(CTYPESGEN) $(CTYPESFLAGS) $($*_LIBS) $(EXTRA_LIBS) $(patsubst %.h,$(ARCH_INCDIR)/%.h,$($*_INC)) -o $@

$(PYDIR):
	$(MKDIR) $@

$(GDIR): | $(PYDIR)
	$(MKDIR) $@

$(DSTDIR): | $(GDIR)
	$(MKDIR) $@

.SECONDARY: $(patsubst %,%.py,$(MODULES))
